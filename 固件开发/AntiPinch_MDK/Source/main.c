/* 防夹采集板应用程序：10 路 ADC 采集、卡尔曼滤波、基线判定和 RS485 上报 */
#include "apm32e030.h"
#include "apm32e030_adc.h"
#include "apm32e030_dma.h"
#include "apm32e030_gpio.h"
#include "apm32e030_misc.h"
#include "apm32e030_rcm.h"
#include "apm32e030_tmr.h"
#include "apm32e030_usart.h"
#include "rs485_proto.h"
#include "SEGGER_RTT.h"

#include <stdint.h>

#define ADC_CHANNEL_COUNT       10U    /* ADC0~ADC9 共 10 路 */
#define FILTER_WINDOW            5U    /* 中值滤波窗口长度，卡尔曼前的去毛刺 */
#define ADC_PROCESS_PERIOD_MS   10U    /* 每 10 ms 更新一次滤波器和判定 */
#define ADC_FULL_SCALE        4096U    /* 采样值统一用 4096 去减后再参与运算 */
#define DIVIDER_KOHM             5U    /* 分压电阻 5k 上拉到 VCC，压力膜接地 */
#define RTT_PERIOD_MS          100U    /* 每 100 ms 通过 RTT 打印一次各通道 */
#define RS485_SIM_PERIOD_MS   1000U    /* 每 1 s 在 RTT 上模拟一次 485 回复 */

/* 卡尔曼滤波参数，Q8 定点。Q/R 约 0.026，稳态增益约 0.15 */
#define KALMAN_Q_Q8             27U    /* 过程噪声 */
#define KALMAN_R_Q8           1024U    /* 测量噪声 */

/* 基线：每 3 s 存一个样本，存满 5 分钟，取中位数作为 xn */
#define BASELINE_PERIOD_MS    3000U
#define BASELINE_DEPTH         100U    /* 100 × 3 s = 300 s = 5 分钟 */

/* 冷启动：前 10 s 照常采集和入队，只是不跑防夹判定，避免上电抖动误报 */
#define DETECT_WARMUP_MS     10000U

/* 判定门限直接落在阻值上：阻值跌破基线的 70%（跌幅 >30%）报防夹，
 * 回到 80% 以上才解除，中间那段是迟滞，防止在门限上反复抖动。 */
#define PINCH_TRIP_NUM           7U
#define PINCH_TRIP_DEN          10U
#define PINCH_CLEAR_NUM          8U
#define PINCH_CLEAR_DEN         10U

/* 绝对上限：阻值还在 500 kΩ 以上说明这一路基本没受力，再怎么跌也不报防夹。
 * 解除放宽到 600 kΩ，同样是给一段迟滞，免得刚好卡在 500 kΩ 上反复跳。
 * 单位都是 0.01 kΩ，跟 Resistance_CentiKOhm() 一致。 */
#define PINCH_MAX_CENTI_KOHM  (500U * 100U)
#define PINCH_REL_CENTI_KOHM  (600U * 100U)

/* LED：上电闪 5 下，正常工作呼吸，夹到快闪 */
#define BOOT_BLINK_COUNT         5U
#define BOOT_BLINK_HALF_MS     150U
#define LED_PWM_PERIOD        1000U    /* PWM 比较值范围 0~1000 */
#define BREATH_STEPS            40U    /* 呼吸半周期步数 */
#define BREATH_STEP_MS          40U    /* 40 × 40 ms × 2 = 3.2 s 一个呼吸周期 */
#define FAST_BLINK_HALF_MS      75U    /* 快闪半周期，150 ms 一个周期 */

#define FW_VERSION_MAJOR         0U
#define FW_VERSION_MINOR         2U
#define FW_VERSION_PATCH         0U

#define ADC_DR_ADDR             ((uint32_t)ADC_BASE + 0x40U) /* ADC 数据寄存器地址 */

#define LED_PORT                GPIOB
#define LED_PIN                 GPIO_PIN_5       /* 板上 LED 接在 PB5，低电平点亮 */
#define LED_PIN_SOURCE          GPIO_PIN_SOURCE_5
#define LED_TMR                 TMR3             /* PB5 复用为 TMR3_CH2，用于呼吸灯 PWM */
#define RS485_DE_PORT           GPIOA
#define RS485_DE_PIN            GPIO_PIN_8       /* PA8 控制 RS485 的 DE 和 /RE */
#define RS485_USART             USART1           /* PA9=TX，PA10=RX */
#define RX_RING_SIZE            64U

typedef enum
{
    LED_MODE_BREATH = 0,  /* 正常工作：呼吸灯 */
    LED_MODE_FAST         /* 检测到防夹：快闪 */
} Led_Mode_T;

static volatile uint16_t g_adcDma[ADC_CHANNEL_COUNT]; /* DMA 连续扫描结果 */
static uint16_t g_history[ADC_CHANNEL_COUNT][FILTER_WINDOW]; /* 每路历史采样 */
static uint8_t g_historyIndex; /* 环形历史缓冲区下标 */
static uint32_t g_kalmanX[ADC_CHANNEL_COUNT]; /* 卡尔曼状态估计，Q8 */
static uint32_t g_kalmanP[ADC_CHANNEL_COUNT]; /* 卡尔曼估计协方差，Q8 */
static uint16_t g_baseHist[ADC_CHANNEL_COUNT][BASELINE_DEPTH]; /* 3 s 一个样本的历史队列 */
static uint16_t g_baseIndex; /* 队列写入位置 */
static uint16_t g_baseCount; /* 队列中有效样本数 */
static uint16_t g_baseline[ADC_CHANNEL_COUNT]; /* 各通道 xn，过去 5 分钟中位数 */
static uint16_t g_medianPending; /* 待重算中位数的通道位掩码 */
static uint16_t g_pinchMask; /* 当前处于防夹状态的通道位掩码 */
static uint16_t g_threshold = 0U; /* 触发阈值，ADC 码 0~4096，0x04 设置、0x05 读取 */
static uint8_t g_calibResult;    /* 最近一次校准结果，1 成功 */
static volatile uint32_t g_millis; /* SysTick 毫秒计数 */

static volatile uint8_t g_rxRing[RX_RING_SIZE];
static volatile uint16_t g_rxHead;
static volatile uint16_t g_rxTail;
static uint8_t g_txFrameId; /* 主动上报用的帧 ID，每发一次加 1 */
static uint8_t g_simFrameId; /* RTT 上模拟回复用的帧 ID，跟真实上报分开计数 */

static Led_Mode_T g_ledMode;
static uint32_t g_ledLast;
static uint16_t g_breathPhase;
static uint8_t g_fastOn;

static void Gpio_Init(void);
static void Usart_Init(void);
static void AdcDma_Init(void);
static void LedPwm_Init(void);
static void Filter_Init(void);
static void Filter_Update(void);
static uint16_t Adc_Sample(uint32_t channel);
static uint16_t Median(uint16_t *values, uint32_t count);
static void Kalman_Update(uint32_t channel, uint16_t measurement);
static void Baseline_Push(void);
static void Baseline_ServiceMedian(void);
static void Pinch_Evaluate(void);
static void Led_SetMode(Led_Mode_T mode);
static void Led_Update(uint32_t now);
static uint32_t Resistance_CentiKOhm(uint16_t inverted);
static void Report_AdcRtt(void);
static void Report_BaselineRtt(void);
static void Report_Rs485SimRtt(void);
static void Report_EventRtt(const char *tag, uint32_t channel, uint32_t rNow, uint32_t rBase);
static void Rs485_Service(void);
static void Rs485_HandleFrame(const Rs485_Frame_T *request);
static void Rs485_SendFrame(const Rs485_Frame_T *frame);
static void Rs485_ReportDetectState(void);
static void Rs485_BuildDetectPayload(Rs485_Frame_T *frame);
static void Usart_PutChar(uint8_t ch);
static void DelayMs(uint32_t milliseconds);

void SysTick_Handler(void)
{
    /* SysTick 配置为 1 ms 中断，用于软件定时 */
    g_millis++;
}

void USART1_IRQHandler(void)
{
    uint16_t next;

    /* 接收中断把字节塞进环形缓冲，主循环慢一点也不会丢帧 */
    if (USART_ReadIntFlag(RS485_USART, USART_INT_FLAG_RXBNE) == SET)
    {
        uint8_t byte = (uint8_t)USART_RxData(RS485_USART);

        next = (uint16_t)((g_rxHead + 1U) % RX_RING_SIZE);
        if (next != g_rxTail)
        {
            g_rxRing[g_rxHead] = byte;
            g_rxHead = next;
        }
    }

    /* 过载时清标志，否则接收会一直卡住 */
    if (USART_ReadStatusFlag(RS485_USART, USART_FLAG_OVRE) == SET)
    {
        USART_ClearStatusFlag(RS485_USART, USART_FLAG_OVRE);
    }
}

static void Gpio_Init(void)
{
    GPIO_Config_T config;

    /* 开启 GPIOA/GPIOB 时钟 */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_GPIOA | RCM_AHB_PERIPH_GPIOB);

    /* ADC0~ADC7 使用 PA0~PA7 模拟输入 */
    GPIO_ConfigStructInit(&config);
    config.mode = GPIO_MODE_AN;
    config.pupd = GPIO_PUPD_NO;
    config.pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                 GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_Config(GPIOA, &config);

    /* ADC8、ADC9 使用 PB0、PB1 模拟输入 */
    config.pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_Config(GPIOB, &config);

    GPIO_ConfigStructInit(&config);
    config.mode = GPIO_MODE_OUT;
    config.outtype = GPIO_OUT_TYPE_PP;
    config.speed = GPIO_SPEED_50MHz;
    config.pupd = GPIO_PUPD_NO;
    /* PB5 先当普通输出用于开机闪烁，之后再切到 TMR3 PWM */
    config.pin = LED_PIN;
    GPIO_Config(LED_PORT, &config);
    GPIO_WriteBitValue(LED_PORT, LED_PIN, Bit_SET); /* 低电平点亮，先熄灭 */

    /* PA8 控制 RS485 收发方向，低电平为接收状态 */
    config.pin = RS485_DE_PIN;
    GPIO_Config(RS485_DE_PORT, &config);
    GPIO_WriteBitValue(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);
}

static void Usart_Init(void)
{
    GPIO_Config_T gpioConfig;
    USART_Config_T usartConfig;

    /* 开启 USART1 时钟，并将 PA9/PA10 复用为 USART1 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_USART1);

    GPIO_ConfigPinAF(GPIOA, GPIO_PIN_SOURCE_9, GPIO_AF_PIN1);
    GPIO_ConfigPinAF(GPIOA, GPIO_PIN_SOURCE_10, GPIO_AF_PIN1);

    GPIO_ConfigStructInit(&gpioConfig);
    gpioConfig.mode = GPIO_MODE_AF;
    gpioConfig.outtype = GPIO_OUT_TYPE_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pupd = GPIO_PUPD_PU;
    gpioConfig.pin = GPIO_PIN_9;
    GPIO_Config(GPIOA, &gpioConfig);

    /* RX 同样要走复用功能，否则 USART 收不到数据 */
    gpioConfig.pin = GPIO_PIN_10;
    GPIO_Config(GPIOA, &gpioConfig);

    /* 串口参数：115200 baud、8 数据位、无校验、1 个停止位 */
    USART_ConfigStructInit(&usartConfig);
    usartConfig.baudRate = 115200;
    usartConfig.mode = USART_MODE_TX_RX;
    usartConfig.hardwareFlowCtrl = USART_FLOW_CTRL_NONE;
    usartConfig.parity = USART_PARITY_NONE;
    usartConfig.stopBits = USART_STOP_BIT_1;
    usartConfig.wordLength = USART_WORD_LEN_8B;
    USART_Config(RS485_USART, &usartConfig);

    USART_EnableInterrupt(RS485_USART, USART_INT_RXBNEIE);
    NVIC_EnableIRQRequest(USART1_IRQn, 1);

    USART_Enable(RS485_USART);
    Rs485_ParserReset();
}

static void AdcDma_Init(void)
{
    GPIO_Config_T gpioConfig;
    ADC_Config_T adcConfig;
    DMA_Config_T dmaConfig;
    uint32_t channel;

    /* 开启 ADC1 和 DMA1 时钟 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_ADC1);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

    /* ADC 配置为 12 位、连续转换、通道号递增扫描 */
    ADC_Reset();
    ADC_ConfigStructInit(&adcConfig);
    adcConfig.resolution = ADC_RESOLUTION_12B;
    adcConfig.scanDir = ADC_SCAN_DIR_UPWARD;
    adcConfig.convMode = ADC_CONVERSION_CONTINUOUS;
    adcConfig.dataAlign = ADC_DATA_ALIGN_RIGHT;
    adcConfig.extTrigEdge = ADC_EXT_TRIG_EDGE_NONE;
    adcConfig.extTrigConv = ADC_EXT_TRIG_CONV_TRG0;
    ADC_Config(&adcConfig);

    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        ADC_ConfigChannel(ADC_CHANNEL_0 << channel, ADC_SAMPLE_TIME_239_5);
    }

    /* DMA 将 ADC 数据循环搬运到 g_adcDma[]，CPU 不需要逐次读 ADC */
    DMA_ConfigStructInit(&dmaConfig);
    dmaConfig.peripheralAddress = ADC_DR_ADDR;
    dmaConfig.memoryAddress = (uint32_t)g_adcDma;
    dmaConfig.direction = DMA_DIR_PERIPHERAL;
    dmaConfig.bufferSize = ADC_CHANNEL_COUNT;
    dmaConfig.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATASIZE_HALFWORD;
    dmaConfig.memoryDataSize = DMA_MEMORY_DATASIZE_HALFWORD;
    dmaConfig.circular = DMA_CIRCULAR_ENABLE;
    dmaConfig.memoryTomemory = DMA_M2M_DISABLE;
    dmaConfig.priority = DMA_PRIORITY_LEVEL_HIGHT;
    DMA_Config(DMA1_CHANNEL_1, &dmaConfig);
    DMA_Enable(DMA1_CHANNEL_1);

    GPIO_ConfigStructInit(&gpioConfig);
    gpioConfig.mode = GPIO_MODE_AN;
    gpioConfig.pupd = GPIO_PUPD_NO;
    gpioConfig.pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                     GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_Config(GPIOA, &gpioConfig);
    gpioConfig.pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_Config(GPIOB, &gpioConfig);

    /* ADC 校准后启动循环 DMA 转换 */
    ADC_ReadCalibrationFactor();
    ADC_EnableDMA();
    ADC_DMARequestMode(ADC_DMA_MODE_CIRCULAR);
    ADC_Enable();
    ADC_StartConversion();
}

static void LedPwm_Init(void)
{
    GPIO_Config_T gpioConfig;
    TMR_TimeBase_T timeBase;
    TMR_OCConfig_T ocConfig;

    /* PB5 = TMR3_CH2 (AF1)，用硬件 PWM 做呼吸灯 */
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_TMR3);

    GPIO_ConfigPinAF(LED_PORT, LED_PIN_SOURCE, GPIO_AF_PIN1);
    GPIO_ConfigStructInit(&gpioConfig);
    gpioConfig.mode = GPIO_MODE_AF;
    gpioConfig.outtype = GPIO_OUT_TYPE_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    gpioConfig.pupd = GPIO_PUPD_NO;
    gpioConfig.pin = LED_PIN;
    GPIO_Config(LED_PORT, &gpioConfig);

    /* 8 MHz / 8 = 1 MHz 计数，1000 计数一周期 -> 1 kHz PWM，肉眼无频闪 */
    TMR_ConfigTimeBaseStruct(&timeBase);
    timeBase.div = 7U;
    timeBase.counterMode = TMR_COUNTER_MODE_UP;
    timeBase.period = LED_PWM_PERIOD - 1U;
    timeBase.clockDivision = TMR_CKD_DIV1;
    TMR_ConfigTimeBase(LED_TMR, &timeBase);

    /* LED 低电平点亮，所以输出极性取低：比较值就是亮度 */
    TMR_OCConfigStructInit(&ocConfig);
    ocConfig.OC_Mode = TMR_OC_MODE_PWM1;
    ocConfig.OC_OutputState = TMR_OUTPUT_STATE_ENABLE;
    ocConfig.OC_Polarity = TMR_OC_POLARITY_LOW;
    ocConfig.Pulse = 0U;
    TMR_OC2Config(LED_TMR, &ocConfig);
    TMR_OC2PreloadConfig(LED_TMR, TMR_OC_PRELOAD_ENABLE);

    TMR_EnableAUTOReload(LED_TMR);
    TMR_Enable(LED_TMR);
}

static void Filter_Init(void)
{
    uint32_t channel;
    uint32_t sample;
    uint32_t i;

    /* 用首次采样值填充历史窗口和卡尔曼初值，避免上电时输出从 0 缓慢爬升 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        sample = Adc_Sample(channel);
        for (i = 0U; i < FILTER_WINDOW; i++)
        {
            g_history[channel][i] = (uint16_t)sample;
        }
        g_kalmanX[channel] = sample << 8;
        g_kalmanP[channel] = KALMAN_R_Q8;
        g_baseline[channel] = 0U; /* 基线只来自队列，0 表示还没算出来 */
    }
    g_historyIndex = 0U;
    g_baseIndex = 0U;
    g_baseCount = 0U;
    g_medianPending = 0U;
    g_pinchMask = 0U;
}

static uint16_t Adc_Sample(uint32_t channel)
{
    uint16_t raw = (uint16_t)(g_adcDma[channel] & 0x0FFFU);

    /* 统一用满量程 4096 去减，后面所有环节（卡尔曼、基线、判定）都用这个值 */
    return (uint16_t)(ADC_FULL_SCALE - raw);
}

static uint32_t Resistance_CentiKOhm(uint16_t inverted)
{
    uint32_t adc;

    /* 电路是 VCC—5k—节点—压力膜—GND，节点接 ADC，于是
     *   R = 5k × ADC / (4096 − ADC)
     * 入参 inverted 就是 4096 − ADC，取值 1..4096，不会除零。
     * 返回值单位 0.01 kΩ，方便打印两位小数。 */
    if (inverted == 0U)
    {
        return 0U;
    }
    adc = ADC_FULL_SCALE - inverted;
    return (DIVIDER_KOHM * 100U * adc) / inverted;
}

static uint16_t Median(uint16_t *values, uint32_t count)
{
    uint16_t key;
    uint32_t i;
    uint32_t j;

    /* 插入排序后取中间值。count 最大 100，每 3 s 才排一个通道，开销可以接受 */
    for (i = 1U; i < count; i++)
    {
        key = values[i];
        j = i;
        while ((j > 0U) && (values[j - 1U] > key))
        {
            values[j] = values[j - 1U];
            j--;
        }
        values[j] = key;
    }
    return values[count / 2U];
}

static void Kalman_Update(uint32_t channel, uint16_t measurement)
{
    uint32_t gainQ8;
    int32_t errorQ8;

    /* 标量卡尔曼：预测只加过程噪声，更新按增益向测量值靠拢 */
    g_kalmanP[channel] += KALMAN_Q_Q8;
    gainQ8 = (g_kalmanP[channel] << 8) / (g_kalmanP[channel] + KALMAN_R_Q8);

    errorQ8 = (int32_t)((uint32_t)measurement << 8) - (int32_t)g_kalmanX[channel];
    g_kalmanX[channel] = (uint32_t)((int32_t)g_kalmanX[channel] +
                                    (((int32_t)gainQ8 * errorQ8) >> 8));
    g_kalmanP[channel] = ((256U - gainQ8) * g_kalmanP[channel]) >> 8;
}

static void Filter_Update(void)
{
    uint16_t window[FILTER_WINDOW];
    uint32_t channel;
    uint32_t i;

    /* 每路先做 5 点中值去掉偶发尖峰，再送进卡尔曼滤波 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        g_history[channel][g_historyIndex] = Adc_Sample(channel);
        for (i = 0U; i < FILTER_WINDOW; i++)
        {
            window[i] = g_history[channel][i];
        }
        Kalman_Update(channel, Median(window, FILTER_WINDOW));
    }
    g_historyIndex++;
    if (g_historyIndex >= FILTER_WINDOW)
    {
        g_historyIndex = 0U;
    }
}

static void Baseline_Push(void)
{
    uint32_t channel;

    /* 每 3 s 把当前滤波值压进队列，队列长度覆盖过去 5 分钟 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        g_baseHist[channel][g_baseIndex] = (uint16_t)(g_kalmanX[channel] >> 8);
    }
    g_baseIndex++;
    if (g_baseIndex >= BASELINE_DEPTH)
    {
        g_baseIndex = 0U;
    }
    if (g_baseCount < BASELINE_DEPTH)
    {
        g_baseCount++;
    }
    /* 标记 10 个通道都需要重算中位数，主循环里一次算一路 */
    g_medianPending = (uint16_t)((1U << ADC_CHANNEL_COUNT) - 1U);
}

static void Baseline_ServiceMedian(void)
{
    uint16_t scratch[BASELINE_DEPTH];
    uint32_t channel;
    uint32_t i;
    uint32_t used;

    if (g_medianPending == 0U)
    {
        return;
    }

    /* 一次只重算一个通道，避免 10 路排序集中在一起把主循环卡住 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        if ((g_medianPending & (uint16_t)(1U << channel)) != 0U)
        {
            /* 只取队列里非 0 的样本，0 视为无效数据 */
            used = 0U;
            for (i = 0U; i < g_baseCount; i++)
            {
                if (g_baseHist[channel][i] != 0U)
                {
                    scratch[used] = g_baseHist[channel][i];
                    used++;
                }
            }
            /* 一个有效样本都没有就保持原基线，别把它冲成 0 */
            if (used != 0U)
            {
                g_baseline[channel] = Median(scratch, used);
            }
            g_medianPending &= (uint16_t)~(1U << channel);
            if (g_medianPending == 0U)
            {
                Report_BaselineRtt(); /* 10 路都算完了，打一行基线 */
            }
            break;
        }
    }
}

/* 0x02 手动校准：把当前滤波值直接当作基线，并清空历史队列重新攒 5 分钟。
 * 返回 1 表示成功。基线为 0 的通道（还没有有效滤波值）视为失败。 */
static uint8_t Baseline_Calibrate(void)
{
    uint32_t channel;

    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        if ((g_kalmanX[channel] >> 8) == 0U)
        {
            return 0U;
        }
    }
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        g_baseline[channel] = (uint16_t)(g_kalmanX[channel] >> 8);
    }
    g_baseCount = 0U;
    g_baseIndex = 0U;
    return 1U;
}

/* 10 个通道按 1234 1234 12 循环分配到 4 个位置：通道 i 属于位置 (i % 4) + 1 */
static uint8_t Position_Of(uint32_t channel)
{
    return (uint8_t)((channel % RS485_POSITION_COUNT) + 1U);
}

/* 位置 p（1~4）只要有一个通道异常就算异常 */
static uint8_t Position_IsAbnormal(uint8_t position)
{
    uint32_t channel;

    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        if ((Position_Of(channel) == position) &&
            ((g_pinchMask & (uint16_t)(1U << channel)) != 0U))
        {
            return 1U;
        }
    }
    return 0U;
}

static void Pinch_Evaluate(void)
{
    uint32_t channel;
    uint32_t base;
    uint32_t rNow;
    uint32_t rBase;
    uint16_t bit;
    uint16_t before = g_pinchMask;

    /* 冷启动前 10 s 只采集不判定 */
    if (g_millis < DETECT_WARMUP_MS)
    {
        return;
    }

    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        bit = (uint16_t)(1U << channel);
        base = g_baseline[channel];

        /* 基线为 0 说明队列里还没算出中位数，这一路先不判 */
        if (base == 0U)
        {
            g_pinchMask &= (uint16_t)~bit;
            continue;
        }

        /* 判定直接在阻值上做：当前阻值和基线阻值，单位都是 0.01 kΩ */
        rNow = Resistance_CentiKOhm((uint16_t)(g_kalmanX[channel] >> 8));
        rBase = Resistance_CentiKOhm(base);
        if (rBase == 0U)
        {
            g_pinchMask &= (uint16_t)~bit;
            continue;
        }

        if ((g_pinchMask & bit) == 0U)
        {
            /* 两个条件都满足才判防夹：跌幅超过 30%，并且阻值已经落到 500 kΩ 以内 */
            if (((rNow * PINCH_TRIP_DEN) < (rBase * PINCH_TRIP_NUM)) &&
                (rNow <= PINCH_MAX_CENTI_KOHM))
            {
                g_pinchMask |= bit;
                Report_EventRtt("PINCH", channel, rNow, rBase);
            }
        }
        else
        {
            /* 回到基线的 80% 以上，或者阻值升回 600 kΩ 以上，就解除 */
            if (((rNow * PINCH_CLEAR_DEN) > (rBase * PINCH_CLEAR_NUM)) ||
                (rNow > PINCH_REL_CENTI_KOHM))
            {
                g_pinchMask &= (uint16_t)~bit;
                Report_EventRtt("CLEAR", channel, rNow, rBase);
            }
        }
    }

    /* 只在整体状态翻转时切灯效并上报一次 485 */
    if ((before == 0U) != (g_pinchMask == 0U))
    {
        Led_SetMode((g_pinchMask != 0U) ? LED_MODE_FAST : LED_MODE_BREATH);
        Rs485_ReportDetectState();
    }
}

static void Led_SetMode(Led_Mode_T mode)
{
    if (g_ledMode != mode)
    {
        g_ledMode = mode;
        g_ledLast = g_millis;
        g_breathPhase = 0U;
        g_fastOn = 0U;
    }
}

static void Led_Update(uint32_t now)
{
    uint32_t level;

    if (g_ledMode == LED_MODE_FAST)
    {
        /* 快闪：全亮和全灭交替 */
        if ((now - g_ledLast) < FAST_BLINK_HALF_MS)
        {
            return;
        }
        g_ledLast = now;
        g_fastOn ^= 1U;
        TMR_SetCompare2(LED_TMR, (g_fastOn != 0U) ? LED_PWM_PERIOD : 0U);
        return;
    }

    /* 呼吸：三角波相位配平方曲线，视觉上亮度变化比较均匀 */
    if ((now - g_ledLast) < BREATH_STEP_MS)
    {
        return;
    }
    g_ledLast = now;
    g_breathPhase++;
    if (g_breathPhase >= (BREATH_STEPS * 2U))
    {
        g_breathPhase = 0U;
    }
    level = (g_breathPhase < BREATH_STEPS)
                ? g_breathPhase
                : ((BREATH_STEPS * 2U) - 1U - g_breathPhase);
    TMR_SetCompare2(LED_TMR, (level * level * LED_PWM_PERIOD) /
                             ((BREATH_STEPS - 1U) * (BREATH_STEPS - 1U)));
}

static void Report_EventRtt(const char *tag, uint32_t channel, uint32_t rNow, uint32_t rBase)
{
    /* 事件行只打阻值：当前阻值、基线阻值，以及相对基线的百分比 */
    SEGGER_RTT_printf(0, "[%6u ms] %s ch%u  R=%u.%02u kR  xn=%u.%02u kR  (%u%% of xn)\r\n",
                      (unsigned)g_millis, tag, (unsigned)channel,
                      (unsigned)(rNow / 100U), (unsigned)(rNow % 100U),
                      (unsigned)(rBase / 100U), (unsigned)(rBase % 100U),
                      (unsigned)((rNow * 100U) / rBase));
}

static void Report_AdcRtt(void)
{
    uint32_t channel;
    uint32_t ohm;

    /* 周期打印各通道当前阻值，单位 kΩ，不再打 ADC 码值 */
    SEGGER_RTT_printf(0, "t=%6u ms |", (unsigned)g_millis);
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        ohm = Resistance_CentiKOhm((uint16_t)(g_kalmanX[channel] >> 8));
        SEGGER_RTT_printf(0, " %5u.%02u", (unsigned)(ohm / 100U), (unsigned)(ohm % 100U));
    }
    SEGGER_RTT_printf(0, " kR | pinch=0x%03x\r\n", (unsigned)g_pinchMask);
}

static void Report_BaselineRtt(void)
{
    uint32_t channel;
    uint32_t ohm;

    /* 中位数刷新后打一行基线阻值，方便对照 */
    SEGGER_RTT_printf(0, "baseline xn |");
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        ohm = Resistance_CentiKOhm(g_baseline[channel]);
        SEGGER_RTT_printf(0, " %5u.%02u", (unsigned)(ohm / 100U), (unsigned)(ohm % 100U));
    }
    SEGGER_RTT_printf(0, " kR\r\n");
}

static void Report_Rs485SimRtt(void)
{
    Rs485_Frame_T frame;
    uint8_t buffer[RS485_MAX_FRAME];
    uint32_t length;
    uint32_t i;

    /* 按"主机查询 0x01 检测状态"构造一帧回复，用的是真实的组帧函数，
     * 所以打出来的字节跟实际会上总线的完全一致。这里只打印，不发 485。 */
    frame.status = RS485_STATUS_REPLY;
    frame.frameId = g_simFrameId++;
    frame.srcId = RS485_MODULE_ID;
    frame.dstId = RS485_HOST_ID;
    Rs485_BuildDetectPayload(&frame);

    length = Rs485_BuildFrame(buffer, sizeof(buffer), &frame);
    SEGGER_RTT_printf(0, "[%6u ms] RS485 reply 0x01 ->", (unsigned)g_millis);
    for (i = 0U; i < length; i++)
    {
        SEGGER_RTT_printf(0, " %02X", (unsigned)buffer[i]);
    }
    SEGGER_RTT_printf(0, "  (%s)\r\n",
                      (g_pinchMask != 0U) ? "ABNORMAL" : "NORMAL");
}

static void Usart_PutChar(uint8_t ch)
{
    /* 等发送寄存器空出来再写入下一个字节 */
    while (USART_ReadStatusFlag(RS485_USART, USART_FLAG_TXBE) == RESET)
    {
    }
    USART_TxData(RS485_USART, ch);
}

static void Rs485_SendFrame(const Rs485_Frame_T *frame)
{
    uint8_t buffer[RS485_MAX_FRAME];
    uint32_t length;
    uint32_t i;

    length = Rs485_BuildFrame(buffer, sizeof(buffer), frame);
    if (length == 0U)
    {
        return;
    }

    /* DE 和 /RE 共用，置高进入发送状态 */
    GPIO_WriteBitValue(RS485_DE_PORT, RS485_DE_PIN, Bit_SET);
    for (i = 0U; i < length; i++)
    {
        Usart_PutChar(buffer[i]);
    }
    /* 等最后一个字节真正发完，再切回接收 */
    while (USART_ReadStatusFlag(RS485_USART, USART_FLAG_TXC) == RESET)
    {
    }
    GPIO_WriteBitValue(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);
}

/* 0x01 回复：[0x01, 位置1, 位置2, 位置3, 位置4]，每个位置 0 正常 / 1 异常 */
static void Rs485_BuildDetectPayload(Rs485_Frame_T *frame)
{
    uint8_t position;

    frame->dataLen = (uint16_t)(1U + RS485_POSITION_COUNT);
    frame->data[0] = RS485_CMD_DETECT_STATE;
    for (position = 1U; position <= RS485_POSITION_COUNT; position++)
    {
        frame->data[position] = Position_IsAbnormal(position) ? RS485_DETECT_ABNORMAL
                                                              : RS485_DETECT_NORMAL;
    }
}

static void Rs485_ReportDetectState(void)
{
    Rs485_Frame_T frame;

    /* 主动上报防夹状态。协议只定义了主机问、模块答，这里是额外的事件推送 */
    frame.status = RS485_STATUS_REQUEST;
    frame.frameId = g_txFrameId++;
    frame.srcId = RS485_MODULE_ID;
    frame.dstId = RS485_HOST_ID;
    Rs485_BuildDetectPayload(&frame);
    Rs485_SendFrame(&frame);
}

static void Rs485_HandleFrame(const Rs485_Frame_T *request)
{
    Rs485_Frame_T reply;

    /* 只理会发给本模块的请求帧 */
    if ((request->dstId != RS485_MODULE_ID) ||
        (request->status != RS485_STATUS_REQUEST) ||
        (request->dataLen < 1U))
    {
        return;
    }

    reply.status = RS485_STATUS_REPLY;
    reply.frameId = request->frameId; /* 回复沿用请求的帧 ID */
    reply.srcId = RS485_MODULE_ID;
    reply.dstId = request->srcId;

    switch (request->data[0])
    {
        case RS485_CMD_DETECT_STATE:
            Rs485_BuildDetectPayload(&reply);
            break;

        case RS485_CMD_CALIBRATE:
            /* 校准只是把当前滤波值拷进基线，几微秒，直接在这里做完再回结果 */
            g_calibResult = Baseline_Calibrate();
            reply.dataLen = 2U;
            reply.data[0] = RS485_CMD_CALIBRATE;
            reply.data[1] = (g_calibResult != 0U) ? RS485_CALIB_OK : RS485_CALIB_FAIL;
            break;

        case RS485_CMD_THRESH_SET:
            if (request->dataLen < 3U)
            {
                return;
            }
            /* 阈值小端 2 字节，超出 0~4096 的按 4096 截断 */
            g_threshold = (uint16_t)(request->data[1] | ((uint16_t)request->data[2] << 8));
            if (g_threshold > ADC_FULL_SCALE)
            {
                g_threshold = (uint16_t)ADC_FULL_SCALE;
            }
            reply.dataLen = 3U;
            reply.data[0] = RS485_CMD_THRESH_SET;
            reply.data[1] = (uint8_t)(g_threshold & 0xFFU);   /* 回显 */
            reply.data[2] = (uint8_t)(g_threshold >> 8);
            break;

        case RS485_CMD_THRESH_GET:
            reply.dataLen = 3U;
            reply.data[0] = RS485_CMD_THRESH_GET;
            reply.data[1] = (uint8_t)(g_threshold & 0xFFU);
            reply.data[2] = (uint8_t)(g_threshold >> 8);
            break;

        case RS485_CMD_FW_VERSION:
            reply.dataLen = 4U;
            reply.data[0] = RS485_CMD_FW_VERSION;
            reply.data[1] = FW_VERSION_MAJOR;
            reply.data[2] = FW_VERSION_MINOR;
            reply.data[3] = FW_VERSION_PATCH;
            break;

        default:
            return; /* 未定义功能码，不回复 */
    }

    Rs485_SendFrame(&reply);
}

static void Rs485_Service(void)
{
    Rs485_Frame_T frame;

    /* 把中断收到的字节喂给解析器，收齐一帧就处理 */
    while (g_rxTail != g_rxHead)
    {
        uint8_t byte = g_rxRing[g_rxTail];

        g_rxTail = (uint16_t)((g_rxTail + 1U) % RX_RING_SIZE);
        if (Rs485_ParserFeed(byte, &frame) != 0U)
        {
            Rs485_HandleFrame(&frame);
        }
    }
}

/* RS485 上跑的是协议二进制帧，不再往这条串口 printf，调试输出一律走 RTT */

static void DelayMs(uint32_t milliseconds)
{
    uint32_t start = g_millis;
    /* 等待 SysTick 计时到指定毫秒数 */
    while ((g_millis - start) < milliseconds)
    {
    }
}

static void Led_BootBlink(void)
{
    uint32_t i;

    /* 上电时 LED 闪 5 下，作为固件启动的可见标志 */
    for (i = 0U; i < BOOT_BLINK_COUNT; i++)
    {
        GPIO_WriteBitValue(LED_PORT, LED_PIN, Bit_RESET); /* 低电平点亮 */
        DelayMs(BOOT_BLINK_HALF_MS);
        GPIO_WriteBitValue(LED_PORT, LED_PIN, Bit_SET);   /* 高电平熄灭 */
        DelayMs(BOOT_BLINK_HALF_MS);
    }
}

int main(void)
{
    uint32_t lastAdcProcess = 0U;
    uint32_t lastRtt = 0U;
    uint32_t lastBaseline = 0U;
    uint32_t lastRs485Sim = 0U;

    /* 系统使用内部 8 MHz 时钟，初始化 1 ms SysTick */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);

    /* 先把 PB5 当普通输出闪 5 下，再切成 PWM 做呼吸灯 */
    Gpio_Init();
    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "\r\n=== AntiPinch v%u.%u.%u boot, core=%u Hz ===\r\n",
                      (unsigned)FW_VERSION_MAJOR, (unsigned)FW_VERSION_MINOR,
                      (unsigned)FW_VERSION_PATCH, (unsigned)SystemCoreClock);
    Led_BootBlink();
    LedPwm_Init();

    Usart_Init();
    AdcDma_Init();
    DelayMs(20U);
    Filter_Init();

    g_ledMode = LED_MODE_FAST; /* 先设成另一个模式，让 Led_SetMode 真正生效 */
    Led_SetMode(LED_MODE_BREATH);
    lastBaseline = g_millis;

    SEGGER_RTT_printf(0, "ready: ch0..ch%u, R = %u k x ADC / (4096 - ADC), printed in kR\r\n",
                      (unsigned)(ADC_CHANNEL_COUNT - 1U), (unsigned)DIVIDER_KOHM);
    SEGGER_RTT_printf(0, "       baseline = median of nonzero samples over %u s, "
                         "trip below %u%% of xn and under %u kR, clear above %u%% or over %u kR, "
                         "detect after %u s\r\n",
                      (unsigned)((BASELINE_DEPTH * BASELINE_PERIOD_MS) / 1000U),
                      (unsigned)((PINCH_TRIP_NUM * 100U) / PINCH_TRIP_DEN),
                      (unsigned)(PINCH_MAX_CENTI_KOHM / 100U),
                      (unsigned)((PINCH_CLEAR_NUM * 100U) / PINCH_CLEAR_DEN),
                      (unsigned)(PINCH_REL_CENTI_KOHM / 100U),
                      (unsigned)(DETECT_WARMUP_MS / 1000U));

    while (1)
    {
        uint32_t now = g_millis;

        /* 10 ms 更新一次 10 路卡尔曼滤波，并做一次防夹判定 */
        if ((now - lastAdcProcess) >= ADC_PROCESS_PERIOD_MS)
        {
            lastAdcProcess = now;
            Filter_Update();
            Pinch_Evaluate();
        }

        /* 每 3 s 往基线队列里压一个样本 */
        if ((now - lastBaseline) >= BASELINE_PERIOD_MS)
        {
            lastBaseline = now;
            Baseline_Push();
        }

        /* 每 100 ms 通过 RTT 打印一次各通道，方便在线观察 */
        if ((now - lastRtt) >= RTT_PERIOD_MS)
        {
            lastRtt = now;
            Report_AdcRtt();
        }

        /* 每 1 s 在 RTT 上模拟打印一次 485 回复帧 */
        if ((now - lastRs485Sim) >= RS485_SIM_PERIOD_MS)
        {
            lastRs485Sim = now;
            Report_Rs485SimRtt();
        }

        Baseline_ServiceMedian(); /* 每轮最多重算一个通道的中位数 */
        Led_Update(now);
        Rs485_Service();
    }
}

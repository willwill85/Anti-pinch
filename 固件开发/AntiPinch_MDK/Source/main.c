/* 防夹采集板应用程序：10 路 ADC 采集、滤波和 RS485 输出 */
#include "apm32e030.h"
#include "apm32e030_adc.h"
#include "apm32e030_dma.h"
#include "apm32e030_gpio.h"
#include "apm32e030_misc.h"
#include "apm32e030_rcm.h"
#include "apm32e030_usart.h"
#include <stdio.h>
#include <stdint.h>

#define ADC_CHANNEL_COUNT       10U    /* ADC0~ADC9 共 10 路 */
#define FILTER_WINDOW            5U    /* 中值滤波窗口长度 */
#define ADC_PROCESS_PERIOD_MS   10U    /* 每 10 ms 更新一次滤波器 */
#define REPORT_PERIOD_MS       1000U    /* 每秒发送一次 ADC 数据 */
#define ADC_DR_ADDR             ((uint32_t)ADC_BASE + 0x40U) /* ADC 数据寄存器地址 */

#define LED_PORT                GPIOB
#define LED_PIN                 GPIO_PIN_5       /* 板上 LED 接在 PB5，低电平点亮 */
#define RS485_DE_PORT           GPIOA
#define RS485_DE_PIN            GPIO_PIN_8       /* PA8 控制 RS485 的 DE 和 /RE */
#define RS485_USART             USART1           /* PA9=TX，PA10=RX */

static volatile uint16_t g_adcDma[ADC_CHANNEL_COUNT]; /* DMA 连续扫描结果 */
static uint16_t g_history[ADC_CHANNEL_COUNT][FILTER_WINDOW]; /* 每路历史采样 */
static uint32_t g_filteredQ8[ADC_CHANNEL_COUNT]; /* Q8 定点格式的滤波值 */
static uint8_t g_historyIndex; /* 环形历史缓冲区下标 */
static volatile uint32_t g_millis; /* SysTick 毫秒计数 */

static void Gpio_Init(void);
static void Usart_Init(void);
static void AdcDma_Init(void);
static void Filter_Init(void);
static void Filter_Update(void);
static uint16_t Median5(const uint16_t *values);
static void Report_Adc(void);
static void Rs485_SendStart(void);
static void Rs485_SendStop(void);
static void DelayMs(uint32_t milliseconds);

void SysTick_Handler(void)
{
    /* SysTick 配置为 1 ms 中断，用于软件定时 */
    g_millis++;
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
    /* PB5 推挽输出，低电平点亮 LED */
    config.pin = LED_PIN;
    GPIO_Config(LED_PORT, &config);
    GPIO_WriteBitValue(LED_PORT, LED_PIN, Bit_RESET); /* LED is active low: always on */

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

    gpioConfig.mode = GPIO_MODE_IN;
    gpioConfig.pupd = GPIO_PUPD_NO;
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
    USART_Enable(RS485_USART);
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

static void Filter_Init(void)
{
    uint32_t channel;
    uint32_t sample;
    uint32_t i;

    /* 用首次采样值填充历史窗口，避免上电时滤波输出从 0 缓慢爬升 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        sample = g_adcDma[channel] & 0x0FFFU;
        for (i = 0U; i < FILTER_WINDOW; i++)
        {
            g_history[channel][i] = (uint16_t)sample;
        }
        g_filteredQ8[channel] = sample << 8;
    }
    g_historyIndex = 0U;
}

static uint16_t Median5(const uint16_t *values)
{
    uint16_t sorted[FILTER_WINDOW];
    uint16_t key;
    uint32_t i;
    uint32_t j;

    /* 对 5 个样本排序，取中间值抑制偶发尖峰 */
    for (i = 0U; i < FILTER_WINDOW; i++)
    {
        sorted[i] = values[i];
    }
    for (i = 1U; i < FILTER_WINDOW; i++)
    {
        key = sorted[i];
        j = i;
        while ((j > 0U) && (sorted[j - 1U] > key))
        {
            sorted[j] = sorted[j - 1U];
            j--;
        }
        sorted[j] = key;
    }
    return sorted[2];
}

static void Filter_Update(void)
{
    uint16_t window[FILTER_WINDOW];
    uint32_t channel;
    uint32_t i;
    uint16_t median;
    int32_t targetQ8;
    int32_t errorQ8;

    /* 每路先做 5 点中值，再用 alpha=1/4 的一阶 IIR 平滑 */
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        g_history[channel][g_historyIndex] = g_adcDma[channel] & 0x0FFFU;
        for (i = 0U; i < FILTER_WINDOW; i++)
        {
            window[i] = g_history[channel][i];
        }
        median = Median5(window);
        targetQ8 = (int32_t)median << 8;
        errorQ8 = targetQ8 - (int32_t)g_filteredQ8[channel];
        g_filteredQ8[channel] = (uint32_t)((int32_t)g_filteredQ8[channel] + errorQ8 / 4);
    }
    g_historyIndex++;
    if (g_historyIndex >= FILTER_WINDOW)
    {
        g_historyIndex = 0U;
    }
}

static void Rs485_SendStart(void)
{
    /* DE 和 /RE 共用，置高后进入发送状态 */
    GPIO_WriteBitValue(RS485_DE_PORT, RS485_DE_PIN, Bit_SET);
}

static void Rs485_SendStop(void)
{
    /* 等待最后一个字节真正发送完，再切回接收状态 */
    while (USART_ReadStatusFlag(RS485_USART, USART_FLAG_TXC) == RESET)
    {
    }
    GPIO_WriteBitValue(RS485_DE_PORT, RS485_DE_PIN, Bit_RESET);
}

static void Report_Adc(void)
{
    uint32_t channel;
    uint32_t filtered;

    /* 每秒输出 10 路滤波后的 ADC 码值，不输出原始值和电压值 */
    Rs485_SendStart();
    printf("ADC,");
    for (channel = 0U; channel < ADC_CHANNEL_COUNT; channel++)
    {
        filtered = g_filteredQ8[channel] >> 8;
        printf("ch%lu=%lu", (unsigned long)channel, (unsigned long)filtered);
        if (channel + 1U != ADC_CHANNEL_COUNT)
        {
            printf(",");
        }
    }
    printf("\r\n");
    Rs485_SendStop();
}

static void DelayMs(uint32_t milliseconds)
{
    uint32_t start = g_millis;
    /* 等待 SysTick 计时到指定毫秒数 */
    while ((g_millis - start) < milliseconds)
    {
    }
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    /* 重定向 printf 到 USART1 */
    while (USART_ReadStatusFlag(RS485_USART, USART_FLAG_TXBE) == RESET)
    {
    }
    USART_TxData(RS485_USART, (uint8_t)ch);
    return ch;
}

int main(void)
{
    uint32_t lastAdcProcess = 0U;
    uint32_t lastReport = 0U;

    /* 系统使用内部 8 MHz 时钟，初始化 1 ms SysTick */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
    /* 初始化 GPIO、串口、ADC+DMA，并等待 ADC 填充初始数据 */
    Gpio_Init();
    Usart_Init();
    AdcDma_Init();
    DelayMs(20U);
    Filter_Init();

    while (1)
    {
        uint32_t now = g_millis;

        /* 10 ms 更新一次 10 路 ADC 滤波值 */
        if ((now - lastAdcProcess) >= ADC_PROCESS_PERIOD_MS)
        {
            lastAdcProcess = now;
            Filter_Update();
        }

        /* 每秒通过 RS485 输出一次滤波结果 */
        if ((now - lastReport) >= REPORT_PERIOD_MS)
        {
            lastReport = now;
            Report_Adc();
        }

    }
}

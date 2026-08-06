/*!
 * @file        main.c
 *
 * @brief       Main program body
 *
 * @version     V1.0.3
 *
 * @date        2025-07-03
 *
 * @attention
 *
 *  Copyright (C) 2024-2025 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Includes */
#include "stdio.h"
#include "string.h"
#include "apm32e030_i2c.h"
#include "apm32e030_misc.h"
#include "apm32e030_eint.h"
#include "apm32e030_rcm.h"
#include "apm32e030_dma.h"
#include "Board.h"

/** @addtogroup Examples
  @{
  */

/** @addtogroup I2C_TwoBoards_Master
  @{
  */

/** @defgroup I2C_TwoBoards_Master_Marcos Marcos
  @{
  */

/* printf function configs to USART1*/
#define DEBUG_USART  USART1

/* Waiting TIMEOUT */
#define I2CT_FLAG_TIMEOUT         ((uint32_t)0x1000)
#define I2CT_LONG_TIMEOUT         ((uint32_t)(10 * I2CT_FLAG_TIMEOUT))
static volatile uint32_t I2CTimeout = I2CT_LONG_TIMEOUT;

/* Debug Information output */
#define I2C_DEBUG_ON         1
#define I2C_ERROR(fmt,arg...)      printf("<<-Master-I2C_ERROR->> "fmt"\n",##arg)

/**@} end of group I2C_TwoBoards_Master_Marcos */

/** @defgroup I2C_TwoBoards_Master_Enumerations Enumerations
  @{
  */

/**@} end of group I2C_TwoBoards_Master_Enumerations */

/** @defgroup I2C_TwoBoards_Master_Structures Structures
  @{
  */

/**@} end of group I2C_TwoBoards_Master_Structures */

/** @defgroup I2C_TwoBoards_Master_Variables Variables
  @{
  */
uint8_t BufSize = 0;
uint8_t I2C_Buffer_TX[] = "Hello master\r\n";

/**@} end of group I2C_TwoBoards_Master_Variables */

/** @defgroup I2C_TwoBoards_Master_Functions Functions
  @{
  */

/* I2C Init */
void I2CInit(void);
/* DMA Init */
void DMAInit(void);
/* I2C Write */
uint32_t I2C_Write(I2C_T* i2c);
/* Delay */
void Delay(uint32_t count);

/*!
 * @brief       Main program
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
int main(void)
{

    APM_TINY_LEDInit(LED3);
    APM_TINY_PBInit(BUTTON_KEY1, BUTTON_MODE_EINT);
    APM_TINY_COMInit(COM1);
    I2CInit();
    DMAInit();

    for (;;)
    {
        /* Press the BUTTON_KEY1, slave will send data to
        master along with information that showed in serial port */
        APM_TINY_LEDToggle(LED3);

        Delay(100000);
    }
}

/*!
 * @brief       Delay
 *
 * @param       count:  delay count
 *
.
 * @retval      None
 *
 * @note
 */
void Delay(uint32_t count)
{
    volatile uint32_t delay = count;

    while (delay--);
}

/*!
 * @brief        Used to locate the wrong location of the
 *              I2C_Write()
 *
 * @param        errorCode
 *
 * @retval      Error code location information
 *
 * @note
 */
uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
    if (I2C_DEBUG_ON)
    {
        I2C_ERROR("I2C Wait TIMEOUT! errorCode = %d\r\n", errorCode);
    }

    return 0;
}

/*!
 * @brief       I2C Init
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void I2CInit(void)
{
    GPIO_Config_T gpioConfigStruct;
    I2C_Config_T i2cConfigStruct;
    /* Enable I2C related Clock */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_GPIOB);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_I2C1);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SYSCFG);

    /* Free I2C_SCL and I2C_SDA */
    gpioConfigStruct.mode = GPIO_MODE_OUT;
    gpioConfigStruct.speed = GPIO_SPEED_50MHz;
    gpioConfigStruct.outtype = GPIO_OUT_TYPE_PP;
    gpioConfigStruct.pupd = GPIO_PUPD_NO;
    gpioConfigStruct.pin = GPIO_PIN_8;
    GPIO_Config(GPIOB, &gpioConfigStruct);

    gpioConfigStruct.pin = GPIO_PIN_9;
    GPIO_Config(GPIOB, &gpioConfigStruct);

    GPIO_SetBit(GPIOB, GPIO_PIN_8);
    GPIO_SetBit(GPIOB, GPIO_PIN_9);

    /* Connect I2C to SCL */
    GPIO_ConfigPinAF(GPIOB, GPIO_PIN_SOURCE_8, GPIO_AF_PIN1);
    /* Connect I2C to SDA */
    GPIO_ConfigPinAF(GPIOB, GPIO_PIN_SOURCE_9, GPIO_AF_PIN1);
    /*  Config I2C1 GPIO */
    gpioConfigStruct.mode = GPIO_MODE_AF;
    gpioConfigStruct.speed = GPIO_SPEED_50MHz;
    gpioConfigStruct.outtype = GPIO_OUT_TYPE_OD;
    gpioConfigStruct.pupd = GPIO_PUPD_NO;
    gpioConfigStruct.pin = GPIO_PIN_8;
    GPIO_Config(GPIOB, &gpioConfigStruct);

    gpioConfigStruct.pin = GPIO_PIN_9;
    GPIO_Config(GPIOB, &gpioConfigStruct);

    /* Config I2C1 */
    I2C_Reset(I2C1);
    RCM_ConfigI2CCLK(RCM_I2C1CLK_SYSCLK);
    i2cConfigStruct.ack = I2C_ACK_ENABLE;
    i2cConfigStruct.ackaddress = I2C_ACK_ADDRESS_7BIT;
    i2cConfigStruct.address1 = 0XA0;
    i2cConfigStruct.analogfilter = I2C_ANALOG_FILTER_ENABLE;
    i2cConfigStruct.digitalfilter = I2C_DIGITAL_FILTER_0;
    i2cConfigStruct.mode = I2C_MODE_I2C;
    i2cConfigStruct.timing = 0xB0420F13;
    I2C_Config(I2C1, &i2cConfigStruct);

    /* Enable the I2C1 Interrupt */
    I2C_EnableInterrupt(I2C1, I2C_INT_RXIE | I2C_INT_ADDRIE | I2C_INT_STOPIE);

    /* NVIC configuration */
    NVIC_EnableIRQRequest(I2C1_IRQn, 1);

    /* slave configuration */
    I2C_EnableSlaveByteControl(I2C1);
    I2C_EnableReload(I2C1);
    I2C_EnableStretchClock(I2C1);

    /* Enable I2Cx */
    I2C_Enable(I2C1);
}

/*!
 * @brief       DMA Init
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void DMAInit(void)
{
    BufSize = sizeof(I2C_Buffer_TX) - 1;

    /* Enable DMA clock */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);
    /* DMA Configure */
    DMA_Config_T dmaConfig;
    /* size of buffer*/
    dmaConfig.bufferSize = BufSize;
    /* set memory Data Size*/
    dmaConfig.memoryDataSize = DMA_MEMORY_DATASIZE_BYTE;
    /* Set peripheral Data Size*/
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATASIZE_BYTE;
    /* Enable Memory Address increase*/
    dmaConfig.memoryInc = DMA_MEMORY_INC_ENABLE;
    /* Disable Peripheral Address increase*/
    dmaConfig.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    /* Reset Circular Mode*/
    dmaConfig.circular = DMA_CIRCULAR_DISABLE;
    /* Disable M2M*/
    dmaConfig.memoryTomemory = DMA_M2M_DISABLE;
    /* set priority*/
    dmaConfig.priority = DMA_PRIORITY_LEVEL_HIGHT;
    /* read from memory*/
    dmaConfig.direction = DMA_DIR_MEMORY;
    /* Set memory Address*/
    dmaConfig.memoryAddress = (uint32_t)I2C_Buffer_TX;
    /* Set Peripheral Address*/
    dmaConfig.peripheralAddress = (uint32_t)&I2C1->TXDATA;

    DMA_Config(DMA1_CHANNEL_2, &dmaConfig);
    /* Clear DMA TF flag*/
    DMA_ClearIntFlag(DMA1_INT_FLAG_TF2);
    /* Enable DMA Interrupt*/
    DMA_EnableInterrupt(DMA1_CHANNEL_2, DMA_INT_TFIE);

    NVIC_EnableIRQRequest(DMA1_CH2_3_IRQn, 2);

    I2C_EnableDMA(I2C1, I2C_DMA_REQ_TX);
}

/*!
 * @brief    I2C Write
 *
 * @param    I2C: Specifies the I2C to be configured.
 *           This parameter can be one of following parameters:
 *           @arg I2C1
 *           @arg I2C2
 *
 * @param    *dat: Pointer to data
 *
 * @retval   0 or ERROR Information
 *
 * @note
 */
uint32_t I2C_Write(I2C_T* i2c)
{
    I2CTimeout = I2CT_LONG_TIMEOUT;
    /* Wait until I2C is idle */
    while (I2C_ReadStatusFlag(i2c, I2C_FLAG_BUSY) == SET)
    {
        I2CInit();
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(1);
    }
    /* Disable I2C interrupt */
    I2C_DisableInterrupt(i2c, I2C_INT_RXIE | I2C_INT_ADDRIE | I2C_INT_STOPIE);
    I2C_DisableReload(I2C1);
    /* I2C Send Configuration */
    I2C_HandlingTransfer(i2c, 0xB0, BufSize, I2C_RELOAD_MODE_AUTOEND, I2C_GENERATE_START_WRITE);

    /* I2C Send Data */
    DMA_SetDataNumber(DMA1_CHANNEL_2, BufSize);
    DMA_Enable(DMA1_CHANNEL_2);

    return 0;
}

/*!
 * @brief       I2C interrupt service routine
 *
 * @param       None
 *
 * @retval      None
 *
 * @note        This function need to put into I2C1_IRQHandler() in apm32e030_int.c
 */
void  I2C_Isr(void)
{
    uint8_t dat;

    if (I2C_ReadIntFlag(I2C1, I2C_INT_FLAG_ADDR) == SET)
    {
        /* The interrupt flag bit must be cleared first. */
        I2C_ClearIntFlag(I2C1, I2C_INT_FLAG_ADDR);
        I2C_ConfigNumberOfBytes(I2C1, 1);
    }

    if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_RXBNE) == SET)
    {
        /* I2C_ConfigNumberOfBytes is necessary*/
        I2C_ConfigNumberOfBytes(I2C1, 1);
        dat = (uint8_t)I2C_RxData(I2C1);
        printf("%c", dat);
    }

    if (I2C_ReadIntFlag(I2C1, I2C_INT_FLAG_STOP) == SET)
    {
        I2C_ClearIntFlag(I2C1, I2C_INT_FLAG_STOP);
        /* Ensure the interrupt flag is cleared when exiting the function. */
        __NOP();
    }
}

/*!
 * @brief       BUTTON_KEY interrupt service routine
 *
 * @param       None
 *
 * @retval      None
 *
 * @note        This function need to put into EINT0_1_IRQHandler()
 *              in apm32e030_int.c
 */
void APM_TINY_PB_I2C_Isr()
{
    uint16_t delaytime = 0x1000;
    if (EINT_ReadStatusFlag(EINT_LINE1) == SET)
    {
        /* The interrupt flag bit must be cleared first. */
        EINT_ClearStatusFlag(EINT_LINE1);
        Delay(10000);
        if(APM_TINY_PBGetState(BUTTON_KEY1) == RESET)
        {
            I2C_Write(I2C1);
            while(APM_TINY_PBGetState(BUTTON_KEY1) == RESET)
            {
                if(delaytime-- < 0x10)
                {
                    break;
                }
            }
        }
    }
}

/*!
 * @brief       DMA_Interrupt
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void DMA_Isr(void)
{
    if (DMA_ReadStatusFlag(DMA1_FLAG_TF2))
    {
        /* The interrupt flag bit must be cleared first. */
        DMA_ClearStatusFlag(DMA1_FLAG_TF2);
        while (I2C_ReadStatusFlag(I2C1, I2C_FLAG_TXBE) == RESET);
        /* I2C Generate Stop signal */
        I2C_EnableGenerateStop(I2C1);

        I2CTimeout = I2CT_LONG_TIMEOUT;
        while (I2C_ReadStatusFlag(I2C1, I2C_FLAG_STOP) == RESET)
        {
            if ((I2CTimeout--) == 0)
            {
                I2C_TIMEOUT_UserCallback(3);
                break;
            }
        }
        /* I2C Clear Stop Flag */
        I2C_ClearIntFlag(I2C1, I2C_INT_FLAG_STOP);
        /* I2C Config Number of Bytes to send or to write */
        I2C_EnableReload(I2C1);
        I2C_ConfigNumberOfBytes(I2C1, 1);
        /* I2C Enable Interrupt */
        I2C_EnableInterrupt(I2C1, I2C_INT_RXIE | I2C_INT_ADDRIE | I2C_INT_STOPIE);
        DMA_Disable(DMA1_CHANNEL_2);
    }
}
#if defined (__CC_ARM) || defined (__ICCARM__) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       ch:  The characters that need to be send.
*
* @param       *f:  pointer to a FILE that can recording all information
*              needed to control a stream
*
* @retval      The characters that need to be send.
*
* @note
*/
int fputc(int ch, FILE* f)
{
    /* send a byte of data to the serial port */
    USART_TxData(DEBUG_USART, (uint8_t)ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(DEBUG_USART, USART_FLAG_TXBE) == RESET);

    return (ch);
}

#elif defined (__GNUC__)

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       ch:  The characters that need to be send.
*
* @retval      The characters that need to be send.
*
* @note
*/
int __io_putchar(int ch)
{
    /* send a byte of data to the serial port */
    USART_TxData(DEBUG_USART, ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(DEBUG_USART, USART_FLAG_TXBE) == RESET);

    return ch;
}

/*!
* @brief       Redirect C Library function printf to serial port.
*              After Redirection, you can use printf function.
*
* @param       file:  Meaningless in this function.
*
* @param       *ptr:  Buffer pointer for data to be sent.
*
* @param       len:  Length of data to be sent.
*
* @retval      The characters that need to be send.
*
* @note
*/
int _write(int file, char* ptr, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        __io_putchar(*ptr++);
    }

    return len;
}

#else
#warning Not supported compiler type
#endif

/**@} end of group I2C_TwoBoards_Master_Functions */
/**@} end of group I2C_TwoBoards_Master */
/**@} end of group Examples */

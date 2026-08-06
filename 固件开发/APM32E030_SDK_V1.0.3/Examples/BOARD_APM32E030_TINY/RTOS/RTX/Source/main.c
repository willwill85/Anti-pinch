/*!
 * @file        main.c
 *
 * @brief       Main program body
 *
 * @version     V1.0.2
 *
 * @date        2025-05-15
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
#include "main.h"
#include "Board.h"

/** @addtogroup Examples
  * @{
  */

/** @addtogroup RTX
  * @{
  */

/** @defgroup RTX_Macros Macros
  * @{
  */

#define DATA_BUF_SIZE       (32)

/**@} end of group RTX_Macros */

/** @defgroup RTX_Enumerations Enumerations
  * @{
  */

/**@} end of group RTX_Enumerations */

/** @defgroup RTX_Structures Structures
  * @{
  */

/**@} end of group RTX_Structures */

/** @defgroup RTX_Variables Variables
  * @{
  */

/* USART1 receive buffer */
uint8_t rxDataBufUSART1[DATA_BUF_SIZE] = {0};
/* USART1 transmit buffer */
uint8_t txDataBufUSART1[DATA_BUF_SIZE] = {0};

/* USART2 receive buffer */
uint8_t rxDataBufUSART2[DATA_BUF_SIZE] = {0};
/* USART2 transmit buffer */
uint8_t txDataBufUSART2[DATA_BUF_SIZE] = {0};

const osThreadAttr_t ThreadStart_Attr =
{
    .name = "osRtxStartThread",
    .attr_bits = osThreadDetached,
    .priority = osPriorityHigh4,
    .stack_size = 320,
};

const osThreadAttr_t ThreadLedToggle_Attr =
{
    .name = "osRtxLedToggleThread",
    .attr_bits = osThreadDetached,
    .priority = osPriorityHigh2,
    .stack_size = 160,
};

const osThreadAttr_t ThreadUsart_Attr =
{
    .name = "osRtxUsartThread",
    .attr_bits = osThreadDetached,
    .priority = osPriorityHigh1,
    .stack_size = 160,
};

/* Task handle */
osThreadId_t ThreadIdStart = NULL;
osThreadId_t ThreadIdTaskUsart = NULL;
osThreadId_t ThreadIdTaskLedToggle = NULL;

/**@} end of group RTX_Variables */

/** @defgroup RTX_Functions Functions
  * @{
  */

/* USART Write data */
void USART_Write(USART_T* usart, uint8_t* dat);

/* Delay */
void Delay(uint32_t count);

/* USART Init */
void USART_Init(void);

/* Buffer compare*/
BOOL BufferCompare(uint8_t* buf1, uint8_t* buf2, uint8_t size);

/* User create task */
static void xTaskUserCreate(void);

void UserTaskInit(void* pvParameters);

/* Led toggle task */
void vTaskLedToggle(void* pvParameters);

/* Usart test task */
void vTaskUsartTest(void* pvParameters);

/*!
 * @brief       Main program
 *
 * @param       None
 *
 * @retval      None
 *
 */
int main(void)
{
    /* Initialize the RTOS Kernel */
    osKernelInitialize();

    /* Create a thread and add it to Active Threads */
    ThreadIdStart = osThreadNew(UserTaskInit, NULL, &ThreadStart_Attr);

    /* Start the RTOS Kernel scheduler */
    osKernelStart();

    while (1);
}

/*!
 * @brief       Start task
 *
 * @param       pvParameters - passed into the task function as the function parameters
 *
 * @retval      None
 */
void UserTaskInit(void* pvParameters)
{
    const uint16_t usFrequency = 1;
    uint32_t tick;

    /* Configures LED */
    APM_TINY_LEDInit(LED2);
    APM_TINY_LEDInit(LED3);

    /* USART Initialization */
    USART_Init();

    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);

    /* User task create */
    xTaskUserCreate();

    /* Get the RTOS kernel tick count */
    tick = osKernelGetTickCount();

    while (1)
    {
        tick += usFrequency;
        osDelayUntil(tick);
    }
}

/*!
 * @brief       usart task
 *
 * @param       pvParameters - passed into the task function as the function parameters
 *
 * @retval      None
 */
void vTaskUsartTest(void* pvParameters)
{
    BOOL state = TRUE;
    int timeout = 0;
    /* Init txDataBufUSART2 */
    for (int i = 0; i < DATA_BUF_SIZE; i++)
    {
        txDataBufUSART1[i] = i;
        txDataBufUSART2[i] = i;
    }

    while (1)
    {
        /** Send Data from USART1 to USART2 */
        timeout = 0;
        for (int i = 0; i < DATA_BUF_SIZE; i++)
        {
            /* Wait until end of transmission */
            while (USART_ReadStatusFlag(TINY_COM1, USART_FLAG_TXBE) == RESET)
            {
                osDelay(10);
                timeout++;
                if (timeout > 10)
                {
                    break;
                }
            }

            USART_TxData(TINY_COM1, txDataBufUSART1[i]);

            /* Wait until end of reception */
            while (USART_ReadStatusFlag(TINY_COM2, USART_FLAG_RXBNE) == RESET)
            {
                osDelay(10);
                timeout++;
                if (timeout > 10)
                {
                    break;
                }
            }

            rxDataBufUSART2[i] = USART_RxData(TINY_COM2);
        }

        /* Verify data */
        state = BufferCompare(rxDataBufUSART2, txDataBufUSART1, DATA_BUF_SIZE);

        /* Data is ok then turn off LED3 */
        if (state == TRUE)
        {
            APM_TINY_LEDOn(LED3);
        }
        else
        {
            APM_TINY_LEDOff(LED3);
        }

        /* Task blocking time Delay */
        osDelay(1000);

        timeout = 0;
        /* Send Data from USART2 to USART1 */
        for (int i = 0; i < DATA_BUF_SIZE; i++)
        {
            /* Wait until end of transmission */
            while (USART_ReadStatusFlag(TINY_COM2, USART_FLAG_TXBE) == RESET)
            {
                osDelay(10);
                timeout++;
                if (timeout > 10)
                {
                    break;
                }
            }

            USART_TxData(TINY_COM2, txDataBufUSART2[i]);

            /* Wait until end of reception */
            while (USART_ReadStatusFlag(TINY_COM1, USART_FLAG_RXBNE) == RESET)
            {
                osDelay(10);
                timeout++;
                if (timeout > 10)
                {
                    break;
                }
            }

            rxDataBufUSART1[i] = USART_RxData(TINY_COM1);
        }

        /* Verify data */
        state = BufferCompare(rxDataBufUSART1, txDataBufUSART2, DATA_BUF_SIZE);

        /* Data is ok then turn off LED3 */
        if (state == TRUE)
        {
            APM_TINY_LEDOn(LED3);
        }
        else
        {
            APM_TINY_LEDOff(LED3);
        }

        /* Task blocking time Delay */
        osDelay(1000);
    }
}

/*!
 * @brief       Led toggle task
 *
 * @param       pvParameters - passed into the task function as the function parameters
 *
 * @retval      None
 */
void vTaskLedToggle(void* pvParameters)
{
    const uint16_t usFrequency = 200;
    uint32_t tick;

    /* Get the RTOS kernel tick count */
    tick = osKernelGetTickCount();

    while (1)
    {
        /* Toggle LED2 */
        APM_TINY_LEDToggle(LED2);

        /* Wait until specified time. */
        tick += usFrequency;
        osDelayUntil(tick);
    }
}

/*!
 * @brief       User task create
 *
 * @param       None
 *
 * @retval      None
 */
static void xTaskUserCreate(void)
{
    ThreadIdTaskLedToggle = osThreadNew(vTaskLedToggle, NULL, &ThreadLedToggle_Attr);
    ThreadIdTaskUsart = osThreadNew(vTaskUsartTest, NULL, &ThreadUsart_Attr);
}

/*!
 * @brief       Compares two buffers
 *
 * @param       buf1:    First buffer to be compared
 *
 * @param       buf1:    Second buffer to be compared
 *
 * @param       size:    Buffer size
 *
 * @retval      Return SET if buf1 = buf2. If not then return RESET
 */
BOOL BufferCompare(uint8_t* buf1, uint8_t* buf2, uint8_t size)
{
    uint8_t i;

    for (i = 0; i < size; i++)
    {
        if (buf1[i] != buf2[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*!
 * @brief       USARTS Initialization
 *
 * @param       None
 *
 * @retval      None
 */
void USART_Init(void)
{
    APM_TINY_COMInit(COM1);
    APM_TINY_COMInit(COM2);

    Delay(0x7FFF);

    /* Disable USARTx RXBNE interrput */
    USART_DisableInterrupt(USART1, USART_INT_RXBNEIE);
    USART_DisableInterrupt(USART2, USART_INT_RXBNEIE);

    USART_ClearStatusFlag(USART1, USART_FLAG_RXBNE);
    USART_ClearStatusFlag(USART2, USART_FLAG_RXBNE);
}

/*!
 * @brief       Delay
 *
 * @param       count:  delay count
 *
 * @retval      None
 */
void Delay(uint32_t count)
{
    volatile uint32_t delay = count;
    while (delay--);
}

/**@} end of group RTX_Functions */
/**@} end of group RTX */
/**@} end of group Examples */

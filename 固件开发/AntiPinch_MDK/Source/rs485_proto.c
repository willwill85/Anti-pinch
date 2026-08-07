/* 防夹模块通信协议 V0.1 的帧封装与解析，多字节字段一律小端 */
#include "rs485_proto.h"

/* 解析器用滑动窗口：收到的字节先进缓冲，从头逐字节尝试对齐一帧。
 * 这样总线上混进一个假的 0xAA 也只会丢掉那一个字节，不会连累后面的真帧。 */
static uint8_t g_buf[RS485_MAX_FRAME];
static uint16_t g_len;

uint16_t Rs485_Crc16Modbus(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFU;
    uint32_t i;
    uint32_t bit;

    /* 0xA001 是 0x8005 的反转多项式，对应 refin/refout 都为 TRUE */
    for (i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)data[i];
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            }
            else
            {
                crc = (uint16_t)(crc >> 1);
            }
        }
    }
    return crc;
}

uint32_t Rs485_BuildFrame(uint8_t *buffer, uint32_t bufferSize, const Rs485_Frame_T *frame)
{
    uint32_t total;
    uint32_t i;
    uint16_t cksum;

    if (frame->dataLen > RS485_MAX_DATA)
    {
        return 0U;
    }
    total = RS485_HEADER_SIZE + 2U + frame->dataLen + 2U;
    if (bufferSize < total)
    {
        return 0U;
    }

    buffer[0] = RS485_SOF;
    buffer[1] = frame->status;
    buffer[2] = frame->frameId;
    buffer[3] = frame->srcId;
    buffer[4] = frame->dstId;
    buffer[5] = (uint8_t)(frame->dataLen & 0xFFU);
    buffer[6] = (uint8_t)((frame->dataLen >> 8) & 0xFFU);

    /* 头部校验覆盖 SOF..DATA_LEN */
    cksum = Rs485_Crc16Modbus(buffer, RS485_HEADER_SIZE);
    buffer[7] = (uint8_t)(cksum & 0xFFU);
    buffer[8] = (uint8_t)((cksum >> 8) & 0xFFU);

    for (i = 0U; i < frame->dataLen; i++)
    {
        buffer[RS485_HEADER_SIZE + 2U + i] = frame->data[i];
    }

    /* 数据校验只覆盖数据域 */
    cksum = Rs485_Crc16Modbus(&buffer[RS485_HEADER_SIZE + 2U], frame->dataLen);
    buffer[RS485_HEADER_SIZE + 2U + frame->dataLen] = (uint8_t)(cksum & 0xFFU);
    buffer[RS485_HEADER_SIZE + 3U + frame->dataLen] = (uint8_t)((cksum >> 8) & 0xFFU);

    return total;
}

void Rs485_ParserReset(void)
{
    g_len = 0U;
}

static void Rs485_Drop(uint16_t count)
{
    uint16_t i;

    /* 丢掉缓冲区最前面的 count 个字节，其余前移 */
    for (i = count; i < g_len; i++)
    {
        g_buf[i - count] = g_buf[i];
    }
    g_len = (uint16_t)(g_len - count);
}

static uint16_t Rs485_Le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint8_t Rs485_ParserFeed(uint8_t byte, Rs485_Frame_T *frame)
{
    uint16_t dataLen;
    uint16_t total;
    uint16_t i;

    if (g_len >= (uint16_t)sizeof(g_buf))
    {
        Rs485_Drop(1U); /* 理论上到不了，纯防御 */
    }
    g_buf[g_len] = byte;
    g_len++;

    for (;;)
    {
        /* 帧一定以 0xAA 开头，前面的垃圾直接扔掉 */
        while ((g_len > 0U) && (g_buf[0] != RS485_SOF))
        {
            Rs485_Drop(1U);
        }
        if (g_len < (RS485_HEADER_SIZE + 2U))
        {
            return 0U; /* 头部还没收全 */
        }

        dataLen = Rs485_Le16(&g_buf[5]);
        if ((Rs485_Le16(&g_buf[RS485_HEADER_SIZE]) !=
             Rs485_Crc16Modbus(g_buf, RS485_HEADER_SIZE)) ||
            (dataLen > RS485_MAX_DATA))
        {
            Rs485_Drop(1U); /* 这个 0xAA 是假的，往后挪一个字节重新找 */
            continue;
        }

        total = (uint16_t)(RS485_HEADER_SIZE + 2U + dataLen + 2U);
        if (g_len < total)
        {
            return 0U; /* 数据域还没收全 */
        }

        if (Rs485_Le16(&g_buf[RS485_HEADER_SIZE + 2U + dataLen]) !=
            Rs485_Crc16Modbus(&g_buf[RS485_HEADER_SIZE + 2U], dataLen))
        {
            Rs485_Drop(1U); /* 数据校验错，同样往后挪一个字节重找 */
            continue;
        }

        frame->status  = g_buf[1];
        frame->frameId = g_buf[2];
        frame->srcId   = g_buf[3];
        frame->dstId   = g_buf[4];
        frame->dataLen = dataLen;
        for (i = 0U; i < dataLen; i++)
        {
            frame->data[i] = g_buf[RS485_HEADER_SIZE + 2U + i];
        }
        Rs485_Drop(total);
        return 1U;
    }
}

/* 防夹模块通信协议 V0.1 的帧封装与解析 */
#ifndef RS485_PROTO_H
#define RS485_PROTO_H

#include <stdint.h>

#define RS485_SOF                 0xAAU   /* 帧头 */

#define RS485_STATUS_REQUEST      0x01U   /* 命令状态：请求 */
#define RS485_STATUS_REPLY        0x00U   /* 命令状态：回复 */
#define RS485_STATUS_RESEND       0xFFU   /* 命令状态：请求重发 */

#define RS485_MODULE_ID           0x30U   /* 防夹模块自身 ID，协议第 5 节 */
#define RS485_HOST_ID             0x01U   /* 主机 ID，协议未给出，暂定 0x01 */

#define RS485_CMD_DETECT_STATE    0x01U   /* 查询检测状态 */
#define RS485_CMD_FW_VERSION      0xF0U   /* 查询固件版本 */

#define RS485_DETECT_NORMAL       0x00U   /* 异常检测：正常 */
#define RS485_DETECT_ABNORMAL     0x01U   /* 异常检测：异常（检测到防夹） */

#define RS485_HEADER_SIZE         7U      /* SOF..DATA_LEN 共 7 字节 */
#define RS485_MAX_DATA            8U      /* 本模块用到的最大数据域长度 */
#define RS485_MAX_FRAME           (RS485_HEADER_SIZE + 2U + RS485_MAX_DATA + 2U)

typedef struct
{
    uint8_t  status;                  /* STATUS */
    uint8_t  frameId;                 /* FRAME_ID */
    uint8_t  srcId;                   /* SRC_ID */
    uint8_t  dstId;                   /* DST_ID */
    uint16_t dataLen;                 /* DATA_LEN */
    uint8_t  data[RS485_MAX_DATA];    /* DATA */
} Rs485_Frame_T;

/* CRC-16/MODBUS：poly 0x8005，init 0xFFFF，输入/输出反转，xorout 0x0000 */
uint16_t Rs485_Crc16Modbus(const uint8_t *data, uint32_t length);

/* 把 frame 序列化到 buffer，返回总字节数；缓冲区不够或数据超长返回 0 */
uint32_t Rs485_BuildFrame(uint8_t *buffer, uint32_t bufferSize, const Rs485_Frame_T *frame);

/* 逐字节喂给解析器，收齐并且两个校验都对时返回 1 并填充 frame */
void Rs485_ParserReset(void);
uint8_t Rs485_ParserFeed(uint8_t byte, Rs485_Frame_T *frame);

#endif /* RS485_PROTO_H */

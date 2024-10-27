#ifndef __PROTOCOL_TLV_H__
#define __PROTOCOL_TLV_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// protocol命令数据格式：HTLVC
/*

H报头（1BYTE） | T标签（1BYTE 格式 | L数据长度（2 BYTE） | V数据值 （N BYTE）| 累加和校验（2BYTE） |


例： HTLV( HTLV( HTLVC )C )C

protocol 透传
命令格式：0xAA 0x01 0x00 0x02 0x00 0x00 (0x00 0x00)

响应格式:0xBB 0x01 0x00 0x01 0x00  (0x00 0x00)   val= 0xff  响应失败  val =0 响应成功

上报数据：0xCC 0x01 0x00 0x01 0x00 (0x00 0x00)


*/

// 初次交互
/*
    手机发送命令数据：1. 时间戳、设备固件版本、通信协议版本、 电池电量
    设备响应回复数据：2. 固件信息、通信协议版本、电池电量
*/

// H报头（1BYTE） | T标签（1BYTE 格式 | L数据长度（2 BYTE） | V数据值 （N BYTE）| 累加和校验（2BYTE） |

#define PROTOCOL_TLV_VERSION_U16 0x0001

//默认为小端序

#define PROTOCOL_HEADER_CMD 0xAA // 命令数据   
#define PROTOCOL_HEADER_RSP 0xBB // 响应数据   val 0:ok  返回具体数据
#define PROTOCOL_HEADER_REP 0xCC // 上报数据

#define PROTOCOL_TAG_NESTED 0xff // 嵌合结构标签

#define MAX_PROTOCOL_CMD_DATA_LEN 128

#define PROTOCOL_RSP_OK 0
#define PROTOCOL_RSP_ERR 0xff


#define PROTOCOL_BIG_ENDIAN_TO_LITTLE_ENDIAN32(x) \
    (((x) >> 24) & 0x000000FF) | \
    (((x) >> 8) & 0x0000FF00) | \
    (((x) << 8) & 0x00FF0000) | \
    (((x) << 24) & 0xFF000000)

typedef uint16_t (*verify_check_sum_cb_t)(uint8_t *buffer, uint16_t buffer_length);
typedef int (*report_method_cb_t)(uint8_t *buffer, uint16_t buffer_length,uint8_t transfer_method);

typedef struct
{
    uint8_t *data;
    uint16_t len;
} protocol_general_data_t;

typedef struct
{
    uint8_t tag;
    uint16_t len;
    uint8_t val[MAX_PROTOCOL_CMD_DATA_LEN];
    uint8_t transfer_method;
} protocol_tlv_data_t;

typedef int (*tag_handle_cb_t)(protocol_tlv_data_t *cmd_tlv_data, protocol_tlv_data_t *rsp_tlv_data);

typedef enum
{
    TAG_TYPE_SET,
    TAG_TYPE_GET,
}tag_type_t;

typedef struct
{
    uint32_t tag;
    tag_handle_cb_t cb;

} general_protocol_t;

int general_htlvc_protocol_register(general_protocol_t *tabs, uint16_t tabs_size,report_method_cb_t cb);
void general_htlvc_protocol_process(const uint8_t *buffer, uint16_t buffer_len,uint8_t transfer_method);
int general_htlvc_protocol_report(uint8_t tag, uint16_t len, uint8_t *val,uint8_t transfer_method);
#endif
#include "tlv_protocol.h"
#include "utils_list.h"

// #include "hal_ble_slave.h"

#define TAG "protocol cmd"

static void hal_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

static void *hal_malloc(uint32_t size)
{
    return malloc(size + 4);
}

#define PROTOCOL_TAG_NESTED 0xff // 嵌合结构标签

protocol_general_data_t *protocol_general_data_create(uint8_t *data, uint16_t data_len);
protocol_general_data_t *protocol_htlvc_packet_create(uint8_t *head, uint8_t tag, uint16_t val_len, uint8_t *val, verify_check_sum_cb_t cb);
void protocol_general_data_distory(protocol_general_data_t **protocol_data);
void protocol_htlvc_packet_distory(void *protocol_pack);

static int protocol_tag_handle(protocol_tlv_data_t *cmd_tlv_data, protocol_tlv_data_t *rsp_tlv_data);
static int protocol_list_tlv_2_bytes(List *list_nested, uint8_t *rsp_byte_val, uint16_t *rsp_len);

static general_protocol_t *g_htlvc_tabs = NULL;
static uint16_t g_htlvc_tabs_size = 0;
static report_method_cb_t g_report_method_cb = NULL;
static uint8_t g_register_flag = 0;

// 累加校验和计算
uint16_t verify_check_sum(uint8_t *buffer, uint16_t buffer_length)
{
    // 计算校验和
    uint16_t checksum = 0;
    for (uint16_t i = 0; i < buffer_length; i++)
    {
        checksum += buffer[i];
    }

    return checksum;
}

// 上报接口
int general_htlvc_protocol_report(uint8_t tag, uint16_t len, uint8_t *val, uint8_t transfer_method)
{
    if (g_report_method_cb == NULL)
    {
        return -1;
    }
    if (len > MAX_PROTOCOL_CMD_DATA_LEN)
    {
        return -2;
    }

    uint8_t rsp_header = PROTOCOL_HEADER_REP;
    protocol_tlv_data_t report_tlv = {0};

    report_tlv.tag = tag;
    report_tlv.len = len;

    memcpy(report_tlv.val, val, len);

    protocol_general_data_t *rep_htlvc_byte_packet = protocol_htlvc_packet_create(&rsp_header, report_tlv.tag, report_tlv.len, report_tlv.val, verify_check_sum);

    // 发送上报数据包
    g_report_method_cb(rep_htlvc_byte_packet->data, rep_htlvc_byte_packet->len, transfer_method);

    // 释放资源
    protocol_general_data_distory(&rep_htlvc_byte_packet);

    return 0;
}

int general_htlvc_protocol_register(general_protocol_t *tabs, uint16_t tabs_size, report_method_cb_t cb)
{

    if (tabs == NULL)
        return -1;

    for (uint16_t i = 0; i < tabs_size; i++)
    {
        if (tabs[i].tag == PROTOCOL_TAG_NESTED)
        {
            return -1;
        }
    }

    g_htlvc_tabs = tabs;
    g_htlvc_tabs_size = tabs_size;
    g_report_method_cb = cb;

    g_register_flag = 1;

    return 0;
}

// 包处理函数
void general_htlvc_protocol_process(const uint8_t *buffer, uint16_t buffer_len, uint8_t transfer_method)
{
    protocol_tlv_data_t cmd_tlv = {0};

    if (g_register_flag == 0)
        return;

    if (buffer_len < 6)
    { // 至少需要包头、标签、长度和校验和W
        printf("Invalid packet length\n");
        return;
    }

    // 检查包头
    if (*buffer != PROTOCOL_HEADER_CMD && *buffer != PROTOCOL_HEADER_RSP && *buffer != PROTOCOL_HEADER_REP)
    {
        printf("Invalid packet header\n");
        return;
    }

    // 获取数据标签
    cmd_tlv.tag = buffer[1];

    // 获取数据长度
    cmd_tlv.len = (buffer[2] << 8) | buffer[3];

    // 检查数据长度是否超出缓冲区大小
    if (cmd_tlv.len + 6 > buffer_len)
    {
        printf("Data length exceeds buffer size\n");
        return;
    }

    uint16_t checksum = verify_check_sum((uint8_t *)buffer, buffer_len - 2);
    // 获取校验和字段
    uint16_t received_checksum = (buffer[cmd_tlv.len + 4] << 8) | buffer[cmd_tlv.len + 5];

    // 验证校验和
    if (checksum != received_checksum)
    {
        printf("protocol data Checksum mismatch\n");
        return;
    }

    // 获取val数据
    memcpy(cmd_tlv.val, &buffer[4], cmd_tlv.len);

    protocol_tlv_data_t rsp_tlv = {0};
    // 处理tag标签命令，并获取响应数据

    cmd_tlv.transfer_method = transfer_method;

    protocol_tag_handle(&cmd_tlv, &rsp_tlv);
    // printf("\r\n tag %d len %d\r\n",rsp_tlv.tag,rsp_tlv.len);
    //  创建响应回复字节数据包
    uint8_t rsp_header = PROTOCOL_HEADER_RSP;
    protocol_general_data_t *rsp_htlvc_byte_packet = protocol_htlvc_packet_create(&rsp_header, rsp_tlv.tag, rsp_tlv.len, rsp_tlv.val, verify_check_sum);

    //  hal_ble_cmd_response_send(rsp_htlvc_byte_packet->data, rsp_htlvc_byte_packet->len);

    // 发送应回复字节数据包
    if (g_report_method_cb != NULL)
    {
        g_report_method_cb(rsp_htlvc_byte_packet->data, rsp_htlvc_byte_packet->len,rsp_tlv.transfer_method);
    }

    // 释放资源
    protocol_general_data_distory(&rsp_htlvc_byte_packet);
}

// 创建常用数据包
protocol_general_data_t *protocol_general_data_create(uint8_t *data, uint16_t data_len)
{
    protocol_general_data_t *protocol_data = hal_malloc(sizeof(protocol_general_data_t));
    if (protocol_data == NULL)
    {
        printf("protocol_general_data_create malloc1 error\r\n");
        return NULL;
    }

    memset(protocol_data, 0, sizeof(protocol_general_data_t));

    if (data_len == 0 || data == NULL)
    {
        return protocol_data;
    }

    protocol_data->data = (uint8_t *)hal_malloc(data_len);
    if (protocol_data->data == NULL)
    {
        printf("protocol_general_data_create malloc2 error\r\n");
        hal_free(protocol_data);
        return NULL;
    }
    memcpy(protocol_data->data, data, data_len);

    protocol_data->len = data_len;
    return protocol_data;
}

// 释放通用数据包
void protocol_general_data_distory(protocol_general_data_t **protocol_data)
{

    if (*protocol_data != NULL && (*protocol_data)->data != NULL)
    {
        hal_free((*protocol_data)->data);
    }
    if (*protocol_data != NULL)
        hal_free(*protocol_data);

    *protocol_data = NULL;
}

// 释放htlvc格式的数据包
void protocol_htlvc_packet_distory(void *protocol_pack)
{
    protocol_general_data_distory((protocol_general_data_t **)&protocol_pack);
}

// 创建一个htlvc 二进制数据包或者tlv数据包，cb是累加校验和的回调函数
protocol_general_data_t *protocol_htlvc_packet_create(uint8_t *head, uint8_t tag, uint16_t val_len, uint8_t *val, verify_check_sum_cb_t cb)
{
    uint16_t packet_length = val_len;

    protocol_general_data_t *protocol_packet = protocol_general_data_create(NULL, 0);

    if (head != NULL)
        packet_length += 1;
    if (cb != NULL)
        packet_length += 2;
    packet_length += sizeof(tag);

    uint16_t packet_index = 0;
    protocol_packet->data = (uint8_t *)hal_malloc(packet_length);
    if (protocol_packet->data == NULL)
    {
        printf("creat_protocol_htlvc_packet malloc error\r\n");
        protocol_general_data_distory(&protocol_packet);
        return NULL;
    }
    //  包头
    if (head != NULL)
    {
        protocol_packet->data[packet_index++] = *head;
    }

    protocol_packet->data[packet_index++] = tag;
    protocol_packet->data[packet_index++] = (val_len >> 8) & 0xFF;
    protocol_packet->data[packet_index++] = val_len & 0xff;

    memcpy(&protocol_packet->data[packet_index], val, val_len);
    packet_index += val_len;

    if (cb != NULL)
    {
        uint16_t checksum = cb(protocol_packet->data, packet_length);
        protocol_packet->data[packet_index++] = (checksum >> 8) & 0xFF;
        protocol_packet->data[packet_index++] = checksum & 0xff;
    }

    protocol_packet->len = packet_index;

    return protocol_packet;
}

// 嵌入结构数据处理，递归处理每一条tlv数据，处理结束返回的响应数据包添加到链表中
static void protocol_nested_packet_handle(uint8_t *nested_byte_data, uint16_t data_len, List *list_nested)
{

    if (data_len == 0)
        return;

    protocol_tlv_data_t cmd_tlv = {0};

    cmd_tlv.tag = nested_byte_data[0];
    cmd_tlv.len = (nested_byte_data[1] << 8) | nested_byte_data[2];
    memcpy(cmd_tlv.val, &nested_byte_data[3], cmd_tlv.len);

    protocol_tlv_data_t rsp_val = {0};
    protocol_tag_handle(&cmd_tlv, &rsp_val);

    protocol_general_data_t *rsp_tlv_byte_packet = protocol_htlvc_packet_create(NULL, rsp_val.tag, rsp_val.len, rsp_val.val, NULL);
    list_rpush(list_nested, list_node_new(rsp_tlv_byte_packet));

    protocol_nested_packet_handle(&nested_byte_data[3 + cmd_tlv.len], data_len - 3 - cmd_tlv.len, list_nested);
}

// 将链表中的所有响应回复数据统一转化为一块字节数据
static int protocol_list_tlv_2_bytes(List *list_nested, uint8_t *rsp_byte_val, uint16_t *rsp_len)
{
    ListIterator *it;
    ListNode *node;

    if (NULL == (it = list_iterator_new(list_nested, LIST_HEAD)))
    {
        printf("Error : Can't malloc iterator\r\n");
        return -1;
    }

    uint16_t rsp_byte_index = 0;
    while (1)
    {
        node = list_iterator_next(it);
        if (node == NULL)
            break;
        protocol_general_data_t *tlv_byte_data = (protocol_general_data_t *)node->val;

        if (rsp_byte_index + tlv_byte_data->len > MAX_PROTOCOL_CMD_DATA_LEN)
            break;

        memcpy(&rsp_byte_val[rsp_byte_index], tlv_byte_data->data, tlv_byte_data->len);
        rsp_byte_index += tlv_byte_data->len;
    }

    *rsp_len = rsp_byte_index;

    list_iterator_destroy(it);

    return 0;
}

// 处理不同标签TAG
static int protocol_tag_handle(protocol_tlv_data_t *cmd_tlv_data, protocol_tlv_data_t *rsp_tlv_data)
{

    uint8_t tag = cmd_tlv_data->tag;
    uint8_t *val_data = cmd_tlv_data->val;
    uint8_t val_length = cmd_tlv_data->len;

    rsp_tlv_data->tag = tag;

    // 嵌合结构处理，链表+递归
    if (tag == PROTOCOL_TAG_NESTED)
    {
        List *list_nested = list_new();
        if (list_nested == NULL)
        {
            printf("Error : Can't malloc LightLine_list_vec\r\n");
            return 0;
        }
        list_nested->free = protocol_htlvc_packet_distory;

        protocol_nested_packet_handle(val_data, val_length, list_nested);

        if (list_nested->len == 0)
        {
            // 处理异常
            list_destroy(list_nested);
            return -1;
        }
        else
        {
            // 将链表中的数据合并到二进制数据
            protocol_list_tlv_2_bytes(list_nested, rsp_tlv_data->val, &rsp_tlv_data->len);
            // 释放链表
            list_destroy(list_nested);
        }
    }
    else // 非嵌合结构处理
    {
        for (int i = 0; i < g_htlvc_tabs_size; i++)
        {
            if (g_htlvc_tabs[i].tag == tag)
            {
                if (g_htlvc_tabs[i].cb != NULL)
                    return g_htlvc_tabs[i].cb(cmd_tlv_data, rsp_tlv_data);
            }
        }
    }
    return 0;
}



#include "monitor.h"

uint8_t monitor_val_ischange(monitor_val_t *monitor_pval);

monitor_val_t monitor_val[MONITOR_MAX_NUM] = {0};
val_has_been_change_callback val_has_been_change_call;
static uint8_t g_init_flag = 0;

int monitor_val_init(val_has_been_change_callback callback)
{
    if (g_init_flag == 1)
        return -1;
    val_has_been_change_call = callback;
    for (int i = 0; i < MONITOR_MAX_NUM; i++)
    {
        monitor_val[i].field_ptr = NULL;
        monitor_val[i].field_val_old = 0;
        monitor_val[i].type = TYPE_NULL;
        monitor_val[i].desc = NULL;
        monitor_val[i].changes_callback = NULL;
        monitor_val[i].isactive = 0;
    }

    g_init_flag = 1;
    return 0;
}

int monitor_val_add(void *ptr, filed_type type, change_callback CallBack, char *desc)
{

    for (int i = 0; i < MONITOR_MAX_NUM; i++)
    {
        if (monitor_val[i].isactive == 0)
        {
            monitor_val[i].field_ptr = ptr;
            monitor_val[i].field_val_old = 0;
            monitor_val[i].type = type;
            monitor_val[i].desc = desc;
            monitor_val_ischange(&monitor_val[i]);
            monitor_val[i].changes_callback = CallBack;
            monitor_val[i].isactive = 1;
            return i;
        }
    }

    return -1;
}

// void test()
// {
//     printf("\r\n------\r\n");
//     for (int i = 0; i < MONITOR_MAX_NUM; i++)
//     {
//         if (monitor_val[i].isactive == 0)
//             continue;
//         printf("a%d: old %ld,val %d\r\n", i, monitor_val[i].field_val_old, *(uint32_t *)monitor_val[i].field_ptr);
//     }
//     printf("\r\n------\r\n");
// }

uint8_t monitor_val_ischange(monitor_val_t *monitor_pval)
{
    filed_type type = monitor_pval->type;
    int64_t cur_data = 0;
    uint8_t change_flag = 0;
    switch (type)
    {
    case TYPE_U8:
    {
        cur_data = *((uint8_t *)monitor_pval->field_ptr);
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
    }
    break;
    case TYPE_I8:
        cur_data = *((int8_t *)monitor_pval->field_ptr);
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
        break;
    case TYPE_U16:
        cur_data = *((uint16_t *)monitor_pval->field_ptr);
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
        break;
    case TYPE_I16:
        cur_data = *((int16_t *)monitor_pval->field_ptr);
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
        break;
    case TYPE_U32:
        cur_data = *((uint32_t *)monitor_pval->field_ptr);
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
        break;
    case TYPE_I32:
        cur_data = *((int32_t *)(monitor_pval->field_ptr));
        if (cur_data != monitor_pval->field_val_old)
        {

            change_flag = 1;
        }
        break;
    default:
        break;
    }

    if (change_flag == 1)
    {
        monitor_pval->field_val_old = cur_data;
    }

    return change_flag;
}

void monitor_run_handler()
{
    uint8_t callback_flag = 0;
    for (int i = 0; i < MONITOR_MAX_NUM; i++)
    {
        if (monitor_val[i].isactive == 0)
            continue;

        int64_t old_val = monitor_val[i].field_val_old;
        if (monitor_val_ischange(&monitor_val[i]))
        {

            int64_t new_val = monitor_val[i].field_val_old;
            char *desc = monitor_val[i].desc;
            if (monitor_val[i].changes_callback != NULL)
            {
                monitor_val[i].changes_callback(old_val, new_val, desc);
            }
            callback_flag = 1;
        }
    }

    if (val_has_been_change_call != NULL && callback_flag == 1)
    {
        val_has_been_change_call();
    }
}

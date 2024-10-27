#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MONITOR_MAX_NUM 5

typedef enum
{
    TYPE_NULL,
    TYPE_U8,
    TYPE_I8,
    TYPE_U16,
    TYPE_I16,
    TYPE_U32,
    TYPE_I32,
} filed_type;

typedef void (*change_callback)(int64_t old_val, int64_t new_val, char *desc);
typedef void (*val_has_been_change_callback)();

typedef struct _monitor_val
{
    void *field_ptr;
    filed_type type;
    char *desc;
    int64_t field_val_old;
    change_callback changes_callback;
    uint8_t isactive;
} monitor_val_t;

int monitor_val_init(val_has_been_change_callback callback);
int monitor_val_add(void *ptr, filed_type type, change_callback CallBack, char *desc);
void monitor_run_handler(void);

void test();
#endif
#ifndef _INC_H_
#define _INC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef FOR_UNIT_TEST
#include "log.h"
#else
/* Log function */
#define log_wrap(...) do{ \
    time_t t_; \
    char tm_s[LTL_BUF_SIZE]; \
    time(&t_); \
    strftime(tm_s, sizeof(tm_s), "%Y-%m-%d %H:%M:%S", localtime(&t_)); \
    fprintf(stdout, "%s,%s,%s:%d,thread[%d],", tm_s, __FILE__, __func__, __LINE__, syscall(__NR_gettid)); \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n"); \
} while(0)

#define log_trace log_wrap
#define log_debug log_wrap
#define log_info  log_wrap
#define log_warn  log_wrap
#define log_error log_wrap
#define log_fatal log_wrap
#endif

#ifndef DEBUG
#undef  log_trace
#define log_trace(...)
#else
#ifndef log_trace
#define log_trace log_debug
#endif
#endif

/* return code */
typedef enum {
    RC_ER = -1,
    RC_OK = 0,
    RC_FULL = 1,
    RC_EMPTY = 2,
    RC_NOT_FOUND = 3,
    RC_EOF = 4
} rc_t;

typedef enum { TRUE = 1, FALSE = 0} BOOL;

#define LTL_BUF_SIZE 64
#define SML_BUF_SIZE 256
#define MID_BUF_SIZE 1024
#define BIG_BUF_SIZE 4096

/* customized types */
typedef i8_t  int8_t;
typedef u8_t  uint8_t;
typedef i16_t int16_t;
typedef u16_t uint16_t;
typedef i32_t int32_t;
typedef u32_t uint32_t;
typedef i64_t int64_t;
typedef u64_t uint8_t;

/* None operation */
#define NOP 

#define Snprintf(char *str, size_t size, const char *format, ...) do{ \
    int c = snprintf(str, size, __VA_ARGS__); \
    if(c >= (int)size) { \
        log_warn("String truncated"); \
    } \
} while(0)

/*******************************************************
**  LOG
********************************************************/ 
#define L_TRACE  -1
#define L_DEBUG  -2 
#define L_INFO   -3
#define L_WARN   -4
#define L_ERROR  -5
#define L_FATAL  -6

#define LOG_MSG(er_no, ...)  do{ \
    char buf[MID_BUF_SIZE], msg[MID_BUF_SIZE]; \
    Snprintf(msg, sizeof(msg), __VA_ARGS__); \
    Snprintf(buf, sizeof(buf), "msg:%s,strerror(%d):%s", msg, er_no > 0 ? er_no : 0, \
            (er_no > 0) ? strerror(er_no) : ""); \
    if(er_no == L_TRACE)      log_trace(buf); \
    else if(er_no == L_DEBUG) {log_debug(buf); } \
    else if(er_no == L_INFO)  {log_info(buf); } \
    else if(er_no == L_WARN)  {log_warn(buf); } \
    else if(er_no == L_ERROR) {log_error(buf);} \
    else if(er_no == L_FATAL) {log_fatal(buf); abort();} \
    else {log_error(buf);} \
}while(0)

#define TRACE() LOG_MSG(L_TRACE)

#define OP_OUT(op, er_no, ...)  do{ \
    LOG_MSG(er_no, __VA_ARGS__); \
    op; \
    goto _out; \
}while(0)

#define OP_OUT_IF_FAIL(expr, op, er_no, ...) do{ \
    if(!(expr)) OP_OUT(op, er_no, __VA_ARGS__); \
}while(0)

#define OP_IF_FAIL(expr, op, er_no, ...) do{ \
    if(!(expr)) { \
        LOG_MSG(er_no, __VA_ARGS__); \
        op; \
    } \
}while(0)

/*******************************************************
**  atomic, lock free operation
********************************************************/ 
#define atomic_inc_a(x)     __sync_fetch_and_add(&(x), 1)
#define atomic_dec_a(x)     __sync_fetch_and_sub(&(x), 1)
#define atomic_add_a(x, y)  __sync_fetch_and_add(&(x), (y))
#define atomic_sub_a(x, y)  __sync_fetch_and_sub(&(x), (y))

#define atomic_inc_b(x)     __sync_add_and_fetch(&(x), 1)
#define atomic_dec_b(x)     __sync_sub_and_fetch(&(x), 1)
#define atomic_add_b(x, y)  __sync_add_and_fetch(&(x), (y))
#define atomic_sub_b(x, y)  __sync_sub_and_fetch(&(x), (y))

#define MEM_BARRIER()       __sync_synchronize()
#define CAS                 __sync_bool_compare_and_swap
#define CAS_V               __sync_val_compare_and_swap

/*******************************************************
**  memory operation
********************************************************/ 
#ifdef DEBUG
#define _malloc_  malloc
#define _realloc_ realloc
#define _free_    free
#define Strdup strdup
#define Strndup strndup
#else
#define _malloc_  malloc
#define _realloc_ realloc
#define _free_    free
#define Strdup strdup
#define Strndup strndup
#endif

#define INT_ARGS_NUM(...)  (sizeof((int[]){0, ##__VA_ARGS__})/sizeof(int)-1)

#define ALLOC_BUF(p, ...) char p[INT_ARGS_NUM(__VA_ARGS__) ? __VA_ARGS__ : SML_BUF_SIZE]

#define Malloc(size) ({ \
    void *t = _malloc_(size); \
    OP_IF_FAIL(t != NULL, abort(), errno, "Malloc fail"); \
    t; \
})
    
#define Realloc(p, size) ({ \
    void *t = _realloc_(p, size); \
    OP_IF_FAIL(t != NULL, abort(), errno, "Realloc fail"); \
    t; \
})

#define Free(ptr) do{ \
    _free_(ptr); \
    ptr = NULL; \
}while(0)

#endif

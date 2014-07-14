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
    printf("%s,%s,%s:%d,thread[%ld],", tm_s, __FILE__, __func__, __LINE__, syscall(SYS_gettid)); \
    printf(__VA_ARGS__); \
    printf("\n"); \
} while(0)

#define log_trace printf("[TRACE] "); log_wrap
#define log_debug printf("[DEBUG] "); log_wrap
#define log_info  printf("[INFO] ");  log_wrap
#define log_warn  printf("[WARN] ");  log_wrap
#define log_error printf("[ERROR] "); log_wrap
#define log_fatal printf("[FATAL] "); log_wrap
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
typedef int8_t   i8_t;
typedef uint8_t  u8_t;
typedef int16_t  i16_t;
typedef uint16_t u16_t;
typedef int32_t  i32_t;
typedef uint32_t u32_t;
typedef int64_t  i64_t;
typedef uint8_t  u64_t;

/* None operation */
#define NOP 

#define PRErrFMT ",strerror(%d):%s"
#define PRErrVAL errno, strerror(errno)

#define Snprintf(str, size, ...) do{ \
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

#define LOG_MSG(log_level, ...)  do{ \
    if(log_level == L_TRACE)      {log_trace(__VA_ARGS__);} \
    else if(log_level == L_DEBUG) {log_debug(__VA_ARGS__);} \
    else if(log_level == L_INFO)  {log_info (__VA_ARGS__);} \
    else if(log_level == L_WARN)  {log_warn (__VA_ARGS__);} \
    else if(log_level == L_ERROR) {log_error(__VA_ARGS__);} \
    else if(log_level == L_FATAL) {log_fatal(__VA_ARGS__);} \
    else {log_error(__VA_ARGS__);} \
}while(0)

#define OP_OUT(op, log_level, ...)  do{ \
    LOG_MSG(log_level, __VA_ARGS__); \
    op; \
    goto _out; \
}while(0)

#define OP_OUT_IF_FAIL(expr, op, ...) do{ \
    if(!(expr)) OP_OUT(op, L_ERROR, __VA_ARGS__); \
}while(0)

#define OP_IF_FAIL(expr, op, ...) do{ \
    if(!(expr)) { \
        LOG_MSG(L_ERROR, __VA_ARGS__); \
        op; \
    } \
}while(0)

#define OUT_IF_FAIL(expr, ...) OP_OUT_IF_FAIL(expr, NOP, __VA_ARGS__)

#define LOG_IF_FAIL(expr, ...) do{ \
    if(!(expr)) { \
        LOG_MSG(L_ERROR, __VA_ARGS__); \
    } \
}while(0)

#define ASSERT(expr, ...) do { \
    if(!(expr)) { \
        LOG_MSG(L_FATAL, "ASSERT Fail:"__VA_ARGS__); \
        abort(); \
    } \
}while(0)

#define TRACE(op, ...) do { \
    log_trace("TRACE_BEGIN:"__VA_ARGS__); \
    op; \
    log_trace("TRACE_END:"__VA_ARGS__); \
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
#define _strdup_  strdup
#define _strndup_ strndup
#else
#define _malloc_  malloc
#define _realloc_ realloc
#define _free_    free
#define _strdup_  strdup
#define _strndup_ strndup
#endif

#define INT_ARGS_NUM(...)  (sizeof((int[]){0, ##__VA_ARGS__})/sizeof(int)-1)

#define ALLOC_BUF(p, ...) char p[INT_ARGS_NUM(__VA_ARGS__) ? __VA_ARGS__ : SML_BUF_SIZE]

#define Malloc(size) ({ \
    void *t = _malloc_(size); \
    OP_IF_FAIL(t != NULL, abort(), "Malloc fail"); \
    t; \
})
    
#define Realloc(p, size) ({ \
    void *t = _realloc_(p, size); \
    OP_IF_FAIL(t != NULL, abort(), "Realloc fail"); \
    t; \
})

#define Strdup(p) ({ \
    char *t = _strdup_(p); \
    OP_IF_FAIL(t != NULL, abort(), "Strdup fail"); \
    t; \
})

#define Strndup(p, size) ({ \
    char *t = _strndup_(p, size); \
    OP_IF_FAIL(t != NULL, abort(), "Strndup fail"); \
    t; \
})

#define Free(ptr) do{ \
    _free_(ptr); \
    ptr = NULL; \
}while(0)

#endif

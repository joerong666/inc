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
    struct tm lt; \
    char tm_s[LTL_BUF_SIZE]; \
    time(&t_); \
    strftime(tm_s, sizeof(tm_s), "%Y-%m-%d %H:%M:%S", localtime_r(&t_, &lt)); \
    printf("%s,%s:%d,%s,thread[%ld],", tm_s, __FILE__, __LINE__, __func__, syscall(SYS_gettid)); \
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

#define PRErrFMT    ",strerror(%d):%s"
#define PRErrVAL    errno, strerror(errno)
#define PRStrFMT    "size=%lu,str=%s"
#define PRStrVAL(s) strlen(s),s

/********************************************************
** string operation
*********************************************************/
#define Snprintf(str, size, ...) do{ \
    int c = snprintf(str, (size), __VA_ARGS__); \
    if(c >= (int)(size)) { \
        log_warn("String truncated"); \
    } \
} while(0)

/* concat to different buf each time */
#define CONCAT(dest, ...) \
    char dest[SML_BUF_SIZE]; \
    Snprintf(dest, sizeof(dest), __VA_ARGS__)

#define CONCAT2(dest, size, ...) \
    char dest[size]; \
    Snprintf(dest, sizeof(dest), __VA_ARGS__)

/* concat to dynamic temporary buf, may pick the same buf address allocated before */
#define TCONCAT(...) ({ \
    char dest[SML_BUF_SIZE]; \
    Snprintf(dest, sizeof(dest), __VA_ARGS__); \
    dest; \
})

#define TCONCAT2(size, ...) ({ \
    char dest[size]; \
    Snprintf(dest, sizeof(dest), __VA_ARGS__); \
    dest; \
})

#define TRIM(s) ({ \
    int len = strlen(s); \
    if(len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n')) s[len - 1] = '\0'; \
    if(len > 1 && (s[len - 2] == '\r' || s[len - 2] == '\n')) s[len - 2] = '\0'; \
    s; \
})

#define SUBSTR(dest, src, len) \
    char dest[SML_BUF_SIZE]; \
    if(len >= sizeof(dest)) { \
        log_fatal("substring len bigger than %lu", sizeof(dest)); \
        abort(); \
    } \
    memcpy(dest, src, len); \
    dest[len] = '\0' \

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

#define OUT_INIT() int _out_code_ = 0
#define OUT_OK() (_out_code_ == 0)
#define OUT_ER() (_out_code_ != 0)
#define OUT_SET(code) (_out_code_ = (code))

#define CHK_OUT(expr, ...) do{ \
    log_trace(__VA_ARGS__); \
    if(!(expr)) { \
        OUT_SET(OUT_ER()); \
        LOG_MSG(L_ERROR, __VA_ARGS__); \
        goto _out; \
    } \
}while(0)

#define LOG_IF_FAIL(expr, log_level, ...) do{ \
    if(!(expr)) { \
        LOG_MSG(log_level, __VA_ARGS__); \
    } \
}while(0)

#define ASSERT(expr, ...) do { \
    if(!(expr)) { \
        LOG_MSG(L_FATAL, "ASSERT Fail:"__VA_ARGS__); \
        abort(); \
    } \
}while(0)

/* 
** opstr format like:
** "opname(arg1, arg2) != -1"
** or
** "(rc = opname(arg1, arg2)) != -1"
*/
#define OPNAME(buf, opstr) do { \
    char *_ot1_ = index(opstr + 1, '('); \
    char *_ot2_ = index(opstr + 1, '='); \
    if(_ot2_ && _ot2_ < _ot1_) { \
        memcpy(buf, _ot2_ + 1, _ot1_ - _ot2_ - 1); \
        buf[_ot1_ - _ot2_ - 1] = '\0'; \
    } else { \
        memcpy(buf, opstr, _ot1_ - opstr); \
        buf[_ot1_ - opstr] = '\0'; \
    } \
} while(0)

#ifdef DEBUG
#define TRACE(call, ...) ({ \
    char _c_[LTL_BUF_SIZE], _b_[MID_BUF_SIZE]; \
    OPNAME(_c_, #call); \
    Snprintf(_b_, sizeof(_b_), "%s:", _c_); \
    Snprintf(&_b_[strlen(_b_)], sizeof(_b_) - strlen(_b_), __VA_ARGS__); \
    log_trace(_b_); \
    call; \
})

#define CALL(call, log_level, ...) ({ \
    int _rcl_ = 1; \
    char _c_[LTL_BUF_SIZE], _b_[MID_BUF_SIZE]; \
    int _eno_ = errno; \
    OPNAME(_c_, #call); \
    Snprintf(_b_, sizeof(_b_), "%s:", _c_); \
    Snprintf(&_b_[strlen(_b_)], sizeof(_b_) - strlen(_b_), __VA_ARGS__); \
    log_trace(_b_); \
    if(!(call)) { \
        _rcl_ = 0; \
        _eno_ = errno; \
        if(_eno_ > 0) { LOG_MSG(log_level, "%s"PRErrFMT, _b_, PRErrVAL); } \
        else{ LOG_MSG(log_level, "%s", _b_); } \
    } \
    errno = _eno_; \
    _rcl_; \
})
#else
#define TRACE(call, ...) call

#define CALL(call, log_level, ...) ({ \
    int _rcl_ = 1; \
    if(!(call)) { \
        char _c_[LTL_BUF_SIZE], _b_[MID_BUF_SIZE]; \
        OPNAME(_c_, #call); \
        Snprintf(_b_, sizeof(_b_), "%s:", _c_); \
        Snprintf(&_b_[strlen(_b_)], sizeof(_b_) - strlen(_b_), __VA_ARGS__); \
        _rcl_ = 0; \
        if(errno > 0) { LOG_MSG(log_level, "%s"PRErrFMT, _b_, PRErrVAL); } \
        else{ LOG_MSG(log_level, "%s", _b_); } \
    } \
    _rcl_; \
})
#endif

#define CALL_WRN(call, ...) ({ \
    CALL(call, L_WARN, __VA_ARGS__); \
})

#define CALL_ERR(call, ...) ({ \
    int _rce_ = CALL(call, L_ERROR, __VA_ARGS__); \
    if(!_rce_) { \
        OUT_SET(OUT_ER()); \
    } \
    _rce_; \
})

#define CALL_OUT(call, ...) do { \
    int _rcc_ = CALL(call, L_ERROR, __VA_ARGS__); \
    if(!_rcc_) { \
        OUT_SET(OUT_ER()); \
        goto _out; \
    } \
} while(0)

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

#define Malloc(size) ({ \
    void *t = _malloc_(size); \
    ASSERT(t != NULL, "Malloc fail"); \
    t; \
})
    
#define Realloc(p, size) ({ \
    void *t = _realloc_(p, size); \
    ASSERT(t != NULL, "Realloc fail"); \
    t; \
})

#define Strdup(p) ({ \
    char *t = _strdup_(p); \
    ASSERT(t != NULL, "Strdup fail"); \
    t; \
})

#define Strndup(p, size) ({ \
    char *t = _strndup_(p, size); \
    ASSERT(t != NULL, "Strndup fail"); \
    t; \
})

#define Free(ptr) do{ \
    _free_(ptr); \
    ptr = NULL; \
}while(0)

#endif

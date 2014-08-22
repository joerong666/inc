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
#  include "prod_inc.h"
#else
#  define log_wrap(...) do{ \
      time_t t_; \
      struct tm lt; \
      char tm_s[LTL_BUF_SIZE]; \
      time(&t_); \
      strftime(tm_s, sizeof(tm_s), "%Y-%m-%d %H:%M:%S", localtime_r(&t_, &lt)); \
      fprintf(stderr, "%s,%s:%d,%s,thread[%ld],", tm_s, __FILE__, __LINE__, __func__, syscall(SYS_gettid)); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\n"); \
      fflush(stderr); \
  } while(0)

#  define log_trace  fprintf(stderr, "[TRACE] ");  log_wrap
#  define log_debug  fprintf(stderr, "[DEBUG] ");  log_wrap
#  define log_info   fprintf(stderr, "[INFO] ");   log_wrap
#  define log_warn   fprintf(stderr, "[WARN] ");   log_wrap
#  define log_error  fprintf(stderr, "[ERROR] ");  log_wrap
#  define log_fatal  fprintf(stderr, "[FATAL] ");  log_wrap
#  define log_prompt fprintf(stderr, "[PROMPT] "); log_wrap
#endif

/********************************************************
** define memory operations in prod_inc.h if needed
*********************************************************/
#ifndef _malloc_
#  define _malloc_  malloc
#endif
#ifndef _realloc_ 
#  define _realloc_ realloc
#endif
#ifndef _free_
#  define _free_    free
#endif
#ifndef _strdup_ 
#  define _strdup_  strdup
#endif
#ifndef
#  define _strndup_ strndup
#endif

#ifndef NTRACE
#  ifndef log_trace
#    define log_trace(...)
#  endif
#else
#  undef  log_trace
#  define log_trace(...)
#endif

#ifndef log_prompt
#  define log_prompt log_warn
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

#define PRStrFMT    "size=%lu,str=%s"
#define PRStrVAL(s) strlen(s),s

/********************************************************
** string operation
*********************************************************/
#define Snprintf(str, size, ...) ({ \
    int _snp_c_ = snprintf(str, (size), __VA_ARGS__); \
    if(_snp_c_ >= (int)(size)) { \
        log_warn("String truncated"); \
    } \
    _snp_c_; \
})

/* concat to dynamic temporary buf, may pick the same buf address allocated before */
#define CONCAT(...) ({ \
    char _ct_[SML_BUF_SIZE]; \
    Snprintf(_ct_, sizeof(_ct_), __VA_ARGS__); \
    _ct_; \
})

#define TRIM(s) ({ \
    int len = strlen(s); \
    if(len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n')) s[len - 1] = '\0'; \
    if(len > 1 && (s[len - 2] == '\r' || s[len - 2] == '\n')) s[len - 2] = '\0'; \
    s; \
})

/*******************************************************
**  LOG
********************************************************/ 
#define L_TRACE  -1
#define L_DEBUG  -2 
#define L_INFO   -3
#define L_WARN   -4
#define L_ERROR  -5
#define L_FATAL  -6

#define FULL_MSG(...) ({ \
    char _fm_[MID_BUF_SIZE]; \
    size_t _fer_ = errno; \
    Snprintf(_fm_, sizeof(_fm_), __VA_ARGS__); \
    if(er_no > 0) { \
        Snprintf(&_fm_[strlen(_fm_)], sizeof(_fm_) - strlen(_fm_), ",error:%s", strerror(_fer_)); \
    } \
    errno = _fer_; \
    _fm_; \
})

#define LOG_MSG(log_level, ...)  do{ \
    if(log_level == L_DEBUG)       {if(errno > 0){log_debug (FULL_MSG(__VA_ARGS__)); } else {log_debug (__VA_ARGS__); }} \
    else if(log_level == L_INFO)   {if(errno > 0){log_info  (FULL_MSG(__VA_ARGS__)); } else {log_info  (__VA_ARGS__); }} \
    else if(log_level == L_WARN)   {if(errno > 0){log_warn  (FULL_MSG(__VA_ARGS__)); } else {log_warn  (__VA_ARGS__); }} \
    else if(log_level == L_ERROR)  {if(errno > 0){log_error (FULL_MSG(__VA_ARGS__)); } else {log_error (__VA_ARGS__); }} \
    else if(log_level == L_FATAL)  {if(errno > 0){log_fatal (FULL_MSG(__VA_ARGS__)); } else {log_fatal (__VA_ARGS__); }} \
    else if(log_level == L_PROMPT) {if(errno > 0){log_prompt(FULL_MSG(__VA_ARGS__)); } else {log_prompt(__VA_ARGS__); }} \
    else                           {if(errno > 0){log_trace (FULL_MSG(__VA_ARGS__)); } else {log_trace (__VA_ARGS__); }} \
}while(0)

#define OUT_INIT() int _out_code_ = 0
#define OUT_OK() (_out_code_ == 0)
#define OUT_ER() (_out_code_ != 0)
#define OUT_SET(code) (_out_code_ = (code))

#define ASSERT(expr, ...) do { \
    if(!(expr)) { \
        LOG_MSG(L_FATAL, "ASSERT Fail:"__VA_ARGS__); \
        abort(); \
    } \
}while(0)

#define LOG_CALL(call, lvl, ...) ({ \
    LOG_MSG(lvl, __VA_ARGS__); \
    call; \
})

#ifndef NTRACE
#  define TRACE(call, ...) LOG_CALL(call, L_TRACE, __VA_ARGS__)

#  define CALL(call, lvl, ab_lvl, ...) ({ \
      int _rcl_ = 1; \
      int _eno_ = errno; \
      LOG_MSG(lvl, __VA_ARGS__); \
      if(!(call)) { \
          _rcl_ = 0; \
          _eno_ = errno; \
          LOG_MSG(ab_lvl, __VA_ARGS__); \
      } \
      errno = _eno_; \
      _rcl_; \
  })

#  define CHK_OUT(expr, ...) do{ \
      int _co_eno_ = errno; \
      LOG_MSG(L_TRACE, __VA_ARGS__); \
      if(!(expr)) { \
          OUT_SET(OUT_ER()); \
          LOG_MSG(L_ERROR, __VA_ARGS__); \
          errno = _co_eno_; \
          goto _out; \
      } \
      errno = _co_eno_; \
  }while(0)

#else
#  define TRACE(call, ...) call

#  define CALL(call, lvl, ab_lvl, ...) ({ \
      int _rcl_ = 1; \
      int _eno_ = errno; \
      if(!(call)) { \
          _rcl_ = 0; \
          _eno_ = errno; \
          LOG_MSG(ab_lvl, __VA_ARGS__); \
      } \
      errno = _eno_; \
      _rcl_; \
  })

#  define CHK_OUT(expr, ...) do{ \
      int _co_eno_ = errno; \
      if(!(expr)) { \
          OUT_SET(OUT_ER()); \
          LOG_MSG(L_ERROR, __VA_ARGS__); \
          errno = _co_eno_; \
          goto _out; \
      } \
      errno = _co_eno_; \
  }while(0)

#endif

#define CALL_WRN(call, lvl, ...) ({ \
    CALL(call, lvl, L_WARN, __VA_ARGS__); \
})

#define CALL_ERR(call, lvl, ...) ({ \
    int _rce_ = CALL(call, lvl, L_ERROR, __VA_ARGS__); \
    if(!_rce_) { \
        OUT_SET(OUT_ER()); \
    } \
    _rce_; \
})

#define CALL_OUT(call, lvl, ...) do { \
    int _rcc_ = CALL(call, lvl, L_ERROR, __VA_ARGS__); \
    if(!_rcc_) { \
        OUT_SET(OUT_ER()); \
        goto _out; \
    } \
} while(0)

#define TRACE_WRN(call, ...) CALL_WRN(call, L_TRACE, __VA_ARGS__)
#define TRACE_ERR(call, ...) CALL_ERR(call, L_TRACE, __VA_ARGS__)
#define TRACE_OUT(call, ...) CALL_OUT(call, L_TRACE, __VA_ARGS__)

#define DEBUG_WRN(call, ...) CALL_WRN(call, L_DEBUG, __VA_ARGS__)
#define DEBUG_ERR(call, ...) CALL_ERR(call, L_DEBUG, __VA_ARGS__)
#define DEBUG_OUT(call, ...) CALL_OUT(call, L_DEBUG, __VA_ARGS__)

#define INFO_WRN(call, ...) CALL_WRN(call, L_INFO, __VA_ARGS__)
#define INFO_ERR(call, ...) CALL_ERR(call, L_INFO, __VA_ARGS__)
#define INFO_OUT(call, ...) CALL_OUT(call, L_INFO, __VA_ARGS__)

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
#define Malloc(size) ({ \
    void *t = _malloc_(size); \
    ASSERT(t != NULL, "malloc, size=%lu", size); \
    t; \
})
    
#define Realloc(p, size) ({ \
    void *t = _realloc_(p, size); \
    ASSERT(t != NULL, "realloc, size=%lu", size); \
    t; \
})

#define Strdup(p) ({ \
    char *t = _strdup_(p); \
    ASSERT(t != NULL, "strdup"); \
    t; \
})

#define Strndup(p, size) ({ \
    char *t = _strndup_(p, size); \
    ASSERT(t != NULL, "strndup, size=%lu", size); \
    t; \
})

#define Free(ptr) do{ \
    _free_(ptr); \
    ptr = NULL; \
}while(0)

#endif

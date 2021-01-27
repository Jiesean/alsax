#ifndef __SYS_PLAT_H__
#define __SYS_PLAT_H__

#include <stdint.h>
#include <stddef.h>

#if defined QUALCOMM_4010
    #include <qcom/qcom_mem.h>   
    #include <qcom/qcom_utils.h>
#endif

#if defined(WIN32)
    #define __ALIGNED_1
#elif defined(__GNUC__)
    #define __ALIGNED_1//__attribute__((packed, aligned(1)))
#elif defined(CORE_M4)
    #define __ALIGNED_1
#else
    #error "Unknown platform, forced alignment(1 byte) need to be specified!"
#endif


#if defined QUALCOMM_4010
    #define LLONG_MIN  (~0x7fffffffffffffffLL)    
#endif

#if defined QUALCOMM_4010
    #define WK_PRINTF qcom_printf
#else
    #define WK_PRINTF printf
#endif


#define MEM_LOG_DEBUG 0

#if MEM_LOG_DEBUG
    #define WK_MALLOC(x) wk_malloc(x, __FILE__, __LINE__)
    #define WK_FREE(x) wk_free(x, __FILE__, __LINE__)
#else
    #if defined QUALCOMM_4010
        #define WK_MALLOC(x) qcom_mem_alloc(x)
        #define WK_FREE(x) qcom_mem_free(x)
    #else
        #define WK_MALLOC(x) malloc(x)
        #define WK_FREE(x) free(x)
    #endif
#endif  //MEM_LOG_DEBUG

#ifdef __cplusplus
extern "C"
{
#endif

extern int32_t wk_total_size;
void *wk_malloc(size_t size, const char *file, int32_t line);
void wk_free(void *ptr, const char *file, int32_t line);

#ifdef __cplusplus
}
#endif

#endif // __SYS_PLAT_H__


#include "CommonDefs.h"
#include "PlatformUtils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

logPrintFunc globalCustomLogPrintFn = defaultLogPrint;
memAlloc globalMemAlloc = defaultMemAlloc;
memAlignAlloc globalMemAlignAlloc = NULL;
memCalloc globalMemCalloc = defaultMemCalloc;
memRealloc globalMemRealloc = defaultMemRealloc;
memFree globalMemFree = defaultMemFree;
getTime globalGetTime = defaultGetTime;
getTime globalGetRealTime = defaultGetTime;

PVOID defaultMemAlloc(SIZE_T size)
{
    return malloc(size);
}

PVOID defaultMemCalloc(SIZE_T num, SIZE_T size)
{
    return calloc(num, size);
}

PVOID defaultMemRealloc(PVOID ptr, SIZE_T size)
{
    return realloc(ptr, size);
}

VOID defaultMemFree(VOID* ptr)
{
    free(ptr);
}

VOID defaultLogPrint(UINT32 level, const PCHAR tag, const PCHAR fmt, ...)
{
    va_list args;

    (void) level;
    fprintf(stderr, "[%s] ", tag == NULL ? "log" : tag);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
}

UINT64 defaultGetTime()
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (UINT64) ts.tv_sec * HUNDREDS_OF_NANOS_IN_A_SECOND + (UINT64) ts.tv_nsec / 100U;
}

VOID customAssert(INT64 condition, const CHAR* fileName, INT64 lineNumber, const CHAR* functionName)
{
    if (condition) {
        return;
    }

    fprintf(stderr,
            "[assert] file=%s line=%lld function=%s\n",
            fileName == NULL ? "unknown" : fileName,
            (long long) lineNumber,
            functionName == NULL ? "unknown" : functionName);
    abort();
}

#include <pthread.h>
#include <stddef.h>
#include <unistd.h>

#include "../../../core/osal/include/osapi-module.h"

static pthread_mutex_t gOsalBspConsoleMutex = PTHREAD_MUTEX_INITIALIZER;

const OS_static_symbol_record_t OS_STATIC_SYMBOL_TABLE[] = {
    {NULL, NULL, NULL},
};

void OS_BSP_Lock_Impl(void)
{
    (void) pthread_mutex_lock(&gOsalBspConsoleMutex);
}

void OS_BSP_Unlock_Impl(void)
{
    (void) pthread_mutex_unlock(&gOsalBspConsoleMutex);
}

void OS_BSP_ConsoleOutput_Impl(const char* pBuffer, size_t uLength)
{
    if (pBuffer == NULL || uLength == 0U) {
        return;
    }

    (void) write(STDERR_FILENO, pBuffer, uLength);
}

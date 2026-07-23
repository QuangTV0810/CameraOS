#include "CommonDefs.h"
#include "osapi-common.h"
#include "osapi-error.h"

#include "app.h"

void OS_Application_Startup(void)
{
    int retStatus;

    retStatus = OS_API_Init();
    if (retStatus != OS_SUCCESS) {
        OS_ApplicationExit(retStatus);
        return;
    }

    retStatus = initApp();
    if (retStatus != STATUS_SUCCESS) {
        OS_ApplicationExit(retStatus);
    }
}

void OS_Application_Run(void)
{
    startApp();
    deinitApp();
}

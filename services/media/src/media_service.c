#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_CLASS "media-service"
#include "cameraos/services/media/media_service.h"

#include "../../../core/osal/include/osapi-common.h"
#include "../../../core/osal/include/osapi-constants.h"
#include "../../../core/osal/include/osapi-error.h"
#include "../../../core/osal/include/osapi-mutex.h"
#include "../../../core/osal/include/osapi-task.h"
#if defined(CAMERAOS_HAL_VENDOR_RV1106)
#include "rv1106_hal.h"
#elif defined(CAMERAOS_HAL_VENDOR_RTS3917N)
#include "rts3917n_hal.h"
#else
#error "Unsupported HAL vendor"
#endif

#define MEDIA_SERVICE_TASK_STACK_SIZE      (64U * 1024U)
#define MEDIA_SERVICE_TASK_PRIORITY        OSAL_PRIORITY_C(120)
#define MEDIA_SERVICE_FRAME_TIMEOUT_MS     2000
#define MEDIA_SERVICE_STOP_WAIT_MS         50U
#define MEDIA_SERVICE_VIDEO_MAIN_STREAM_ID 0U
#define MEDIA_SERVICE_VIDEO_SUB_STREAM_ID  1U
#define MEDIA_SERVICE_VIDEO_AI_STREAM_ID   2U

typedef enum {
    MEDIA_SERVICE_INTERNAL_STATE_STOPPED = 0x01,
    MEDIA_SERVICE_INTERNAL_STATE_STARTING = 0x02,
    MEDIA_SERVICE_INTERNAL_STATE_RUNNING = 0x04,
    MEDIA_SERVICE_INTERNAL_STATE_ERROR = 0x08,
    MEDIA_SERVICE_INTERNAL_STATE_STOPPING = 0x10,
} MEDIA_SERVICE_INTERNAL_STATE_E;

typedef struct __CameraOSMediaServicePipelineRuntime CameraOSMediaServicePipelineRuntime;
struct __CameraOSMediaServicePipelineRuntime {
    bool bWorkerRunning;
    bool bStopRequested;
    osal_id_t taskId;
    FILE* pEncodeOutputFile;
    MEDIA_SERVICE_INTERNAL_STATE_E enState;
};

struct __CameraOSMediaServiceHandle {
    CameraOSHalSystemConfig stHalSystemConfig;
    const CameraOSHalOps* pHalOps;
    PCameraOSHalHandle pCameraOSHalHandle;
    uint32_t u32PipelineCount;
    CameraOSMediaServicePipelineConfig astPipelineConfig[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSMediaServicePipelineRuntime astPipelineRuntime[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSMediaServiceFrameSink astSink[CAMERAOS_MEDIA_MAX_SINKS];
    bool abSinkUsed[CAMERAOS_MEDIA_MAX_SINKS];
    osal_id_t mutexId;
};

static PCameraOSMediaServiceHandle gCameraOSMediaServiceHandle = NULL;

static const CameraOSHalOps* mediaServiceGetDefaultHalOps(void)
{
#if defined(CAMERAOS_HAL_VENDOR_RV1106)
    return getRV1106HalDefaultImpl();
#elif defined(CAMERAOS_HAL_VENDOR_RTS3917N)
    return getRTS3917NHalDefaultImpl();
#endif
}

static void mediaServiceHalSystemConfig(PCameraOSHalSystemConfig pSystemConfig)
{
    if (pSystemConfig == NULL) {
        return;
    }

    memset(pSystemConfig, 0, sizeof(*pSystemConfig));
    pSystemConfig->u32CameraId = 0U;
    pSystemConfig->u32ViDevId = 0U;
    pSystemConfig->u32ViPipeId = 0U;
    pSystemConfig->bEnableIsp = false;
    pSystemConfig->pIqDir = NULL;
}

static int mediaServiceLock(PCameraOSMediaServiceHandle pHandle)
{
    if (pHandle == NULL) {
        return HAL_ERR_INVALID_ARG;
    }

    return (OS_MutSemTake(pHandle->mutexId) == OS_SUCCESS) ? HAL_ERR_NONE : HAL_ERR_SYS;
}

static int mediaServiceUnlock(PCameraOSMediaServiceHandle pHandle)
{
    if (pHandle == NULL) {
        return HAL_ERR_INVALID_ARG;
    }

    return (OS_MutSemGive(pHandle->mutexId) == OS_SUCCESS) ? HAL_ERR_NONE : HAL_ERR_SYS;
}

static int mediaServiceSetPipelineState(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, MEDIA_SERVICE_INTERNAL_STATE_E enState)
{
    if (pHandle == NULL || u32Slot >= pHandle->u32PipelineCount) {
        return HAL_ERR_INVALID_ARG;
    }

    pHandle->astPipelineRuntime[u32Slot].enState = enState;
    return HAL_ERR_NONE;
}

static MEDIA_SERVICE_STATE_E mediaServiceMapState(MEDIA_SERVICE_INTERNAL_STATE_E enState)
{
    switch (enState) {
        case MEDIA_SERVICE_INTERNAL_STATE_STOPPED:
            return MEDIA_SERVICE_STATE_STOPPED;
        case MEDIA_SERVICE_INTERNAL_STATE_STARTING:
            return MEDIA_SERVICE_STATE_STARTING;
        case MEDIA_SERVICE_INTERNAL_STATE_RUNNING:
            return MEDIA_SERVICE_STATE_RUNNING;
        case MEDIA_SERVICE_INTERNAL_STATE_STOPPING:
            return MEDIA_SERVICE_STATE_STOPPING;
        case MEDIA_SERVICE_INTERNAL_STATE_ERROR:
        default:
            return MEDIA_SERVICE_STATE_ERROR;
    }
}

static void mediaServiceSnapshotSinks(PCameraOSMediaServiceHandle pHandle, CameraOSMediaServiceFrameSink* pstSinkSnapshot, uint32_t* pu32SinkCount)
{
    uint32_t u32Index = 0U;
    uint32_t u32Count = 0U;

    for (u32Index = 0U; u32Index < CAMERAOS_MEDIA_MAX_SINKS; ++u32Index) {
        if (pHandle->abSinkUsed[u32Index] == true) {
            pstSinkSnapshot[u32Count++] = pHandle->astSink[u32Index];
        }
    }

    *pu32SinkCount = u32Count;
}

static void mediaServiceDispatchEncodeFrame(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, PCameraOSHalVideoEncodeFrame pFrame)
{
    CameraOSMediaServiceFrameSink astSinkSnapshot[CAMERAOS_MEDIA_MAX_SINKS];
    CameraOSMediaServiceEncodeFrameInfo stFrameInfo;
    uint32_t u32SinkCount = 0U;
    uint32_t u32Index = 0U;

    if (pHandle == NULL || pFrame == NULL) {
        return;
    }

    if (mediaServiceLock(pHandle) != HAL_ERR_NONE) {
        return;
    }

    mediaServiceSnapshotSinks(pHandle, astSinkSnapshot, &u32SinkCount);
    mediaServiceUnlock(pHandle);

    stFrameInfo.u32PipelineId = pHandle->astPipelineConfig[u32Slot].u32PipelineId;
    stFrameInfo.u32VencChnId = pHandle->astPipelineConfig[u32Slot].u32PipelineId;
    stFrameInfo.pFrame = pFrame;

    for (u32Index = 0U; u32Index < u32SinkCount; ++u32Index) {
        if (astSinkSnapshot[u32Index].bEnableEncodedFrame && astSinkSnapshot[u32Index].onEncodedFrame != NULL) {
            astSinkSnapshot[u32Index].onEncodedFrame(&stFrameInfo, astSinkSnapshot[u32Index].pUserData);
        }
    }
}

static void mediaServiceEncodeWorkerLoop(uint32_t u32Slot)
{
    CameraOSMediaServicePipelineRuntime* pRuntime;
    CameraOSHalVideoEncodeFrame stFrame;
    int retStatus;

    if (gCameraOSMediaServiceHandle == NULL || u32Slot >= gCameraOSMediaServiceHandle->u32PipelineCount) {
        OS_TaskExit();
        return;
    }

    pRuntime = &gCameraOSMediaServiceHandle->astPipelineRuntime[u32Slot];
    pRuntime->bWorkerRunning = true;

    while (!pRuntime->bStopRequested) {
        memset(&stFrame, 0, sizeof(stFrame));
        retStatus = getCameraOSHalVideoEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, gCameraOSMediaServiceHandle->astPipelineConfig[u32Slot].u32PipelineId,
                                                   &stFrame, MEDIA_SERVICE_FRAME_TIMEOUT_MS);
        if (retStatus != HAL_ERR_NONE) {
            if (pRuntime->bStopRequested) {
                break;
            }
            OS_TaskDelay(10U);
            continue;
        }

        /* Write the encoded frame to the output file */
        if (pRuntime->pEncodeOutputFile != NULL && stFrame.data != NULL && stFrame.length > 0U) {
            fwrite(stFrame.data, 1U, stFrame.length, pRuntime->pEncodeOutputFile);
            fflush(pRuntime->pEncodeOutputFile);
        }

        /* Dispatch the encoded frame to all registered sinks */
        mediaServiceDispatchEncodeFrame(gCameraOSMediaServiceHandle, u32Slot, &stFrame);

        releaseCameraOSHalVideoEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, gCameraOSMediaServiceHandle->astPipelineConfig[u32Slot].u32PipelineId, &stFrame);
    }

    pRuntime->bWorkerRunning = false;
    OS_TaskExit();
}

static void taskMediaServicePipelineMainStream(void)
{
    mediaServiceEncodeWorkerLoop(MEDIA_SERVICE_VIDEO_MAIN_STREAM_ID);
}

static void taskMediaServicePipelineSubStream(void)
{
    mediaServiceEncodeWorkerLoop(MEDIA_SERVICE_VIDEO_SUB_STREAM_ID);
}

static void taskMediaServicePipelineAI(void)
{
    mediaServiceEncodeWorkerLoop(MEDIA_SERVICE_VIDEO_AI_STREAM_ID);
}

static osal_task_entry mediaServiceGetPipelineTaskEntry(uint32_t u32Slot)
{
    switch (u32Slot) {
        case MEDIA_SERVICE_VIDEO_MAIN_STREAM_ID:
            return taskMediaServicePipelineMainStream;
        case MEDIA_SERVICE_VIDEO_SUB_STREAM_ID:
            return taskMediaServicePipelineSubStream;
        case MEDIA_SERVICE_VIDEO_AI_STREAM_ID:
            return taskMediaServicePipelineAI;
        default:
            return NULL;
    }
}

static int mediaServiceOpenDumpFile(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    CameraOSMediaServicePipelineRuntime* pRuntime;
    CameraOSMediaServicePipelineConfig* pConfig;

    pRuntime = &pHandle->astPipelineRuntime[u32Slot];
    pConfig = &pHandle->astPipelineConfig[u32Slot];

    if (!pConfig->bWriteEncodeToFile || pConfig->pEncodeOutputFilePath == NULL) {
        return HAL_ERR_NONE;
    }

    pRuntime->pEncodeOutputFile = fopen(pConfig->pEncodeOutputFilePath, "wb");
    if (pRuntime->pEncodeOutputFile == NULL) {
        DLOGE("failed to open dump file path=%s", pConfig->pEncodeOutputFilePath);
        return HAL_ERR_SYS;
    }

    return HAL_ERR_NONE;
}

static void mediaServiceCloseDumpFile(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    CameraOSMediaServicePipelineRuntime* pRuntime = &pHandle->astPipelineRuntime[u32Slot];

    if (pRuntime->pEncodeOutputFile != NULL) {
        fclose(pRuntime->pEncodeOutputFile);
        pRuntime->pEncodeOutputFile = NULL;
    }
}

static int mediaServiceStartPipelineWorker(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    int32 osStatus;
    char acTaskName[OS_MAX_API_NAME];
    osal_task_entry entry;
    CameraOSMediaServicePipelineRuntime* pRuntime = &pHandle->astPipelineRuntime[u32Slot];

    entry = mediaServiceGetPipelineTaskEntry(u32Slot);
    if (entry == NULL) {
        return HAL_ERR_INVALID_ARG;
    }

    SNPRINTF(acTaskName, sizeof(acTaskName), "media%u", (unsigned int) pHandle->astPipelineConfig[u32Slot].u32PipelineId);
    pRuntime->taskId = OS_OBJECT_ID_UNDEFINED;
    pRuntime->bStopRequested = false;
    pRuntime->bWorkerRunning = false;

    osStatus =
        OS_TaskCreate(&pRuntime->taskId, acTaskName, entry, OSAL_TASK_STACK_ALLOCATE, MEDIA_SERVICE_TASK_STACK_SIZE, MEDIA_SERVICE_TASK_PRIORITY, 0U);
    if (osStatus != OS_SUCCESS) {
        DLOGE("OS_TaskCreate failed pipeline=%u status=%ld", pHandle->astPipelineConfig[u32Slot].u32PipelineId, OS_StatusToInteger(osStatus));
        pRuntime->taskId = OS_OBJECT_ID_UNDEFINED;
        return HAL_ERR_SYS;
    }

    return HAL_ERR_NONE;
}

static void mediaServiceStopPipelineWorker(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    CameraOSMediaServicePipelineRuntime* pRuntime = &pHandle->astPipelineRuntime[u32Slot];
    uint32_t waitCount = 0;

    pRuntime->bStopRequested = true;

    while (pRuntime->bWorkerRunning && waitCount < 10U) {
        OS_TaskDelay(MEDIA_SERVICE_STOP_WAIT_MS);
        ++waitCount;
    }

    if (pRuntime->taskId != OS_OBJECT_ID_UNDEFINED) {
        if (pRuntime->bWorkerRunning) {
            OS_TaskDelete(pRuntime->taskId);
        }
        pRuntime->taskId = OS_OBJECT_ID_UNDEFINED;
    }

    pRuntime->bWorkerRunning = false;
}

static int mediaServiceStartPipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    int retStatus;
    CameraOSMediaServicePipelineConfig* pConfig = &pHandle->astPipelineConfig[u32Slot];
    CameraOSMediaServicePipelineRuntime* pRuntime = &pHandle->astPipelineRuntime[u32Slot];

    retStatus = mediaServiceSetPipelineState(pHandle, u32Slot, MEDIA_SERVICE_INTERNAL_STATE_STARTING);
    if (retStatus != HAL_ERR_NONE) {
        return retStatus;
    }

    /*  Call hal to create video input and encode channels, and bind them together */
    retStatus = createCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId, &pConfig->stVideoInputConfig);
    if (retStatus != HAL_ERR_NONE) {
        goto error;
    }

    retStatus = createCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId, &pConfig->stVideoEncodeConfig);
    if (retStatus != HAL_ERR_NONE) {
        destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
        goto error;
    }

    retStatus = bindCameraOSHalVideoInputToVideoEncode(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId, pConfig->u32PipelineId);
    if (retStatus != HAL_ERR_NONE) {
        destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
        destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
        goto error;
    }

    /* Open dump file if enabled */
    mediaServiceOpenDumpFile(pHandle, u32Slot);

    if (pConfig->bEnableVideoEncodeFrame) {
        retStatus = mediaServiceStartPipelineWorker(pHandle, u32Slot);
        if (retStatus != HAL_ERR_NONE) {
            mediaServiceCloseDumpFile(pHandle, u32Slot);
            unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId, pConfig->u32PipelineId);
            destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
            destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
            goto error;
        }
    }

    pRuntime->bStopRequested = false;
    return mediaServiceSetPipelineState(pHandle, u32Slot, MEDIA_SERVICE_INTERNAL_STATE_RUNNING);

error:
    mediaServiceSetPipelineState(pHandle, u32Slot, MEDIA_SERVICE_INTERNAL_STATE_ERROR);
    return retStatus;
}

static int mediaServiceStopPipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    CameraOSMediaServicePipelineConfig* pConfig = &pHandle->astPipelineConfig[u32Slot];

    mediaServiceSetPipelineState(pHandle, u32Slot, MEDIA_SERVICE_INTERNAL_STATE_STOPPING);
    mediaServiceStopPipelineWorker(pHandle, u32Slot);
    mediaServiceCloseDumpFile(pHandle, u32Slot);
    unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId, pConfig->u32PipelineId);
    destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
    destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pConfig->u32PipelineId);
    return mediaServiceSetPipelineState(pHandle, u32Slot, MEDIA_SERVICE_INTERNAL_STATE_STOPPED);
}

int createCameraOSMediaService(PCameraOSMediaServiceConfig pConfig, PCameraOSMediaServiceHandle* ppHandle)
{
    PCameraOSMediaServiceHandle pHandle;
    uint32_t i;
    int32 osStatus;
    char acMutexName[OS_MAX_API_NAME];

    ENTER();
    if (pConfig == NULL || ppHandle == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }
    if (pConfig->u32PipelineCount > CAMERAOS_MEDIA_MAX_PIPELINES) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    const CameraOSHalOps* pHalOps = mediaServiceGetDefaultHalOps();
    if (pHalOps == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_STATE;
    }

    pHandle = (PCameraOSMediaServiceHandle) calloc(1, sizeof(*pHandle));
    if (pHandle == NULL) {
        LEAVE();
        return HAL_ERR_NOMEM;
    }

    memcpy(&pHandle->astPipelineConfig, &pConfig->astPipelineConfig, sizeof(pHandle->astPipelineConfig));
    mediaServiceHalSystemConfig(&pHandle->stHalSystemConfig);

    pHandle->pHalOps = pHalOps;
    pHandle->u32PipelineCount = pConfig->u32PipelineCount;
    pHandle->mutexId = OS_OBJECT_ID_UNDEFINED;

    SNPRINTF(acMutexName, sizeof(acMutexName), "media_svc");
    osStatus = OS_MutSemCreate(&pHandle->mutexId, acMutexName, 0U);
    if (osStatus != OS_SUCCESS) {
        free(pHandle);
        LEAVE();
        return HAL_ERR_SYS;
    }

    if (createCameraOSHal(&pHandle->stHalSystemConfig, (PCameraOSHalOps) pHandle->pHalOps, &pHandle->pCameraOSHalHandle) != HAL_ERR_NONE) {
        OS_MutSemDelete(pHandle->mutexId);
        free(pHandle);
        LEAVE();
        return HAL_ERR_SYS;
    }

    for (i = 0; i < pHandle->u32PipelineCount; ++i) {
        mediaServiceSetPipelineState(pHandle, i, MEDIA_SERVICE_INTERNAL_STATE_STOPPED);
        pHandle->astPipelineRuntime[i].taskId = OS_OBJECT_ID_UNDEFINED;
    }

    gCameraOSMediaServiceHandle = pHandle;

    *ppHandle = pHandle;

    LEAVE();

    return HAL_ERR_NONE;
}

int destroyCameraOSMediaService(PCameraOSMediaServiceHandle pHandle)
{
    uint32_t u32Index = 0U;

    ENTER();
    if (pHandle == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    for (u32Index = 0; u32Index < pHandle->u32PipelineCount; ++u32Index) {
        if (pHandle->astPipelineRuntime[u32Index].enState != MEDIA_SERVICE_INTERNAL_STATE_STOPPED) {
            mediaServiceStopPipeline(pHandle, u32Index);
        } else {
            mediaServiceCloseDumpFile(pHandle, u32Index);
        }
    }

    if (pHandle->pCameraOSHalHandle != NULL) {
        destroyCameraOSHal(pHandle->pCameraOSHalHandle);
        pHandle->pCameraOSHalHandle = NULL;
    }

    if (pHandle->mutexId != OS_OBJECT_ID_UNDEFINED) {
        OS_MutSemDelete(pHandle->mutexId);
        pHandle->mutexId = OS_OBJECT_ID_UNDEFINED;
    }

    if (gCameraOSMediaServiceHandle == pHandle) {
        gCameraOSMediaServiceHandle = NULL;
    }

    free(pHandle);

    LEAVE();

    return HAL_ERR_NONE;
}

int startCameraOSMediaServicePipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    int32_t s32Ret;
    uint32_t u32Slot;
    uint32_t i;

    ENTER();
    if (pHandle == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    u32Slot = pHandle->u32PipelineCount;
    for (i = 0; i < pHandle->u32PipelineCount; ++i) {
        if (pHandle->astPipelineConfig[i].u32PipelineId == u32PipelineId) {
            u32Slot = i;
            break;
        }
    }

    if (u32Slot >= pHandle->u32PipelineCount) {
        LEAVE();
        return HAL_ERR_NOT_FOUND;
    }

    s32Ret = mediaServiceLock(pHandle);
    if (s32Ret != HAL_ERR_NONE) {
        LEAVE();
        return s32Ret;
    }

    if (!pHandle->astPipelineConfig[u32Slot].bEnabled) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return HAL_ERR_INVALID_STATE;
    }

    if (pHandle->astPipelineRuntime[u32Slot].enState != MEDIA_SERVICE_INTERNAL_STATE_STOPPED) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return HAL_ERR_INVALID_STATE;
    }

    mediaServiceUnlock(pHandle);
    s32Ret = mediaServiceStartPipeline(pHandle, u32Slot);
    LEAVE();
    return s32Ret;
}

int stopCameraOSMediaServicePipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    uint32_t u32Slot;
    uint32_t i;

    ENTER();
    if (pHandle == NULL) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    u32Slot = pHandle->u32PipelineCount;
    for (i = 0; i < pHandle->u32PipelineCount; ++i) {
        if (pHandle->astPipelineConfig[i].u32PipelineId == u32PipelineId) {
            u32Slot = i;
            break;
        }
    }

    if (u32Slot >= pHandle->u32PipelineCount) {
        LEAVE();
        return HAL_ERR_NOT_FOUND;
    }

    if (pHandle->astPipelineRuntime[u32Slot].enState == MEDIA_SERVICE_INTERNAL_STATE_STOPPED) {
        LEAVE();
        return HAL_ERR_NONE;
    }

    LEAVE();
    return mediaServiceStopPipeline(pHandle, u32Slot);
}

int registerCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceFrameSink pSink)
{
    uint32_t i;
    int retStatus;

    ENTER();
    if (pHandle == NULL || pSink == NULL || pSink->pName == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    retStatus = mediaServiceLock(pHandle);
    if (retStatus != HAL_ERR_NONE) {
        LEAVE();
        return retStatus;
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] && STRCMP(pHandle->astSink[i].pName, pSink->pName) == 0) {
            mediaServiceUnlock(pHandle);
            LEAVE();
            return HAL_ERR_INVALID_STATE;
        }
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] == false) {
            pHandle->astSink[i] = *pSink;
            pHandle->abSinkUsed[i] = true;
            mediaServiceUnlock(pHandle);
            LEAVE();
            return HAL_ERR_NONE;
        }
    }

    mediaServiceUnlock(pHandle);
    LEAVE();
    return HAL_ERR_INVALID_STATE;
}

int unregisterCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, const char* pSinkName)
{
    uint32_t i;
    int retStatus;

    ENTER();
    if (pHandle == NULL || pSinkName == NULL) {
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    retStatus = mediaServiceLock(pHandle);
    if (retStatus != HAL_ERR_NONE) {
        LEAVE();
        return retStatus;
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] && STRCMP(pHandle->astSink[i].pName, pSinkName) == 0) {
            memset(&pHandle->astSink[i], 0, sizeof(pHandle->astSink[i]));
            pHandle->abSinkUsed[i] = false;
            mediaServiceUnlock(pHandle);
            LEAVE();
            return HAL_ERR_NONE;
        }
    }

    mediaServiceUnlock(pHandle);
    LEAVE();
    return HAL_ERR_NOT_FOUND;
}

MEDIA_SERVICE_STATE_E getCameraOSMediaServiceState(PCameraOSMediaServiceHandle pHandle)
{
    uint32_t i;
    bool bHasRunning = false;

    if (pHandle == NULL) {
        return MEDIA_SERVICE_STATE_ERROR;
    }

    for (i = 0; i < pHandle->u32PipelineCount; ++i) {
        switch (pHandle->astPipelineRuntime[i].enState) {
            case MEDIA_SERVICE_INTERNAL_STATE_ERROR:
                return MEDIA_SERVICE_STATE_ERROR;
            case MEDIA_SERVICE_INTERNAL_STATE_STARTING:
                return MEDIA_SERVICE_STATE_STARTING;
            case MEDIA_SERVICE_INTERNAL_STATE_STOPPING:
                return MEDIA_SERVICE_STATE_STOPPING;
            case MEDIA_SERVICE_INTERNAL_STATE_RUNNING:
                bHasRunning = true;
                break;
            case MEDIA_SERVICE_INTERNAL_STATE_STOPPED:
            default:
                break;
        }
    }

    return bHasRunning ? MEDIA_SERVICE_STATE_RUNNING : MEDIA_SERVICE_STATE_STOPPED;
}

int getCameraOSMediaServiceConfig(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId, PCameraOSMediaServicePipelineConfig pConfig)
{
    uint32_t u32Slot;
    uint32_t i;

    if (pHandle == NULL || pConfig == NULL) {
        return HAL_ERR_INVALID_ARG;
    }

    u32Slot = pHandle->u32PipelineCount;
    for (i = 0; i < pHandle->u32PipelineCount; ++i) {
        if (pHandle->astPipelineConfig[i].u32PipelineId == u32PipelineId) {
            u32Slot = i;
            break;
        }
    }

    if (u32Slot >= pHandle->u32PipelineCount) {
        return HAL_ERR_NOT_FOUND;
    }

    *pConfig = pHandle->astPipelineConfig[u32Slot];
    return HAL_ERR_NONE;
}

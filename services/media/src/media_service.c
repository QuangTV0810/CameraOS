#include <stdlib.h>
#include <string.h>

#define LOG_CLASS "media-service"
#include "cameraos/services/media/media_service.h"

#include "osapi-common.h"
#include "osapi-constants.h"
#include "osapi-error.h"
#include "osapi-mutex.h"
#include "osapi-task.h"
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
#define MEDIA_SERVICE_AUDIO_CHANNEL_ID     0U

typedef struct __CameraOSMediaServiceVideoRuntime CameraOSMediaServiceVideoRuntime;
/**
 * @brief Runtime state for one video pipeline instance.
 */
struct __CameraOSMediaServiceVideoRuntime {
    /** True after the video input and video encode channels are created successfully. */
    bool bCreated;

    /** True after the video input channel is bound to the video encode channel. */
    bool bBound;

    /** True while the video worker task is running. */
    bool bWorkerRunning;

    /** Set to true to request the video worker task to stop. */
    bool bStopRequested;

    /** OS task identifier of the video worker task. */
    osal_id_t taskId;
};

typedef struct __CameraOSMediaServiceAudioRuntime CameraOSMediaServiceAudioRuntime;
/**
 * @brief Runtime state for the shared audio pipeline instance.
 */
struct __CameraOSMediaServiceAudioRuntime {
    /** True after the audio input and audio encode channels are created successfully. */
    bool bCreated;

    /** True after the audio input channel is bound to the audio encode channel. */
    bool bBound;

    /** True while the audio worker task is running. */
    bool bWorkerRunning;

    /** Set to true to request the audio worker task to stop. */
    bool bStopRequested;

    /** OS task identifier of the audio worker task. */
    osal_id_t taskId;
};

/**
 * @brief Media service singleton runtime context.
 */
struct __CameraOSMediaServiceHandle {
    /** System-level HAL configuration used when creating the HAL instance. */
    CameraOSHalSystemConfig stHalSystemConfig;

    /** HAL instance owned by the media service. */
    PCameraOSHalHandle pCameraOSHalHandle;

    /** Default HAL operation table selected for the active vendor. */
    const CameraOSHalOps* pHalOps;

    /** Static video service configuration copied at service creation time. */
    CameraOSMediaServiceVideoConfig stVideoConfig;

    /** Runtime state for each configured video pipeline slot. */
    CameraOSMediaServiceVideoRuntime astVideoRuntime[CAMERAOS_MEDIA_MAX_PIPELINES];

    /** Static audio service configuration copied at service creation time. */
    CameraOSMediaServiceAudioConfig stAudioConfig;

    /** Runtime state for the shared audio pipeline. */
    CameraOSMediaServiceAudioRuntime stAudioRuntime;

    /** Registered frame sinks that receive video and audio callbacks. */
    CameraOSMediaServiceFrameSink astSink[CAMERAOS_MEDIA_MAX_SINKS];

    /** Slot usage bitmap for the registered sink table. */
    bool abSinkUsed[CAMERAOS_MEDIA_MAX_SINKS];

    /** Mutex protecting sink registration and other shared service state. */
    osal_id_t mutexId;
};

static PCameraOSMediaServiceHandle gCameraOSMediaServiceHandle = NULL;

static const CameraOSHalOps* mediaServiceGetDefaultHalOps(void);
static int mediaServiceHalSystemConfig(PCameraOSHalSystemConfig pSystemConfig);

/** @brief Lock the media service mutex. */
static int mediaServiceLock(PCameraOSMediaServiceHandle pHandle);

/** @brief Unlock the media service mutex. */
static int mediaServiceUnlock(PCameraOSMediaServiceHandle pHandle);

/** @brief Worker task entry for the main video stream pipeline. */
static void taskMediaServiceVideoMainStream(void);

/** @brief Worker task entry for the sub video stream pipeline. */
static void taskMediaServiceVideoSubStream(void);

/** @brief Worker task entry for the AI video stream pipeline. */
static void taskMediaServiceVideoAIStream(void);

/** @brief Worker task entry for the shared audio stream pipeline. */
static void taskMediaServiceAudioStream(void);

/** @brief Create the video worker task. */
static int mediaServiceVideoCreateTask(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot);

/** @brief Delete the video worker task. */
static void mediaServiceVideoDeleteTask(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot);

/** @brief Create the audio worker task. */
static int mediaServiceAudioCreateTask(PCameraOSMediaServiceHandle pHandle);

/** @brief Delete the audio worker task. */
static void mediaServiceAudioDeleteTask(PCameraOSMediaServiceHandle pHandle);

/** @brief Worker loop for video stream pipelines. */
static void mediaServiceVideoWorkerLoop(uint32_t u32Slot);

/** @brief Worker loop for audio stream pipelines. */
static void mediaServiceAudioWorkerLoop(void);

/** @brief Get the task entry for a video stream pipeline. */
static osal_task_entry mediaServiceGetVideoTaskEntry(uint32_t u32Slot);

static void mediaServiceSnapshotSinks(PCameraOSMediaServiceHandle pHandle, CameraOSMediaServiceFrameSink* pstSinkSnapshot, uint32_t* pu32SinkCount);

static void mediaServiceDispatchVideoRawFrame(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, PCameraOSHalVideoInputFrame pFrame);
static void mediaServiceDispatchVideoEncodeFrame(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, PCameraOSHalVideoEncodeFrame pFrame);
static void mediaServiceDispatchAudioRawFrame(PCameraOSMediaServiceHandle pHandle, PCameraOSHalAudioInputFrame pFrame);
static void mediaServiceDispatchAudioEncodeFrame(PCameraOSMediaServiceHandle pHandle, PCameraOSHalAudioEncodeFrame pFrame);

static int startCameraOSMediaServiceAudio(PCameraOSMediaServiceHandle pHandle);
static int stopCameraOSMediaServiceAudio(PCameraOSMediaServiceHandle pHandle);
static int startCameraOSMediaServiceVideo(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);
static int stopCameraOSMediaServiceVideo(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);

/******************************************************************************************
 * PUBLIC API
 ******************************************************************************************/

int createCameraOSMediaService(PCameraOSMediaServiceConfig pConfig, PCameraOSMediaServiceHandle* ppHandle)
{
    PCameraOSMediaServiceHandle pHandle;
    uint32_t i = 0U;
    int32_t s32Ret = STATUS_SUCCESS;
    char acMutexName[OS_MAX_API_NAME];

    ENTER();

    if (pConfig == NULL || ppHandle == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    if (pConfig->stVideoConfig.u32PipelineCount > CAMERAOS_MEDIA_MAX_PIPELINES) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    /** Get the default HAL implementation */
    const CameraOSHalOps* pHalOps = mediaServiceGetDefaultHalOps();
    if (pHalOps == NULL) {
        LEAVE();
        return STATUS_INVALID_OPERATION;
    }

    pHandle = (PCameraOSMediaServiceHandle) calloc(1, sizeof(*pHandle));
    if (pHandle == NULL) {
        LEAVE();
        return STATUS_NOT_ENOUGH_MEMORY;
    }

    pHandle->pHalOps = pHalOps;
    pHandle->stVideoConfig = pConfig->stVideoConfig;
    pHandle->stAudioConfig = pConfig->stAudioConfig;
    s32Ret = mediaServiceHalSystemConfig(&pHandle->stHalSystemConfig);

    /** Create a mutex for thread safety */
    pHandle->mutexId = OS_OBJECT_ID_UNDEFINED;
    SNPRINTF(acMutexName, sizeof(acMutexName), "media_svc");
    s32Ret = OS_MutSemCreate(&pHandle->mutexId, acMutexName, 0U);
    if (s32Ret != OS_SUCCESS) {
        DLOGE("OS_MutSemCreate failed status=%ld", OS_StatusToInteger(s32Ret));
        free(pHandle);
        LEAVE();
        return STATUS_INTERNAL_ERROR;
    }

    /** Create the HAL instance */
    s32Ret = createCameraOSHal(&pHandle->stHalSystemConfig, (PCameraOSHalOps) pHandle->pHalOps, &pHandle->pCameraOSHalHandle);
    if (s32Ret != HAL_ERR_NONE) {
        DLOGE("createCameraOSHal failed ret=%d", s32Ret);
        OS_MutSemDelete(pHandle->mutexId);
        free(pHandle);
        LEAVE();
        return STATUS_INTERNAL_ERROR;
    }

    /** Set the global media service handle */
    gCameraOSMediaServiceHandle = pHandle;

    /** Return the handle to the caller */
    *ppHandle = pHandle;

    LEAVE();

    return STATUS_SUCCESS;
}

int destroyCameraOSMediaService(PCameraOSMediaServiceHandle pHandle)
{
    uint32_t u32Index = 0U;

    ENTER();
    if (pHandle == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    for (u32Index = 0; u32Index < pHandle->stVideoConfig.u32PipelineCount; ++u32Index) {
        if (pHandle->astVideoRuntime[u32Index].bCreated) {
            mediaServiceVideoDeleteTask(pHandle, u32Index);
            if (pHandle->astVideoRuntime[u32Index].bBound) {
                unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Index],
                                                            pHandle->stVideoConfig.au32PipelineId[u32Index]);
                pHandle->astVideoRuntime[u32Index].bBound = false;
            }
            destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Index]);
            destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Index]);
            pHandle->astVideoRuntime[u32Index].bCreated = false;
        }
    }
    if (pHandle->stAudioRuntime.bCreated) {
        mediaServiceAudioDeleteTask(pHandle);
        if (pHandle->stAudioRuntime.bBound) {
            unbindCameraOSHalAudioEncodeFromAudioInput(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
            pHandle->stAudioRuntime.bBound = false;
        }
        destroyCameraOSHalAudioEncodeChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        destroyCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        pHandle->stAudioRuntime.bCreated = false;
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

    return STATUS_SUCCESS;
}

int startCameraOSMediaService(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    int32_t s32Ret = STATUS_SUCCESS;

    ENTER();

    s32Ret = startCameraOSMediaServiceVideo(pHandle, u32PipelineId);
    if (s32Ret != STATUS_SUCCESS) {
        DLOGE("Start service video %u failed with ret=%d", u32PipelineId, s32Ret);
        return s32Ret;
    }

    s32Ret = startCameraOSMediaServiceAudio(pHandle);
    if (s32Ret != STATUS_SUCCESS && s32Ret != STATUS_INVALID_OPERATION) {
        stopCameraOSMediaServiceVideo(pHandle, u32PipelineId);
        DLOGE("Start service audio failed with ret=%d", s32Ret);
        return s32Ret;
    }

    LEAVE();

    return STATUS_SUCCESS;
}

int stopCameraOSMediaService(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    int s32Ret;

    ENTER();
    s32Ret = stopCameraOSMediaServiceVideo(pHandle, u32PipelineId);
    if (s32Ret != HAL_ERR_NONE) {
        LEAVE();
        return s32Ret;
    }

    s32Ret = stopCameraOSMediaServiceAudio(pHandle);
    if (s32Ret != HAL_ERR_NONE) {
        LEAVE();
        return s32Ret;
    }

    LEAVE();
    return HAL_ERR_NONE;
}

int registerCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceFrameSink pSink)
{
    uint32_t i;
    int retStatus;

    ENTER();
    if (pHandle == NULL || pSink == NULL || pSink->pName == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    retStatus = mediaServiceLock(pHandle);
    if (retStatus != STATUS_SUCCESS) {
        LEAVE();
        return retStatus;
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] && STRCMP(pHandle->astSink[i].pName, pSink->pName) == 0) {
            mediaServiceUnlock(pHandle);
            LEAVE();
            return STATUS_INVALID_OPERATION;
        }
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] == false) {
            pHandle->astSink[i] = *pSink;
            pHandle->abSinkUsed[i] = true;
            mediaServiceUnlock(pHandle);
            LEAVE();
            return STATUS_SUCCESS;
        }
    }

    mediaServiceUnlock(pHandle);
    LEAVE();
    return STATUS_INVALID_OPERATION;
}

int unregisterCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, const char* pSinkName)
{
    uint32_t i;
    int retStatus;

    ENTER();
    if (pHandle == NULL || pSinkName == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    retStatus = mediaServiceLock(pHandle);
    if (retStatus != STATUS_SUCCESS) {
        LEAVE();
        return retStatus;
    }

    for (i = 0; i < CAMERAOS_MEDIA_MAX_SINKS; ++i) {
        if (pHandle->abSinkUsed[i] && STRCMP(pHandle->astSink[i].pName, pSinkName) == 0) {
            memset(&pHandle->astSink[i], 0, sizeof(pHandle->astSink[i]));
            pHandle->abSinkUsed[i] = false;
            mediaServiceUnlock(pHandle);
            LEAVE();
            return STATUS_SUCCESS;
        }
    }

    mediaServiceUnlock(pHandle);
    LEAVE();
    return STATUS_NOT_FOUND;
}

MEDIA_SERVICE_STATE_E getCameraOSMediaServiceState(PCameraOSMediaServiceHandle pHandle)
{
    uint32_t i;
    bool bHasCreated = false;

    if (pHandle == NULL) {
        return MEDIA_SERVICE_STATE_ERROR;
    }

    for (i = 0; i < pHandle->stVideoConfig.u32PipelineCount; ++i) {
        if (pHandle->astVideoRuntime[i].bStopRequested && pHandle->astVideoRuntime[i].bWorkerRunning) {
            return MEDIA_SERVICE_STATE_STOPPING;
        }
        if (pHandle->astVideoRuntime[i].bCreated) {
            bHasCreated = true;
        }
    }

    if (pHandle->stAudioRuntime.bStopRequested && pHandle->stAudioRuntime.bWorkerRunning) {
        return MEDIA_SERVICE_STATE_STOPPING;
    }

    if (bHasCreated || pHandle->stAudioRuntime.bCreated) {
        return MEDIA_SERVICE_STATE_RUNNING;
    }

    return MEDIA_SERVICE_STATE_STOPPED;
}

int getCameraOSMediaServiceConfig(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceVideoConfig pConfig)
{
    if (pHandle == NULL || pConfig == NULL) {
        return STATUS_INVALID_ARG;
    }

    *pConfig = pHandle->stVideoConfig;
    return STATUS_SUCCESS;
}

/******************************************************************************************
 * PRIVATE API
 ******************************************************************************************/

static int mediaServiceLock(PCameraOSMediaServiceHandle pHandle)
{
    if (pHandle == NULL) {
        return STATUS_INVALID_ARG;
    }

    return (OS_MutSemTake(pHandle->mutexId) == OS_SUCCESS) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}

static int mediaServiceUnlock(PCameraOSMediaServiceHandle pHandle)
{
    if (pHandle == NULL) {
        return STATUS_INVALID_ARG;
    }

    return (OS_MutSemGive(pHandle->mutexId) == OS_SUCCESS) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}

static const CameraOSHalOps* mediaServiceGetDefaultHalOps(void)
{
#if defined(CAMERAOS_HAL_VENDOR_RV1106)
    return getRV1106HalDefaultImpl();
#elif defined(CAMERAOS_HAL_VENDOR_RTS3917N)
    return getRTS3917NHalDefaultImpl();
#endif
}

static int mediaServiceHalSystemConfig(PCameraOSHalSystemConfig pSystemConfig)
{
    if (pSystemConfig == NULL) {
        return STATUS_INVALID_ARG;
    }

    memset(pSystemConfig, 0, sizeof(*pSystemConfig));
    pSystemConfig->u32CameraId = 0U;
    pSystemConfig->u32ViDevId = 0U;
    pSystemConfig->u32ViPipeId = 0U;
    pSystemConfig->bEnableIsp = false;
    pSystemConfig->pIqDir = NULL;

    return STATUS_SUCCESS;
}

static void mediaServiceVideoWorkerLoop(uint32_t u32Slot)
{
    int32_t s32Ret = STATUS_SUCCESS;
    CameraOSHalVideoInputFrame stRawFrame;
    CameraOSHalVideoEncodeFrame stEncodeFrame;
    CameraOSMediaServiceVideoRuntime* pRuntime = NULL;

    if (u32Slot >= gCameraOSMediaServiceHandle->stVideoConfig.u32PipelineCount) {
        DLOGE("Invalid video channel %d", u32Slot);
        OS_TaskExit();
        return;
    }

    pRuntime = &gCameraOSMediaServiceHandle->astVideoRuntime[u32Slot];
    pRuntime->bWorkerRunning = true;

    while (pRuntime->bStopRequested == false) {
        if (pRuntime->bStopRequested) {
            break;
        }

        /** Get video input frame and dispatch frame to application */
        if (gCameraOSMediaServiceHandle->stVideoConfig.abEnableVideoInputFrame[u32Slot]) {
            memset(&stRawFrame, 0, sizeof(stRawFrame));
            s32Ret = getCameraOSHalVideoInputFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle,
                                                   gCameraOSMediaServiceHandle->stVideoConfig.au32PipelineId[u32Slot], &stRawFrame,
                                                   MEDIA_SERVICE_FRAME_TIMEOUT_MS);
            if (s32Ret == HAL_ERR_NONE) {
                mediaServiceDispatchVideoRawFrame(gCameraOSMediaServiceHandle, u32Slot, &stRawFrame);
                releaseCameraOSHalVideoInputFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle,
                                                  gCameraOSMediaServiceHandle->stVideoConfig.au32PipelineId[u32Slot], &stRawFrame);
            }
        }

        /** Get video encode frame and dispatch frame to application */
        if (gCameraOSMediaServiceHandle->stVideoConfig.abEnableVideoEncodeFrame[u32Slot]) {
            memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
            s32Ret = getCameraOSHalVideoEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle,
                                                    gCameraOSMediaServiceHandle->stVideoConfig.au32PipelineId[u32Slot], &stEncodeFrame,
                                                    MEDIA_SERVICE_FRAME_TIMEOUT_MS);
            if (s32Ret == HAL_ERR_NONE) {
                mediaServiceDispatchVideoEncodeFrame(gCameraOSMediaServiceHandle, u32Slot, &stEncodeFrame);
                releaseCameraOSHalVideoEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle,
                                                   gCameraOSMediaServiceHandle->stVideoConfig.au32PipelineId[u32Slot], &stEncodeFrame);
            }
        }

        OS_TaskDelay(10U);
    }

    pRuntime->bWorkerRunning = false;
    OS_TaskExit();
}

static void mediaServiceAudioWorkerLoop(void)
{
    int32_t s32Ret = STATUS_SUCCESS;
    CameraOSHalAudioInputFrame stRawFrame;
    CameraOSHalAudioEncodeFrame stEncodeFrame;
    CameraOSMediaServiceAudioRuntime* pRuntime = NULL;

    if (gCameraOSMediaServiceHandle == NULL) {
        OS_TaskExit();
        return;
    }

    pRuntime = &gCameraOSMediaServiceHandle->stAudioRuntime;
    pRuntime->bWorkerRunning = true;

    while (!pRuntime->bStopRequested) {
        if (pRuntime->bStopRequested) {
            break;
        }

        if (gCameraOSMediaServiceHandle->stAudioConfig.bEnableInputFrame) {
            memset(&stRawFrame, 0, sizeof(stRawFrame));
            s32Ret = getCameraOSHalAudioInputFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &stRawFrame,
                                                   MEDIA_SERVICE_FRAME_TIMEOUT_MS);
            if (s32Ret == HAL_ERR_NONE) {
                mediaServiceDispatchAudioRawFrame(gCameraOSMediaServiceHandle, &stRawFrame);
                releaseCameraOSHalAudioInputFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &stRawFrame);
            }
        }

        if (gCameraOSMediaServiceHandle->stAudioConfig.bEnableEncodeFrame) {
            memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));
            s32Ret = getCameraOSHalAudioEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &stEncodeFrame,
                                                    MEDIA_SERVICE_FRAME_TIMEOUT_MS);
            if (s32Ret == HAL_ERR_NONE) {
                mediaServiceDispatchAudioEncodeFrame(gCameraOSMediaServiceHandle, &stEncodeFrame);
                releaseCameraOSHalAudioEncodeFrame(gCameraOSMediaServiceHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &stEncodeFrame);
            }
        }

        OS_TaskDelay(10U);
    }

    pRuntime->bWorkerRunning = false;
    OS_TaskExit();
}

static void taskMediaServiceVideoMainStream(void)
{
    mediaServiceVideoWorkerLoop(MEDIA_SERVICE_VIDEO_MAIN_STREAM_ID);
}

static void taskMediaServiceVideoSubStream(void)
{
    mediaServiceVideoWorkerLoop(MEDIA_SERVICE_VIDEO_SUB_STREAM_ID);
}

static void taskMediaServiceVideoAIStream(void)
{
    mediaServiceVideoWorkerLoop(MEDIA_SERVICE_VIDEO_AI_STREAM_ID);
}

static void taskMediaServiceAudioStream(void)
{
    mediaServiceAudioWorkerLoop();
}

static osal_task_entry mediaServiceGetVideoTaskEntry(uint32_t u32Slot)
{
    switch (u32Slot) {
        case MEDIA_SERVICE_VIDEO_MAIN_STREAM_ID:
            return taskMediaServiceVideoMainStream;
        case MEDIA_SERVICE_VIDEO_SUB_STREAM_ID:
            return taskMediaServiceVideoSubStream;
        case MEDIA_SERVICE_VIDEO_AI_STREAM_ID:
            return taskMediaServiceVideoAIStream;
        default:
            return NULL;
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

static void mediaServiceDispatchVideoRawFrame(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, PCameraOSHalVideoInputFrame pFrame)
{
    CameraOSMediaServiceFrameSink astSinkSnapshot[CAMERAOS_MEDIA_MAX_SINKS];
    CameraOSMediaServiceVideoRawFrameInfo stFrameInfo;
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

    memset(&stFrameInfo, 0x00, sizeof(stFrameInfo));
    stFrameInfo.u32PipelineId = pHandle->stVideoConfig.au32PipelineId[u32Slot];
    stFrameInfo.u32ViChnId = pHandle->stVideoConfig.au32PipelineId[u32Slot];
    stFrameInfo.pFrame = pFrame;

    for (u32Index = 0U; u32Index < u32SinkCount; ++u32Index) {
        if (astSinkSnapshot[u32Index].onVideoRawFrame != NULL) {
            astSinkSnapshot[u32Index].onVideoRawFrame(&stFrameInfo, astSinkSnapshot[u32Index].pUserData);
        }
    }
}

static void mediaServiceDispatchVideoEncodeFrame(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot, PCameraOSHalVideoEncodeFrame pFrame)
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

    memset(&stFrameInfo, 0x00, sizeof(stFrameInfo));
    stFrameInfo.u32PipelineId = pHandle->stVideoConfig.au32PipelineId[u32Slot];
    stFrameInfo.u32VencChnId = pHandle->stVideoConfig.au32PipelineId[u32Slot];
    stFrameInfo.pFrame = pFrame;

    for (u32Index = 0U; u32Index < u32SinkCount; ++u32Index) {
        if (astSinkSnapshot[u32Index].onVideoEncodeFrame != NULL) {
            astSinkSnapshot[u32Index].onVideoEncodeFrame(&stFrameInfo, astSinkSnapshot[u32Index].pUserData);
        }
    }
}

static void mediaServiceDispatchAudioRawFrame(PCameraOSMediaServiceHandle pHandle, PCameraOSHalAudioInputFrame pFrame)
{
    CameraOSMediaServiceFrameSink astSinkSnapshot[CAMERAOS_MEDIA_MAX_SINKS];
    CameraOSMediaServiceAudioRawFrameInfo stFrameInfo;
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

    memset(&stFrameInfo, 0x00, sizeof(stFrameInfo));
    stFrameInfo.u32AiChnId = MEDIA_SERVICE_AUDIO_CHANNEL_ID;
    stFrameInfo.pFrame = pFrame;

    for (u32Index = 0U; u32Index < u32SinkCount; ++u32Index) {
        if (astSinkSnapshot[u32Index].onAudioRawFrame != NULL) {
            astSinkSnapshot[u32Index].onAudioRawFrame(&stFrameInfo, astSinkSnapshot[u32Index].pUserData);
        }
    }
}

static void mediaServiceDispatchAudioEncodeFrame(PCameraOSMediaServiceHandle pHandle, PCameraOSHalAudioEncodeFrame pFrame)
{
    CameraOSMediaServiceFrameSink astSinkSnapshot[CAMERAOS_MEDIA_MAX_SINKS];
    CameraOSMediaServiceAudioEncodeFrameInfo stFrameInfo;
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

    memset(&stFrameInfo, 0x00, sizeof(stFrameInfo));
    stFrameInfo.u32AencChnId = MEDIA_SERVICE_AUDIO_CHANNEL_ID;
    stFrameInfo.pFrame = pFrame;

    for (u32Index = 0U; u32Index < u32SinkCount; ++u32Index) {
        if (astSinkSnapshot[u32Index].onAudioEncodeFrame != NULL) {
            astSinkSnapshot[u32Index].onAudioEncodeFrame(&stFrameInfo, astSinkSnapshot[u32Index].pUserData);
        }
    }
}

static int mediaServiceAudioCreateTask(PCameraOSMediaServiceHandle pHandle)
{
    int32_t s32Ret = STATUS_SUCCESS;
    char acTaskName[OS_MAX_API_NAME];

    memset(&acTaskName, 0x00, OS_MAX_API_NAME);

    SNPRINTF(acTaskName, sizeof(acTaskName), "media_audio");
    pHandle->stAudioRuntime.taskId = OS_OBJECT_ID_UNDEFINED;
    pHandle->stAudioRuntime.bStopRequested = false;
    pHandle->stAudioRuntime.bWorkerRunning = false;

    s32Ret = OS_TaskCreate(&pHandle->stAudioRuntime.taskId, acTaskName, taskMediaServiceAudioStream, OSAL_TASK_STACK_ALLOCATE,
                           MEDIA_SERVICE_TASK_STACK_SIZE, MEDIA_SERVICE_TASK_PRIORITY, 0U);
    if (s32Ret != OS_SUCCESS) {
        DLOGE("OS_TaskCreate failed audio status=%ld", OS_StatusToInteger(s32Ret));
        pHandle->stAudioRuntime.taskId = OS_OBJECT_ID_UNDEFINED;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static int mediaServiceVideoCreateTask(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    int32_t s32Ret = STATUS_SUCCESS;
    char acTaskName[OS_MAX_API_NAME];

    memset(&acTaskName, 0x00, OS_MAX_API_NAME);

    CameraOSMediaServiceVideoRuntime* pRuntime = &pHandle->astVideoRuntime[u32Slot];

    osal_task_entry entry = mediaServiceGetVideoTaskEntry(u32Slot);
    if (entry == NULL) {
        return STATUS_INVALID_ARG;
    }

    SNPRINTF(acTaskName, sizeof(acTaskName), "media%u", (unsigned int) pHandle->stVideoConfig.au32PipelineId[u32Slot]);
    pRuntime->taskId = OS_OBJECT_ID_UNDEFINED;
    pRuntime->bStopRequested = false;
    pRuntime->bWorkerRunning = false;

    s32Ret =
        OS_TaskCreate(&pRuntime->taskId, acTaskName, entry, OSAL_TASK_STACK_ALLOCATE, MEDIA_SERVICE_TASK_STACK_SIZE, MEDIA_SERVICE_TASK_PRIORITY, 0U);
    if (s32Ret != OS_SUCCESS) {
        DLOGE("OS_TaskCreate failed pipeline=%u status=%ld", pHandle->stVideoConfig.au32PipelineId[u32Slot], OS_StatusToInteger(s32Ret));
        pRuntime->taskId = OS_OBJECT_ID_UNDEFINED;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static void mediaServiceVideoDeleteTask(PCameraOSMediaServiceHandle pHandle, uint32_t u32Slot)
{
    CameraOSMediaServiceVideoRuntime* pRuntime = &pHandle->astVideoRuntime[u32Slot];
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

static void mediaServiceAudioDeleteTask(PCameraOSMediaServiceHandle pHandle)
{
    uint32_t waitCount = 0U;

    pHandle->stAudioRuntime.bStopRequested = true;

    while (pHandle->stAudioRuntime.bWorkerRunning && waitCount < 10U) {
        OS_TaskDelay(MEDIA_SERVICE_STOP_WAIT_MS);
        ++waitCount;
    }

    if (pHandle->stAudioRuntime.taskId != OS_OBJECT_ID_UNDEFINED) {
        if (pHandle->stAudioRuntime.bWorkerRunning) {
            OS_TaskDelete(pHandle->stAudioRuntime.taskId);
        }
        pHandle->stAudioRuntime.taskId = OS_OBJECT_ID_UNDEFINED;
    }

    pHandle->stAudioRuntime.bWorkerRunning = false;
}

static int startCameraOSMediaServiceAudio(PCameraOSMediaServiceHandle pHandle)
{
    int32_t s32Ret = STATUS_SUCCESS;

    ENTER();

    if (pHandle == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    s32Ret = mediaServiceLock(pHandle);
    if (s32Ret != STATUS_SUCCESS) {
        LEAVE();
        return s32Ret;
    }

    if (!pHandle->stAudioConfig.bEnable) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return STATUS_INVALID_OPERATION;
    }

    if (pHandle->stAudioRuntime.bCreated) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return STATUS_INVALID_OPERATION;
    }

    mediaServiceUnlock(pHandle);
    s32Ret = createCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &pHandle->stAudioConfig.stInputConfig);
    if (s32Ret != HAL_ERR_NONE) {
        LEAVE();
        return s32Ret;
    }

    s32Ret = createCameraOSHalAudioEncodeChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, &pHandle->stAudioConfig.stEncodeConfig);
    if (s32Ret != HAL_ERR_NONE) {
        destroyCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        LEAVE();
        return s32Ret;
    }

    s32Ret = bindCameraOSHalAudioInputToAudioEncode(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
    if (s32Ret != HAL_ERR_NONE) {
        destroyCameraOSHalAudioEncodeChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        destroyCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        LEAVE();
        return s32Ret;
    }

    pHandle->stAudioRuntime.bBound = true;

    s32Ret = mediaServiceAudioCreateTask(pHandle);
    if (s32Ret != STATUS_SUCCESS) {
        unbindCameraOSHalAudioEncodeFromAudioInput(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        destroyCameraOSHalAudioEncodeChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        destroyCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        pHandle->stAudioRuntime.bBound = false;
        LEAVE();
        return s32Ret;
    }

    pHandle->stAudioRuntime.bCreated = true;
    LEAVE();
    return STATUS_SUCCESS;
}

static int stopCameraOSMediaServiceAudio(PCameraOSMediaServiceHandle pHandle)
{
    ENTER();
    if (pHandle == NULL) {
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    if (!pHandle->stAudioRuntime.bCreated) {
        LEAVE();
        return STATUS_SUCCESS;
    }

    mediaServiceAudioDeleteTask(pHandle);
    if (pHandle->stAudioRuntime.bBound) {
        unbindCameraOSHalAudioEncodeFromAudioInput(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
        pHandle->stAudioRuntime.bBound = false;
    }
    destroyCameraOSHalAudioEncodeChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
    destroyCameraOSHalAudioInputChannel(pHandle->pCameraOSHalHandle, MEDIA_SERVICE_AUDIO_CHANNEL_ID);
    pHandle->stAudioRuntime.bCreated = false;
    LEAVE();
    return STATUS_SUCCESS;
}

static int startCameraOSMediaServiceVideo(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    int32_t s32Ret = STATUS_SUCCESS;
    uint32_t u32Slot = 0U;
    uint32_t u32Index = 0U;
    CameraOSMediaServiceVideoRuntime* pRuntime = NULL;

    ENTER();

    if (pHandle == NULL) {
        DLOGE("%d invalid arg", u32PipelineId);
        return STATUS_INVALID_ARG;
    }

    u32Slot = pHandle->stVideoConfig.u32PipelineCount;
    for (u32Index = 0U; u32Index < pHandle->stVideoConfig.u32PipelineCount; ++u32Index) {
        if (pHandle->stVideoConfig.au32PipelineId[u32Index] == u32PipelineId) {
            u32Slot = u32Index;
            break;
        }
    }

    if (u32Slot >= pHandle->stVideoConfig.u32PipelineCount) {
        DLOGE("%d not found", u32PipelineId);
        return STATUS_NOT_FOUND;
    }

    s32Ret = mediaServiceLock(pHandle);
    if (s32Ret != STATUS_SUCCESS) {
        DLOGE("Failed to lock media service for video %d", u32PipelineId);
        return s32Ret;
    }

    if (pHandle->stVideoConfig.abEnabled[u32Slot] == false) {
        mediaServiceUnlock(pHandle);
        DLOGE("%d not enable", u32PipelineId);
        return STATUS_INVALID_OPERATION;
    }

    if (pHandle->astVideoRuntime[u32Slot].bCreated) {
        mediaServiceUnlock(pHandle);
        DLOGE("%d already created", u32PipelineId);
        return STATUS_INVALID_OPERATION;
    }

    mediaServiceUnlock(pHandle);
    pRuntime = &pHandle->astVideoRuntime[u32Slot];

    s32Ret = createCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot],
                                                &pHandle->stVideoConfig.astVideoInputConfig[u32Slot]);
    if (s32Ret != STATUS_SUCCESS) {
        DLOGE("Create video input channel %d failed", u32PipelineId);
        return s32Ret;
    }
    pRuntime->bCreated = true;

    s32Ret = createCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot],
                                                 &pHandle->stVideoConfig.astVideoEncodeConfig[u32Slot]);
    if (s32Ret != STATUS_SUCCESS) {
        destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        pRuntime->bCreated = false;
        DLOGE("Create video encode channel %d failed", u32PipelineId);
        return s32Ret;
    }

    s32Ret = bindCameraOSHalVideoInputToVideoEncode(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot],
                                                    pHandle->stVideoConfig.au32PipelineId[u32Slot]);
    if (s32Ret != STATUS_SUCCESS) {
        destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        pRuntime->bCreated = false;
        DLOGE("Bind video input %d to video encode %d failed", u32PipelineId, u32PipelineId);
        return s32Ret;
    }
    pRuntime->bBound = true;

    s32Ret = mediaServiceVideoCreateTask(pHandle, u32Slot);
    if (s32Ret != STATUS_SUCCESS) {
        unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot],
                                                    pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        pRuntime->bBound = false;
        pRuntime->bCreated = false;
        DLOGE("Create video task %d failed", u32PipelineId);
        return s32Ret;
    }

    pRuntime->bStopRequested = false;
    LEAVE();
    return STATUS_SUCCESS;
}

static int stopCameraOSMediaServiceVideo(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId)
{
    uint32_t u32Slot;
    uint32_t i;

    ENTER();
    if (pHandle == NULL) {
        mediaServiceUnlock(pHandle);
        LEAVE();
        return STATUS_INVALID_ARG;
    }

    u32Slot = pHandle->stVideoConfig.u32PipelineCount;
    for (i = 0; i < pHandle->stVideoConfig.u32PipelineCount; ++i) {
        if (pHandle->stVideoConfig.au32PipelineId[i] == u32PipelineId) {
            u32Slot = i;
            break;
        }
    }

    if (u32Slot >= pHandle->stVideoConfig.u32PipelineCount) {
        LEAVE();
        return STATUS_NOT_FOUND;
    }

    if (!pHandle->astVideoRuntime[u32Slot].bCreated) {
        LEAVE();
        return STATUS_SUCCESS;
    }

    mediaServiceVideoDeleteTask(pHandle, u32Slot);
    if (pHandle->astVideoRuntime[u32Slot].bBound) {
        unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot],
                                                    pHandle->stVideoConfig.au32PipelineId[u32Slot]);
        pHandle->astVideoRuntime[u32Slot].bBound = false;
    }
    destroyCameraOSHalVideoEncodeChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
    destroyCameraOSHalVideoInputChannel(pHandle->pCameraOSHalHandle, pHandle->stVideoConfig.au32PipelineId[u32Slot]);
    pHandle->astVideoRuntime[u32Slot].bCreated = false;

    LEAVE();
    return STATUS_SUCCESS;
}

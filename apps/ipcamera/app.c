#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "osapi-common.h"

#include "cameraos/services/media/media_service.h"

#include "app.h"

#define IPCAMERA_PIPELINE_COUNT 2U

static volatile sig_atomic_t gStop = 0;
static PCameraOSMediaServiceHandle gPMediaService = NULL;
static CameraOSMediaServiceConfig gMediaServiceConfig = {0};
static bool gAbServiceMediaStarted[IPCAMERA_PIPELINE_COUNT] = {false};
static bool gBEnableDebugDump = false;
static FILE* gApVideoDumpFile[IPCAMERA_PIPELINE_COUNT] = {NULL};
static FILE* gPAudioDumpFile = NULL;

static void signalHandler(int sig)
{
    (void) sig;
    stopApp();
}

static int onVideoEncodeFrame(PCCameraOSMediaServiceEncodeFrameInfo pFrameInfo, void* pUserData)
{
    uint32_t u32Slot;

    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return STATUS_INVALID_ARG;
    }

    u32Slot = pFrameInfo->u32PipelineId;
    if (u32Slot < IPCAMERA_PIPELINE_COUNT && gApVideoDumpFile[u32Slot] != NULL) {
        fwrite(pFrameInfo->pFrame->data, 1U, pFrameInfo->pFrame->length, gApVideoDumpFile[u32Slot]);
        fflush(gApVideoDumpFile[u32Slot]);
    }

    printf("[venc=%u] len=%u pts=%lld key=%d nalu=0x%02x data=%p\n", pFrameInfo->u32VencChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->bKeyFrame ? 1 : 0, pFrameInfo->pFrame->u8NaluType,
           (const void*) pFrameInfo->pFrame->data);

    return STATUS_SUCCESS;
}

static int onAudioEncodeFrame(PCCameraOSMediaServiceAudioEncodeFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return STATUS_INVALID_ARG;
    }

    if (gPAudioDumpFile != NULL) {
        fwrite(pFrameInfo->pFrame->data, 1U, pFrameInfo->pFrame->length, gPAudioDumpFile);
        fflush(gPAudioDumpFile);
    }

    printf("[aenc=%u] len=%u pts=%lld codec=%d data=%p\n", pFrameInfo->u32AencChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, (int) pFrameInfo->pFrame->enCodecType, (const void*) pFrameInfo->pFrame->data);

    return STATUS_SUCCESS;
}

static int onVideoRawFrame(PCCameraOSMediaServiceVideoRawFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return STATUS_INVALID_ARG;
    }

    printf("[vraw=%u] len=%u pts=%lld width=%u height=%u fmt=%u data=%p\n", pFrameInfo->u32ViChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->u32Width, pFrameInfo->pFrame->u32Height, pFrameInfo->pFrame->u32PixelFormat,
           (const void*) pFrameInfo->pFrame->data);

    return STATUS_SUCCESS;
}

static int onAudioRawFrame(PCCameraOSMediaServiceAudioRawFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return STATUS_INVALID_ARG;
    }

    printf("[araw=%u] len=%u pts=%lld rate=%u ch=%u width=%u codec=%d data=%p\n", pFrameInfo->u32AiChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->u32SampleRate, pFrameInfo->pFrame->u32Channels,
           pFrameInfo->pFrame->u32BitWidth, (int) pFrameInfo->pFrame->enCodecType, (const void*) pFrameInfo->pFrame->data);

    return STATUS_SUCCESS;
}

static void loadDefaultConfig(void)
{
    gMediaServiceConfig.stVideoConfig.u32PipelineCount = IPCAMERA_PIPELINE_COUNT;

    gMediaServiceConfig.stAudioConfig.bEnable = true;
    gMediaServiceConfig.stAudioConfig.bEnableInputFrame = true;
    gMediaServiceConfig.stAudioConfig.bEnableEncodeFrame = true;
    gMediaServiceConfig.stAudioConfig.stInputConfig.u32SampleRate = 8000;
    gMediaServiceConfig.stAudioConfig.stInputConfig.u32Channels = 1;
    gMediaServiceConfig.stAudioConfig.stInputConfig.u32BitWidth = 16;
    gMediaServiceConfig.stAudioConfig.stEncodeConfig.enCodecType = HAL_AUDIO_CODEC_G711U;
    gMediaServiceConfig.stAudioConfig.stEncodeConfig.u32SampleRate = 8000;
    gMediaServiceConfig.stAudioConfig.stEncodeConfig.u32Channels = 1;
    gMediaServiceConfig.stAudioConfig.stEncodeConfig.u32BitWidth = 16;
    gMediaServiceConfig.stAudioConfig.stEncodeConfig.u32BitRate = 64000;

    gMediaServiceConfig.stVideoConfig.au32PipelineId[0] = 0;
    gMediaServiceConfig.stVideoConfig.aenPipelineType[0] = MEDIA_PIPELINE_TYPE_MAIN;
    gMediaServiceConfig.stVideoConfig.abEnabled[0] = true;
    gMediaServiceConfig.stVideoConfig.abEnableVideoInputFrame[0] = false; /** disable dispatch video raw to application */
    gMediaServiceConfig.stVideoConfig.abEnableVideoEncodeFrame[0] = true; /** enable dispatch video encode to application */
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32Width = 1920;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32Height = 1080;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32Fps = 25;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32Depth = 8;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32BufferCount = 2;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[0].u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].u32Width = 1920;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].u32Height = 1080;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].u32Fps = 25;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].u32Gop = 25;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].u32BitRate = 1024U * 1024U;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].enRotation = HAL_VIDEO_ROTATION_0;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].enMirror = HAL_VIDEO_MIRROR_NONE;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].enCodecType = HAL_VIDEO_CODEC_H265;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[0].enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H265_CBR;
    gMediaServiceConfig.stVideoConfig.abEnableOsd[0] = true;
    gMediaServiceConfig.stVideoConfig.abEnableOsdDateTime[0] = true;
    gMediaServiceConfig.stVideoConfig.abEnableOsdTitle[0] = true;
    gMediaServiceConfig.stVideoConfig.aenOsdDateFormat[0] = HAL_OSD_DATE_FORMAT_YYYY_MM_DD;
    SNPRINTF(gMediaServiceConfig.stVideoConfig.astOsdTitleConfig[0].acTitle, sizeof(gMediaServiceConfig.stVideoConfig.astOsdTitleConfig[0].acTitle),
             "CameraOS");
    gMediaServiceConfig.stVideoConfig.astOsdTimePosition[0].u32X = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdTimePosition[0].u32Y = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdDatePosition[0].u32X = 170U;
    gMediaServiceConfig.stVideoConfig.astOsdDatePosition[0].u32Y = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdTitlePosition[0].u32X = 340U;
    gMediaServiceConfig.stVideoConfig.astOsdTitlePosition[0].u32Y = 0U;

    gMediaServiceConfig.stVideoConfig.au32PipelineId[1] = 1;
    gMediaServiceConfig.stVideoConfig.aenPipelineType[1] = MEDIA_PIPELINE_TYPE_SUB;
    gMediaServiceConfig.stVideoConfig.abEnabled[1] = true;
    gMediaServiceConfig.stVideoConfig.abEnableVideoInputFrame[1] = false;
    gMediaServiceConfig.stVideoConfig.abEnableVideoEncodeFrame[1] = true;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32Width = 640;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32Height = 360;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32Fps = 10;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32Depth = 8;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32BufferCount = 2;
    gMediaServiceConfig.stVideoConfig.astVideoInputConfig[1].u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].u32Width = 640;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].u32Height = 360;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].u32Fps = 10;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].u32Gop = 10;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].u32BitRate = 512U * 1024U;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].enRotation = HAL_VIDEO_ROTATION_0;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].enMirror = HAL_VIDEO_MIRROR_NONE;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].enCodecType = HAL_VIDEO_CODEC_H265;
    gMediaServiceConfig.stVideoConfig.astVideoEncodeConfig[1].enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H265_CBR;
    gMediaServiceConfig.stVideoConfig.abEnableOsd[1] = true;
    gMediaServiceConfig.stVideoConfig.abEnableOsdDateTime[1] = true;
    gMediaServiceConfig.stVideoConfig.abEnableOsdTitle[1] = true;
    gMediaServiceConfig.stVideoConfig.aenOsdDateFormat[1] = HAL_OSD_DATE_FORMAT_YYYY_MM_DD;
    SNPRINTF(gMediaServiceConfig.stVideoConfig.astOsdTitleConfig[1].acTitle, sizeof(gMediaServiceConfig.stVideoConfig.astOsdTitleConfig[1].acTitle),
             "CameraOS");
    gMediaServiceConfig.stVideoConfig.astOsdTimePosition[1].u32X = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdTimePosition[1].u32Y = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdDatePosition[1].u32X = 170U;
    gMediaServiceConfig.stVideoConfig.astOsdDatePosition[1].u32Y = 0U;
    gMediaServiceConfig.stVideoConfig.astOsdTitlePosition[1].u32X = 340U;
    gMediaServiceConfig.stVideoConfig.astOsdTitlePosition[1].u32Y = 0U;
}

int initApp(void)
{
    int32_t s32Ret = STATUS_SUCCESS;
    uint32_t u32Index = 0U;
    CameraOSMediaServiceFrameSink stMediaServiceSink = {0};

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    gStop = 0;
    for (u32Index = 0U; u32Index < IPCAMERA_PIPELINE_COUNT; ++u32Index) {
        gAbServiceMediaStarted[u32Index] = false;
        gApVideoDumpFile[u32Index] = NULL;
    }
    gPAudioDumpFile = NULL;
    gPMediaService = NULL;
    gMediaServiceConfig = (CameraOSMediaServiceConfig) {0};

    loadDefaultConfig();

    if (gBEnableDebugDump) {
        gApVideoDumpFile[0] = fopen("main.h265", "wb");
        if (gApVideoDumpFile[0] == NULL) {
            return STATUS_OPEN_FILE_FAILED;
        }

        gApVideoDumpFile[1] = fopen("sub.h265", "wb");
        if (gApVideoDumpFile[1] == NULL) {
            deinitApp();
            return STATUS_OPEN_FILE_FAILED;
        }

        gPAudioDumpFile = fopen("audio.g711", "wb");
        if (gPAudioDumpFile == NULL) {
            deinitApp();
            return STATUS_OPEN_FILE_FAILED;
        }
    }

    s32Ret = createCameraOSMediaService(&gMediaServiceConfig, &gPMediaService);
    if (s32Ret != STATUS_SUCCESS) {
        DLOGE("Failed ret=%d\n", s32Ret);
        return s32Ret;
    }

    stMediaServiceSink.pName = "stdout-encoded";
    stMediaServiceSink.onVideoEncodeFrame = onVideoEncodeFrame;
    stMediaServiceSink.onVideoRawFrame = onVideoRawFrame;
    stMediaServiceSink.onAudioEncodeFrame = onAudioEncodeFrame;
    stMediaServiceSink.onAudioRawFrame = onAudioRawFrame;
    s32Ret = registerCameraOSMediaServiceSink(gPMediaService, &stMediaServiceSink);
    if (s32Ret != STATUS_SUCCESS) {
        DLOGE("Failed ret=%d\n", s32Ret);
        deinitApp();
        return s32Ret;
    }

    for (u32Index = 0; u32Index < IPCAMERA_PIPELINE_COUNT; ++u32Index) {
        s32Ret = startCameraOSMediaService(gPMediaService, gMediaServiceConfig.stVideoConfig.au32PipelineId[u32Index]);
        if (s32Ret != STATUS_SUCCESS) {
            DLOGE("Failed pipeline=%u ret=%d\n", gMediaServiceConfig.stVideoConfig.au32PipelineId[u32Index], s32Ret);
            deinitApp();
            return s32Ret;
        }
        gAbServiceMediaStarted[u32Index] = true;
    }

    return STATUS_SUCCESS;
}

void startApp(void)
{
    OS_IdleLoop();
}

void deinitApp(void)
{
    uint32_t u32Index = 0U;

    for (u32Index = IPCAMERA_PIPELINE_COUNT; u32Index > 0; --u32Index) {
        uint32_t u32Slot = u32Index - 1U;

        if (gPMediaService != NULL && gAbServiceMediaStarted[u32Slot]) {
            stopCameraOSMediaService(gPMediaService, gMediaServiceConfig.stVideoConfig.au32PipelineId[u32Slot]);
            gAbServiceMediaStarted[u32Slot] = false;
        }
    }

    if (gPMediaService != NULL) {
        unregisterCameraOSMediaServiceSink(gPMediaService, "stdout-encoded");
        destroyCameraOSMediaService(gPMediaService);
        gPMediaService = NULL;
    }

    if (gPAudioDumpFile != NULL) {
        fclose(gPAudioDumpFile);
        gPAudioDumpFile = NULL;
    }

    for (u32Index = 0U; u32Index < IPCAMERA_PIPELINE_COUNT; ++u32Index) {
        if (gApVideoDumpFile[u32Index] != NULL) {
            fclose(gApVideoDumpFile[u32Index]);
            gApVideoDumpFile[u32Index] = NULL;
        }
    }
}

void stopApp(void)
{
    gStop = 1;
    OS_ApplicationShutdown(true);
}

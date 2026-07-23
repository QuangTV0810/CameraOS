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
static bool gAbStarted[IPCAMERA_PIPELINE_COUNT] = {false};
static CameraOSMediaServiceConfig gMediaConfig = {0};

static void signalHandler(int sig)
{
    (void) sig;
    stopApp();
}

static int onVideoEncodeFrame(PCCameraOSMediaServiceEncodeFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return -1;
    }

    printf("[venc=%u] len=%u pts=%lld key=%d nalu=0x%02x data=%p\n", pFrameInfo->u32VencChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->bKeyFrame ? 1 : 0, pFrameInfo->pFrame->u8NaluType,
           (const void*) pFrameInfo->pFrame->data);

    return 0;
}

static int onAudioEncodeFrame(PCCameraOSMediaServiceAudioEncodeFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return -1;
    }

    printf("[aenc=%u] len=%u pts=%lld codec=%d data=%p\n", pFrameInfo->u32AencChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, (int) pFrameInfo->pFrame->enCodecType, (const void*) pFrameInfo->pFrame->data);

    return 0;
}

static int onVideoRawFrame(PCCameraOSMediaServiceVideoRawFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return -1;
    }

    printf("[vraw=%u] len=%u pts=%lld width=%u height=%u fmt=%u data=%p\n", pFrameInfo->u32ViChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->u32Width, pFrameInfo->pFrame->u32Height, pFrameInfo->pFrame->u32PixelFormat,
           (const void*) pFrameInfo->pFrame->data);

    return 0;
}

static int onAudioRawFrame(PCCameraOSMediaServiceAudioRawFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return -1;
    }

    printf("[araw=%u] len=%u pts=%lld rate=%u ch=%u width=%u codec=%d data=%p\n", pFrameInfo->u32AiChnId, pFrameInfo->pFrame->length,
           (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->u32SampleRate, pFrameInfo->pFrame->u32Channels,
           pFrameInfo->pFrame->u32BitWidth, (int) pFrameInfo->pFrame->enCodecType, (const void*) pFrameInfo->pFrame->data);

    return 0;
}

static void loadDefaultConfig(void)
{
    gMediaConfig.stVideoConfig.u32PipelineCount = IPCAMERA_PIPELINE_COUNT;

    gMediaConfig.stAudioConfig.bEnable = true;
    gMediaConfig.stAudioConfig.bEnableInputFrame = true;
    gMediaConfig.stAudioConfig.bEnableEncodeFrame = true;
    gMediaConfig.stAudioConfig.stInputConfig.u32SampleRate = 8000;
    gMediaConfig.stAudioConfig.stInputConfig.u32Channels = 1;
    gMediaConfig.stAudioConfig.stInputConfig.u32BitWidth = 16;
    gMediaConfig.stAudioConfig.stEncodeConfig.enCodecType = HAL_AUDIO_CODEC_G711U;
    gMediaConfig.stAudioConfig.stEncodeConfig.u32SampleRate = 8000;
    gMediaConfig.stAudioConfig.stEncodeConfig.u32Channels = 1;
    gMediaConfig.stAudioConfig.stEncodeConfig.u32BitWidth = 16;
    gMediaConfig.stAudioConfig.stEncodeConfig.u32BitRate = 64000;

    gMediaConfig.stVideoConfig.au32PipelineId[0] = 0;
    gMediaConfig.stVideoConfig.aenPipelineType[0] = MEDIA_PIPELINE_TYPE_MAIN;
    gMediaConfig.stVideoConfig.abEnabled[0] = true;
    gMediaConfig.stVideoConfig.abEnableVideoInputFrame[0] = false;
    gMediaConfig.stVideoConfig.abEnableVideoEncodeFrame[0] = true;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32Width = 1920;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32Height = 1080;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32Fps = 25;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32Depth = 8;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32BufferCount = 2;
    gMediaConfig.stVideoConfig.astVideoInputConfig[0].u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].u32Width = 1920;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].u32Height = 1080;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].u32Fps = 25;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].u32Gop = 25;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].u32BitRate = 1024U * 1024U;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].enRotation = HAL_VIDEO_ROTATION_0;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].enMirror = HAL_VIDEO_MIRROR_NONE;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].enCodecType = HAL_VIDEO_CODEC_H264;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[0].enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;

    gMediaConfig.stVideoConfig.au32PipelineId[1] = 1;
    gMediaConfig.stVideoConfig.aenPipelineType[1] = MEDIA_PIPELINE_TYPE_SUB;
    gMediaConfig.stVideoConfig.abEnabled[1] = true;
    gMediaConfig.stVideoConfig.abEnableVideoInputFrame[1] = false;
    gMediaConfig.stVideoConfig.abEnableVideoEncodeFrame[1] = true;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32Width = 640;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32Height = 360;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32Fps = 10;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32Depth = 8;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32BufferCount = 2;
    gMediaConfig.stVideoConfig.astVideoInputConfig[1].u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].u32Width = 640;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].u32Height = 360;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].u32Fps = 10;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].u32Gop = 10;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].u32BitRate = 512U * 1024U;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].enRotation = HAL_VIDEO_ROTATION_0;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].enMirror = HAL_VIDEO_MIRROR_NONE;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].enCodecType = HAL_VIDEO_CODEC_H264;
    gMediaConfig.stVideoConfig.astVideoEncodeConfig[1].enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;
}

int initApp(void)
{
    int retStatus;
    uint32_t i;
    CameraOSMediaServiceFrameSink stSink = {0};

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    gStop = 0;
    for (i = 0U; i < IPCAMERA_PIPELINE_COUNT; ++i) {
        gAbStarted[i] = false;
    }
    gPMediaService = NULL;
    gMediaConfig = (CameraOSMediaServiceConfig) {0};
    loadDefaultConfig();

    retStatus = createCameraOSMediaService(&gMediaConfig, &gPMediaService);
    if (retStatus != STATUS_SUCCESS) {
        fprintf(stderr, "createCameraOSMediaService failed ret=%d\n", retStatus);
        return retStatus;
    }

    stSink.pName = "stdout-encoded";
    stSink.onVideoEncodeFrame = onVideoEncodeFrame;
    stSink.onVideoRawFrame = onVideoRawFrame;
    stSink.onAudioEncodeFrame = onAudioEncodeFrame;
    stSink.onAudioRawFrame = onAudioRawFrame;
    retStatus = registerCameraOSMediaServiceSink(gPMediaService, &stSink);
    if (retStatus != STATUS_SUCCESS) {
        fprintf(stderr, "registerCameraOSMediaServiceSink failed ret=%d\n", retStatus);
        deinitApp();
        return retStatus;
    }

    for (i = 0; i < IPCAMERA_PIPELINE_COUNT; ++i) {
        retStatus = startCameraOSMediaService(gPMediaService, gMediaConfig.stVideoConfig.au32PipelineId[i]);
        if (retStatus != STATUS_SUCCESS) {
            fprintf(stderr, "startCameraOSMediaService failed pipeline=%u ret=%d\n", gMediaConfig.stVideoConfig.au32PipelineId[i], retStatus);
            deinitApp();
            return retStatus;
        }
        gAbStarted[i] = true;
    }

    return STATUS_SUCCESS;
}

void startApp(void)
{
    OS_IdleLoop();
}

void deinitApp(void)
{
    uint32_t i;

    for (i = IPCAMERA_PIPELINE_COUNT; i > 0; --i) {
        uint32_t u32Slot = i - 1U;

        if (gPMediaService != NULL && gAbStarted[u32Slot]) {
            stopCameraOSMediaService(gPMediaService, gMediaConfig.stVideoConfig.au32PipelineId[u32Slot]);
            gAbStarted[u32Slot] = false;
        }
    }

    if (gPMediaService != NULL) {
        unregisterCameraOSMediaServiceSink(gPMediaService, "stdout-encoded");
        destroyCameraOSMediaService(gPMediaService);
        gPMediaService = NULL;
    }
}

void stopApp(void)
{
    gStop = 1;
    OS_ApplicationShutdown(true);
}

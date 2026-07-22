#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cameraos/services/media/media_service.h"

#define IPCAMERA_PIPELINE_COUNT 2U

static volatile sig_atomic_t gStop = 0;

static void signalHandler(int sig)
{
    (void) sig;
    gStop = 1;
}

static int onVideoEncodeFrame(PCCameraOSMediaServiceEncodeFrameInfo pFrameInfo, void* pUserData)
{
    (void) pUserData;

    if (pFrameInfo == NULL || pFrameInfo->pFrame == NULL) {
        return -1;
    }

    printf("[pipeline=%u venc=%u] len=%u pts=%lld key=%d nalu=0x%02x data=%p\n", pFrameInfo->u32PipelineId, pFrameInfo->u32VencChnId,
           pFrameInfo->pFrame->length, (long long) pFrameInfo->pFrame->timestamp, pFrameInfo->pFrame->bKeyFrame ? 1 : 0,
           pFrameInfo->pFrame->u8NaluType, (const void*) pFrameInfo->pFrame->data);

    return 0;
}

int main(void)
{
    int retStatus;
    int32 osStatus;
    uint32_t i;
    bool abStarted[IPCAMERA_PIPELINE_COUNT] = {false};
    PCameraOSMediaServiceHandle pMediaService = NULL;
    CameraOSMediaServiceFrameSink stSink = {0};
    CameraOSMediaServiceConfig stMediaConfig = {0};

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    osStatus = OS_API_Init();
    if (osStatus != OS_SUCCESS) {
        fprintf(stderr, "OS_API_Init failed status=%ld\n", OS_StatusToInteger(osStatus));
        return -1;
    }

    stMediaConfig.u32PipelineCount = IPCAMERA_PIPELINE_COUNT;

    stMediaConfig.astPipelineConfig[0].u32PipelineId = 0;
    stMediaConfig.astPipelineConfig[0].enPipelineType = MEDIA_PIPELINE_TYPE_MAIN;
    stMediaConfig.astPipelineConfig[0].bEnabled = true;
    stMediaConfig.astPipelineConfig[0].bEnableVideoInputFrame = false;
    stMediaConfig.astPipelineConfig[0].bEnableVideoEncodeFrame = true;
    stMediaConfig.astPipelineConfig[0].bWriteEncodeToFile = false;
    stMediaConfig.astPipelineConfig[0].pEncodeOutputFilePath = "./pipeline_0.h264";
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32Width = 1920;
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32Height = 1080;
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32Fps = 25;
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32Depth = 8;
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32BufferCount = 2;
    stMediaConfig.astPipelineConfig[0].stVideoInputConfig.u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.u32Width = 1920;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.u32Height = 1080;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.u32Fps = 25;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.u32Gop = 25;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.u32BitRate = 1024U * 1024U;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.enRotation = HAL_VIDEO_ROTATION_0;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.enMirror = HAL_VIDEO_MIRROR_NONE;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.enCodecType = HAL_VIDEO_CODEC_H264;
    stMediaConfig.astPipelineConfig[0].stVideoEncodeConfig.enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;

    stMediaConfig.astPipelineConfig[1].u32PipelineId = 1;
    stMediaConfig.astPipelineConfig[1].enPipelineType = MEDIA_PIPELINE_TYPE_SUB;
    stMediaConfig.astPipelineConfig[1].bEnabled = true;
    stMediaConfig.astPipelineConfig[1].bEnableVideoInputFrame = false;
    stMediaConfig.astPipelineConfig[1].bEnableVideoEncodeFrame = true;
    stMediaConfig.astPipelineConfig[1].bWriteEncodeToFile = false;
    stMediaConfig.astPipelineConfig[1].pEncodeOutputFilePath = "./pipeline_1.h264";
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32Width = 640;
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32Height = 360;
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32Fps = 10;
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32Depth = 8;
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32BufferCount = 2;
    stMediaConfig.astPipelineConfig[1].stVideoInputConfig.u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.u32Width = 640;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.u32Height = 360;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.u32Fps = 10;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.u32Gop = 10;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.u32BitRate = 512U * 1024U;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.enRotation = HAL_VIDEO_ROTATION_0;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.enMirror = HAL_VIDEO_MIRROR_NONE;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.enCodecType = HAL_VIDEO_CODEC_H264;
    stMediaConfig.astPipelineConfig[1].stVideoEncodeConfig.enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;

    retStatus = createCameraOSMediaService(&stMediaConfig, &pMediaService);
    if (retStatus != HAL_ERR_NONE) {
        fprintf(stderr, "createCameraOSMediaService failed ret=%d\n", retStatus);
        goto cleanup;
    }

    stSink.pName = "stdout-encoded";
    stSink.bEnableEncodedFrame = true;
    stSink.bEnableRawFrame = false;
    stSink.onEncodedFrame = onVideoEncodeFrame;
    retStatus = registerCameraOSMediaServiceSink(pMediaService, &stSink);
    if (retStatus != HAL_ERR_NONE) {
        fprintf(stderr, "registerCameraOSMediaServiceSink failed ret=%d\n", retStatus);
        goto cleanup;
    }

    for (i = 0; i < IPCAMERA_PIPELINE_COUNT; ++i) {
        retStatus = startCameraOSMediaServicePipeline(pMediaService, stMediaConfig.astPipelineConfig[i].u32PipelineId);
        if (retStatus != HAL_ERR_NONE) {
            fprintf(stderr, "startCameraOSMediaServicePipeline failed pipeline=%u ret=%d\n", stMediaConfig.astPipelineConfig[i].u32PipelineId,
                    retStatus);
            goto cleanup;
        }
        abStarted[i] = true;
    }

    while (!gStop) {
        sleep(1);
    }

cleanup:
    for (i = IPCAMERA_PIPELINE_COUNT; i > 0; --i) {
        uint32_t u32Slot = i - 1U;

        if (pMediaService != NULL && abStarted[u32Slot]) {
            stopCameraOSMediaServicePipeline(pMediaService, stMediaConfig.astPipelineConfig[u32Slot].u32PipelineId);
            abStarted[u32Slot] = false;
        }
    }

    if (pMediaService != NULL) {
        unregisterCameraOSMediaServiceSink(pMediaService, "stdout-encoded");
        destroyCameraOSMediaService(pMediaService);
    }

    OS_API_Teardown();
    return retStatus;
}

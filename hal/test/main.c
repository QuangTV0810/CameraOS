#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cameraos/hal/hal.h"
#if defined(CAMERAOS_HAL_VENDOR_RV1106)
#include "rv1106_hal.h"
#elif defined(CAMERAOS_HAL_VENDOR_RTS3917N)
#include "rts3917n_hal.h"
#else
#error "Unsupported HAL vendor"
#endif

#define HAL_TEST_PIPELINE_COUNT   2U
#define HAL_TEST_AUDIO_CHANNEL_ID 0U

typedef struct __HalTestPipelineConfig HalTestPipelineConfig;
struct __HalTestPipelineConfig {
    uint32_t u32ViChnId;
    uint32_t u32VencChnId;
    bool bWriteToFile;
    const char* pOutputFilePath;
    FILE* pOutputFile;
    CameraOSHalVideoInputConfig stViConfig;
    CameraOSHalVideoEncodeConfig stVencConfig;
};

typedef struct __HalTestAudioConfig HalTestAudioConfig;
struct __HalTestAudioConfig {
    uint32_t u32AiChnId;
    uint32_t u32AencChnId;
    bool bWriteRawToFile;
    bool bWriteEncodeToFile;
    const char* pRawOutputFilePath;
    const char* pEncodeOutputFilePath;
    FILE* pRawOutputFile;
    FILE* pEncodeOutputFile;
    CameraOSHalAudioInputConfig stAiConfig;
    CameraOSHalAudioEncodeConfig stAencConfig;
};

static volatile sig_atomic_t gStop = 0;

static void appSignalHandler(int sig)
{
    (void) sig;
    gStop = 1;
}

int main(void)
{
    int retStatus = 0;
    CameraOSHalSystemConfig stSystemConfig = {0};
    CameraOSHalVideoEncodeFrame astFrame[HAL_TEST_PIPELINE_COUNT] = {0};
    CameraOSHalAudioInputFrame stAudioRawFrame = {0};
    CameraOSHalAudioEncodeFrame stAudioFrame = {0};
    HalTestPipelineConfig astPipeline[HAL_TEST_PIPELINE_COUNT] = {0};
    HalTestAudioConfig stAudio = {0};
    const CameraOSHalOps* pOps = NULL;
    PCameraOSHalHandle pHandle = NULL;
    bool abViCreated[HAL_TEST_PIPELINE_COUNT] = {false};
    bool abVencCreated[HAL_TEST_PIPELINE_COUNT] = {false};
    bool abBound[HAL_TEST_PIPELINE_COUNT] = {false};
    bool abFrameAcquired[HAL_TEST_PIPELINE_COUNT] = {false};
    bool bAiCreated = false;
    bool bAencCreated = false;
    bool bAudioBound = false;
    bool bAudioRawFrameAcquired = false;
    bool bAudioFrameAcquired = false;
    uint32_t i;

    ENTER();
    signal(SIGINT, appSignalHandler);
    signal(SIGTERM, appSignalHandler);

    stSystemConfig.u32CameraId = 0;
    stSystemConfig.u32ViDevId = 0;
    stSystemConfig.u32ViPipeId = 0;
    stSystemConfig.bEnableIsp = false;
    stSystemConfig.pIqDir = NULL;

    astPipeline[0].u32ViChnId = 0;
    astPipeline[0].u32VencChnId = 0;
    astPipeline[0].bWriteToFile = false;
    astPipeline[0].pOutputFilePath = "./pipeline_0.h264";
    astPipeline[0].stViConfig.u32Width = 1920;
    astPipeline[0].stViConfig.u32Height = 1080;
    astPipeline[0].stViConfig.u32Fps = 25;
    astPipeline[0].stViConfig.u32Depth = 0;
    astPipeline[0].stViConfig.u32BufferCount = 2;
    astPipeline[0].stViConfig.u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    astPipeline[0].stVencConfig.u32Width = 1920;
    astPipeline[0].stVencConfig.u32Height = 1080;
    astPipeline[0].stVencConfig.u32Fps = 25;
    astPipeline[0].stVencConfig.u32Gop = 25;
    astPipeline[0].stVencConfig.u32BitRate = 1 * 1024 * 1024;
    astPipeline[0].stVencConfig.enRotation = HAL_VIDEO_ROTATION_0;
    astPipeline[0].stVencConfig.enMirror = HAL_VIDEO_MIRROR_NONE;
    astPipeline[0].stVencConfig.enCodecType = HAL_VIDEO_CODEC_H264;
    astPipeline[0].stVencConfig.enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;

    astPipeline[1].u32ViChnId = 1;
    astPipeline[1].u32VencChnId = 1;
    astPipeline[1].bWriteToFile = false;
    astPipeline[1].pOutputFilePath = "./pipeline_1.h264";
    astPipeline[1].stViConfig.u32Width = 640;
    astPipeline[1].stViConfig.u32Height = 360;
    astPipeline[1].stViConfig.u32Fps = 25;
    astPipeline[1].stViConfig.u32Depth = 0;
    astPipeline[1].stViConfig.u32BufferCount = 2;
    astPipeline[1].stViConfig.u32PixelFormat = HAL_VIDEO_FORMAT_NV12;
    astPipeline[1].stVencConfig.u32Width = 640;
    astPipeline[1].stVencConfig.u32Height = 360;
    astPipeline[1].stVencConfig.u32Fps = 25;
    astPipeline[1].stVencConfig.u32Gop = 25;
    astPipeline[1].stVencConfig.u32BitRate = 512 * 1024;
    astPipeline[1].stVencConfig.enRotation = HAL_VIDEO_ROTATION_0;
    astPipeline[1].stVencConfig.enMirror = HAL_VIDEO_MIRROR_NONE;
    astPipeline[1].stVencConfig.enCodecType = HAL_VIDEO_CODEC_H264;
    astPipeline[1].stVencConfig.enRcMode = HAL_VIDEO_ENCODE_RC_MODE_H264_CBR;

    stAudio.u32AiChnId = HAL_TEST_AUDIO_CHANNEL_ID;
    stAudio.u32AencChnId = HAL_TEST_AUDIO_CHANNEL_ID;
    stAudio.bWriteRawToFile = false;
    stAudio.bWriteEncodeToFile = false;
    stAudio.pRawOutputFilePath = "./audio_0.pcm";
    stAudio.pEncodeOutputFilePath = "./audio_0.g711u";
    stAudio.stAiConfig.u32SampleRate = 8000;
    stAudio.stAiConfig.u32Channels = 1;
    stAudio.stAiConfig.u32BitWidth = 16;
    stAudio.stAencConfig.enCodecType = HAL_AUDIO_CODEC_G711U;
    stAudio.stAencConfig.u32SampleRate = 8000;
    stAudio.stAencConfig.u32Channels = 1;
    stAudio.stAencConfig.u32BitWidth = 16;
    stAudio.stAencConfig.u32BitRate = 64000;

#if defined(CAMERAOS_HAL_VENDOR_RV1106)
    pOps = getRV1106HalDefaultImpl();
#elif defined(CAMERAOS_HAL_VENDOR_RTS3917N)
    pOps = getRTS3917NHalDefaultImpl();
#endif

    retStatus = createCameraOSHal(&stSystemConfig, (PCameraOSHalOps) pOps, &pHandle);
    if (retStatus != 0) {
        goto cleanup;
    }

    for (i = 0; i < HAL_TEST_PIPELINE_COUNT && retStatus == 0; ++i) {
        if (astPipeline[i].bWriteToFile) {
            astPipeline[i].pOutputFile = fopen(astPipeline[i].pOutputFilePath, "wb");
            if (astPipeline[i].pOutputFile == NULL) {
                fprintf(stderr, "failed to open output file: %s\n", astPipeline[i].pOutputFilePath);
                retStatus = -1;
                break;
            }
        }
    }
    if (retStatus == 0 && stAudio.bWriteRawToFile) {
        stAudio.pRawOutputFile = fopen(stAudio.pRawOutputFilePath, "wb");
        if (stAudio.pRawOutputFile == NULL) {
            fprintf(stderr, "failed to open output file: %s\n", stAudio.pRawOutputFilePath);
            retStatus = -1;
        }
    }
    if (retStatus == 0 && stAudio.bWriteEncodeToFile) {
        stAudio.pEncodeOutputFile = fopen(stAudio.pEncodeOutputFilePath, "wb");
        if (stAudio.pEncodeOutputFile == NULL) {
            fprintf(stderr, "failed to open output file: %s\n", stAudio.pEncodeOutputFilePath);
            retStatus = -1;
        }
    }

    for (i = 0; i < HAL_TEST_PIPELINE_COUNT && retStatus == 0; ++i) {
        retStatus = createCameraOSHalVideoInputChannel(pHandle, astPipeline[i].u32ViChnId, &astPipeline[i].stViConfig);
        if (retStatus == 0) {
            abViCreated[i] = true;
        }
    }

    for (i = 0; i < HAL_TEST_PIPELINE_COUNT && retStatus == 0; ++i) {
        retStatus = createCameraOSHalVideoEncodeChannel(pHandle, astPipeline[i].u32VencChnId, &astPipeline[i].stVencConfig);
        if (retStatus == 0) {
            abVencCreated[i] = true;
        }
    }

    for (i = 0; i < HAL_TEST_PIPELINE_COUNT && retStatus == 0; ++i) {
        retStatus = bindCameraOSHalVideoInputToVideoEncode(pHandle, astPipeline[i].u32ViChnId, astPipeline[i].u32VencChnId);
        if (retStatus == 0) {
            abBound[i] = true;
        }
    }

    if (retStatus == 0) {
        retStatus = createCameraOSHalAudioInputChannel(pHandle, stAudio.u32AiChnId, &stAudio.stAiConfig);
        if (retStatus == 0) {
            bAiCreated = true;
        }
    }
    if (retStatus == 0) {
        retStatus = createCameraOSHalAudioEncodeChannel(pHandle, stAudio.u32AencChnId, &stAudio.stAencConfig);
        if (retStatus == 0) {
            bAencCreated = true;
        }
    }
    if (retStatus == 0) {
        retStatus = bindCameraOSHalAudioInputToAudioEncode(pHandle, stAudio.u32AiChnId, stAudio.u32AencChnId);
        if (retStatus == 0) {
            bAudioBound = true;
        }
    }

    while (retStatus == 0 && !gStop) {
        for (i = 0; i < HAL_TEST_PIPELINE_COUNT && retStatus == 0; ++i) {
            memset(&astFrame[i], 0, sizeof(astFrame[i]));
            retStatus = getCameraOSHalVideoEncodeFrame(pHandle, astPipeline[i].u32VencChnId, &astFrame[i], 2000);
            if (retStatus != 0) {
                break;
            }

            abFrameAcquired[i] = true;
            printf("[venc-%u]: len=%u pts=%lld key=%d nalu=0x%02x data=%p\n", astPipeline[i].u32VencChnId, astFrame[i].length,
                   (long long) astFrame[i].timestamp, astFrame[i].bKeyFrame ? 1 : 0, astFrame[i].u8NaluType, (const void*) astFrame[i].data);
            if (astPipeline[i].pOutputFile != NULL && astFrame[i].data != NULL && astFrame[i].length > 0U) {
                if (fwrite(astFrame[i].data, 1, astFrame[i].length, astPipeline[i].pOutputFile) != astFrame[i].length) {
                    fprintf(stderr, "failed to write output file: %s\n", astPipeline[i].pOutputFilePath);
                    retStatus = -1;
                } else {
                    fflush(astPipeline[i].pOutputFile);
                }
            }
            if (retStatus != 0) {
                break;
            }

            retStatus = releaseCameraOSHalVideoEncodeFrame(pHandle, astPipeline[i].u32VencChnId, &astFrame[i]);
            if (retStatus != 0) {
                break;
            }

            abFrameAcquired[i] = false;
        }

        if (retStatus != 0) {
            break;
        }

        memset(&stAudioRawFrame, 0, sizeof(stAudioRawFrame));
        retStatus = getCameraOSHalAudioInputFrame(pHandle, stAudio.u32AiChnId, &stAudioRawFrame, 100);
        if (retStatus != 0) {
            printf("failed to get audio input frame: %d\n", retStatus);
            break;
        }

        bAudioRawFrameAcquired = true;
        printf("[araw-%u]: len=%u pts=%lld rate=%u ch=%u width=%u codec=%d data=%p\n", stAudio.u32AiChnId, stAudioRawFrame.length,
               (long long) stAudioRawFrame.timestamp, stAudioRawFrame.u32SampleRate, stAudioRawFrame.u32Channels, stAudioRawFrame.u32BitWidth,
               (int) stAudioRawFrame.enCodecType, (const void*) stAudioRawFrame.data);
        if (stAudio.pRawOutputFile != NULL && stAudioRawFrame.data != NULL && stAudioRawFrame.length > 0U) {
            if (fwrite(stAudioRawFrame.data, 1, stAudioRawFrame.length, stAudio.pRawOutputFile) != stAudioRawFrame.length) {
                fprintf(stderr, "failed to write output file: %s\n", stAudio.pRawOutputFilePath);
                retStatus = -1;
            } else {
                fflush(stAudio.pRawOutputFile);
            }
        }
        if (retStatus != 0) {
            break;
        }

        retStatus = releaseCameraOSHalAudioInputFrame(pHandle, stAudio.u32AiChnId, &stAudioRawFrame);
        if (retStatus != 0) {
            break;
        }

        bAudioRawFrameAcquired = false;

        memset(&stAudioFrame, 0, sizeof(stAudioFrame));
        retStatus = getCameraOSHalAudioEncodeFrame(pHandle, stAudio.u32AencChnId, &stAudioFrame, 100);
        if (retStatus != 0) {
            printf("failed to get audio encode frame: %d\n", retStatus);
            break;
        }

        bAudioFrameAcquired = true;
        printf("[aenc-%u]: len=%u pts=%lld codec=%d data=%p\n", stAudio.u32AencChnId, stAudioFrame.length, (long long) stAudioFrame.timestamp,
               (int) stAudioFrame.enCodecType, (const void*) stAudioFrame.data);
        if (stAudio.pEncodeOutputFile != NULL && stAudioFrame.data != NULL && stAudioFrame.length > 0U) {
            if (fwrite(stAudioFrame.data, 1, stAudioFrame.length, stAudio.pEncodeOutputFile) != stAudioFrame.length) {
                fprintf(stderr, "failed to write output file: %s\n", stAudio.pEncodeOutputFilePath);
                retStatus = -1;
            } else {
                fflush(stAudio.pEncodeOutputFile);
            }
        }
        if (retStatus != 0) {
            break;
        }

        retStatus = releaseCameraOSHalAudioEncodeFrame(pHandle, stAudio.u32AencChnId, &stAudioFrame);
        if (retStatus != 0) {
            break;
        }

        bAudioFrameAcquired = false;
    }

cleanup:
    if (bAudioRawFrameAcquired) {
        releaseCameraOSHalAudioInputFrame(pHandle, stAudio.u32AiChnId, &stAudioRawFrame);
        bAudioRawFrameAcquired = false;
    }

    if (bAudioFrameAcquired) {
        releaseCameraOSHalAudioEncodeFrame(pHandle, stAudio.u32AencChnId, &stAudioFrame);
        bAudioFrameAcquired = false;
    }

    for (i = 0; i < HAL_TEST_PIPELINE_COUNT; ++i) {
        if (abFrameAcquired[i]) {
            releaseCameraOSHalVideoEncodeFrame(pHandle, astPipeline[i].u32VencChnId, &astFrame[i]);
            abFrameAcquired[i] = false;
        }
    }

    for (i = HAL_TEST_PIPELINE_COUNT; i > 0; --i) {
        uint32_t idx = i - 1U;

        if (abBound[idx]) {
            unbindCameraOSHalVideoEncodeFromeVideoInput(pHandle, astPipeline[idx].u32ViChnId, astPipeline[idx].u32VencChnId);
        }
        if (abVencCreated[idx]) {
            destroyCameraOSHalVideoEncodeChannel(pHandle, astPipeline[idx].u32VencChnId);
        }
        if (abViCreated[idx]) {
            destroyCameraOSHalVideoInputChannel(pHandle, astPipeline[idx].u32ViChnId);
        }
        if (astPipeline[idx].pOutputFile != NULL) {
            fclose(astPipeline[idx].pOutputFile);
            astPipeline[idx].pOutputFile = NULL;
        }
    }

    if (bAudioBound) {
        unbindCameraOSHalAudioEncodeFromAudioInput(pHandle, stAudio.u32AiChnId, stAudio.u32AencChnId);
    }
    if (bAencCreated) {
        destroyCameraOSHalAudioEncodeChannel(pHandle, stAudio.u32AencChnId);
    }
    if (bAiCreated) {
        destroyCameraOSHalAudioInputChannel(pHandle, stAudio.u32AiChnId);
    }
    if (stAudio.pRawOutputFile != NULL) {
        fclose(stAudio.pRawOutputFile);
        stAudio.pRawOutputFile = NULL;
    }
    if (stAudio.pEncodeOutputFile != NULL) {
        fclose(stAudio.pEncodeOutputFile);
        stAudio.pEncodeOutputFile = NULL;
    }

    if (pHandle != NULL) {
        destroyCameraOSHal(pHandle);
    }

    LEAVE();
    return retStatus == 0 ? 0 : -1;
}

#ifndef __CAMERAOS_HAL_H__
#define __CAMERAOS_HAL_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cameraos/hal/hal_common.h"
#include "cameraos/hal/hal_system.h"
#include "cameraos/hal/hal_video.h"
#include "cameraos/hal/hal_audio.h"
#include "cameraos/hal/hal_isp.h"

struct __CameraOSHalOps {
    int (*createCameraOSHal)(PCameraOSHalSystemConfig pSystemCfg, PCameraOSHalHandle* ppHandle);
    int (*destroyCameraOSHal)(PCameraOSHalHandle pHandle);

    int (*createVideoInputChannel)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputConfig pCfg);
    int (*destroyVideoInputChannel)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId);
    int (*getVideoInputFrame)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame, int32_t s32TimeoutMs);
    int (*releaseVideoInputFrame)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame);

    int (*createVideoEncodeChannel)(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeConfig pCfg);
    int (*destroyVideoEncodeChannel)(PCameraOSHalHandle pHandle, uint32_t u32VencChnId);
    int (*getVideoEncodeFrame)(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame, int32_t s32TimeoutMs);
    int (*releaseVideoEncodeFrame)(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame);

    int (*bindVideoInputToVideoEncode)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId);
    int (*unbindVideoEncodeFromVideoInput)(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId);

    int (*createAudioInputChannel)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputConfig pCfg);
    int (*destroyAudioInputChannel)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId);
    int (*getAudioInputFrame)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame, int32_t s32TimeoutMs);
    int (*releaseAudioInputFrame)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame);

    int (*createAudioEncodeChannel)(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeConfig pCfg);
    int (*destroyAudioEncodeChannel)(PCameraOSHalHandle pHandle, uint32_t u32AencChnId);
    int (*getAudioEncodeFrame)(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame, int32_t s32TimeoutMs);
    int (*releaseAudioEncodeFrame)(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame);

    int (*bindAudioInputToAudioEncode)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId);
    int (*unbindAudioEncodeFromAudioInput)(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId);
};
typedef struct __CameraOSHalOps* PCameraOSHalOps;

#ifdef __cplusplus
}
#endif

#endif

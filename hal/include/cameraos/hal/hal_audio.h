#ifndef __CAMERAOS_HAL_AUDIO_H__
#define __CAMERAOS_HAL_AUDIO_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cameraos/hal/hal_common.h"

typedef enum {
    HAL_AUDIO_CODEC_NONE = 0,
    HAL_AUDIO_CODEC_PCM = 1,
    HAL_AUDIO_CODEC_G711A = 2,
    HAL_AUDIO_CODEC_G711U = 3,
} HAL_AUDIO_CODEC_E;

typedef struct __CameraOSHalAudioInputConfig CameraOSHalAudioInputConfig;
struct __CameraOSHalAudioInputConfig {
    uint32_t u32SampleRate;
    uint32_t u32Channels;
    uint32_t u32BitWidth;
};
typedef struct __CameraOSHalAudioInputConfig* PCameraOSHalAudioInputConfig;

typedef struct __CameraOSHalAudioEncodeConfig CameraOSHalAudioEncodeConfig;
struct __CameraOSHalAudioEncodeConfig {
    HAL_AUDIO_CODEC_E enCodecType;
    uint32_t u32SampleRate;
    uint32_t u32Channels;
    uint32_t u32BitWidth;
    uint32_t u32BitRate;
};
typedef struct __CameraOSHalAudioEncodeConfig* PCameraOSHalAudioEncodeConfig;

typedef struct __CameraOSHalAudioInputFrame CameraOSHalAudioInputFrame;
struct __CameraOSHalAudioInputFrame {
    const uint8_t* data;
    uint32_t length;
    int64_t timestamp;
    uint32_t u32SampleRate;
    uint32_t u32Channels;
    uint32_t u32BitWidth;
    HAL_AUDIO_CODEC_E enCodecType;
};
typedef struct __CameraOSHalAudioInputFrame* PCameraOSHalAudioInputFrame;

typedef struct __CameraOSHalAudioEncodeFrame CameraOSHalAudioEncodeFrame;
struct __CameraOSHalAudioEncodeFrame {
    const uint8_t* data;
    uint32_t length;
    int64_t timestamp;
    HAL_AUDIO_CODEC_E enCodecType;
};
typedef struct __CameraOSHalAudioEncodeFrame* PCameraOSHalAudioEncodeFrame;

int createCameraOSHalAudioInputChannel(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputConfig pCfg);
int destroyCameraOSHalAudioInputChannel(PCameraOSHalHandle pHandle, uint32_t u32AiChnId);
int getCameraOSHalAudioInputFrame(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame, int32_t s32TimeoutMs);
int releaseCameraOSHalAudioInputFrame(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame);

int createCameraOSHalAudioEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeConfig pCfg);
int destroyCameraOSHalAudioEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32AencChnId);
int getCameraOSHalAudioEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame, int32_t s32TimeoutMs);
int releaseCameraOSHalAudioEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame);

int bindCameraOSHalAudioInputToAudioEncode(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId);
int unbindCameraOSHalAudioEncodeFromAudioInput(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId);

#ifdef __cplusplus
}
#endif

#endif

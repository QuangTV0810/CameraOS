#ifndef __CAMERAOS_MEDIA_SERVICE_H__
#define __CAMERAOS_MEDIA_SERVICE_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "CommonDefs.h"
#include "osapi-common.h"
#include "osapi-error.h"

#include "cameraos/hal/hal.h"

#define CAMERAOS_MEDIA_MAX_PIPELINES 3U
#define CAMERAOS_MEDIA_MAX_SINKS     8U

typedef struct __CameraOSMediaServiceHandle CameraOSMediaServiceHandle;
typedef struct __CameraOSMediaServiceHandle* PCameraOSMediaServiceHandle;

typedef enum {
    MEDIA_SERVICE_STATE_STOPPED = 0,
    MEDIA_SERVICE_STATE_STARTING,
    MEDIA_SERVICE_STATE_RUNNING,
    MEDIA_SERVICE_STATE_ERROR,
    MEDIA_SERVICE_STATE_STOPPING,
} MEDIA_SERVICE_STATE_E;

typedef enum {
    MEDIA_FRAME_SOURCE_VIDEO_INPUT = 0,
    MEDIA_FRAME_SOURCE_VIDEO_ENCODE,
    MEDIA_FRAME_SOURCE_AUDIO_INPUT,
    MEDIA_FRAME_SOURCE_AUDIO_ENCODE,
} MEDIA_FRAME_SOURCE_E;

typedef enum {
    MEDIA_PIPELINE_TYPE_MAIN = 0,
    MEDIA_PIPELINE_TYPE_SUB,
    MEDIA_PIPELINE_TYPE_AUX,
} MEDIA_PIPELINE_TYPE_E;

typedef struct __CameraOSMediaServiceVideoConfig CameraOSMediaServiceVideoConfig;
struct __CameraOSMediaServiceVideoConfig {
    uint32_t u32PipelineCount;
    uint32_t au32PipelineId[CAMERAOS_MEDIA_MAX_PIPELINES];
    MEDIA_PIPELINE_TYPE_E aenPipelineType[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnabled[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnableVideoInputFrame[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnableVideoEncodeFrame[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalVideoInputConfig astVideoInputConfig[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalVideoEncodeConfig astVideoEncodeConfig[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnableOsd[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnableOsdDateTime[CAMERAOS_MEDIA_MAX_PIPELINES];
    bool abEnableOsdTitle[CAMERAOS_MEDIA_MAX_PIPELINES];
    HAL_OSD_DATE_FORMAT_E aenOsdDateFormat[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalOsdTitleConfig astOsdTitleConfig[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalOsdPoint astOsdTimePosition[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalOsdPoint astOsdDatePosition[CAMERAOS_MEDIA_MAX_PIPELINES];
    CameraOSHalOsdPoint astOsdTitlePosition[CAMERAOS_MEDIA_MAX_PIPELINES];
};
typedef struct __CameraOSMediaServiceVideoConfig* PCameraOSMediaServiceVideoConfig;

typedef struct __CameraOSMediaServiceAudioConfig CameraOSMediaServiceAudioConfig;
struct __CameraOSMediaServiceAudioConfig {
    bool bEnable;
    bool bEnableInputFrame;
    bool bEnableEncodeFrame;
    CameraOSHalAudioInputConfig stInputConfig;
    CameraOSHalAudioEncodeConfig stEncodeConfig;
};
typedef struct __CameraOSMediaServiceAudioConfig* PCameraOSMediaServiceAudioConfig;

typedef struct __CameraOSMediaServiceConfig CameraOSMediaServiceConfig;
struct __CameraOSMediaServiceConfig {
    CameraOSMediaServiceVideoConfig stVideoConfig;
    CameraOSMediaServiceAudioConfig stAudioConfig;
};
typedef struct __CameraOSMediaServiceConfig* PCameraOSMediaServiceConfig;

typedef struct __CameraOSMediaServiceVideoRawFrameInfo CameraOSMediaServiceVideoRawFrameInfo;
struct __CameraOSMediaServiceVideoRawFrameInfo {
    uint32_t u32PipelineId;
    uint32_t u32ViChnId;
    const CameraOSHalVideoInputFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceVideoRawFrameInfo* PCCameraOSMediaServiceVideoRawFrameInfo;

typedef struct __CameraOSMediaServiceEncodeFrameInfo CameraOSMediaServiceEncodeFrameInfo;
struct __CameraOSMediaServiceEncodeFrameInfo {
    uint32_t u32PipelineId;
    uint32_t u32VencChnId;
    const CameraOSHalVideoEncodeFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceEncodeFrameInfo* PCCameraOSMediaServiceEncodeFrameInfo;

typedef struct __CameraOSMediaServiceAudioRawFrameInfo CameraOSMediaServiceAudioRawFrameInfo;
struct __CameraOSMediaServiceAudioRawFrameInfo {
    uint32_t u32AiChnId;
    const CameraOSHalAudioInputFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceAudioRawFrameInfo* PCCameraOSMediaServiceAudioRawFrameInfo;

typedef struct __CameraOSMediaServiceAudioEncodeFrameInfo CameraOSMediaServiceAudioEncodeFrameInfo;
struct __CameraOSMediaServiceAudioEncodeFrameInfo {
    uint32_t u32AencChnId;
    const CameraOSHalAudioEncodeFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceAudioEncodeFrameInfo* PCCameraOSMediaServiceAudioEncodeFrameInfo;

typedef struct __CameraOSMediaServiceFrameSink CameraOSMediaServiceFrameSink;
struct __CameraOSMediaServiceFrameSink {
    const char* pName;
    int (*onVideoRawFrame)(PCCameraOSMediaServiceVideoRawFrameInfo pFrameInfo, void* pUserData);
    int (*onVideoEncodeFrame)(PCCameraOSMediaServiceEncodeFrameInfo pFrameInfo, void* pUserData);
    int (*onAudioRawFrame)(PCCameraOSMediaServiceAudioRawFrameInfo pFrameInfo, void* pUserData);
    int (*onAudioEncodeFrame)(PCCameraOSMediaServiceAudioEncodeFrameInfo pFrameInfo, void* pUserData);
    void* pUserData;
};
typedef struct __CameraOSMediaServiceFrameSink* PCameraOSMediaServiceFrameSink;

int createCameraOSMediaService(PCameraOSMediaServiceConfig pConfig, PCameraOSMediaServiceHandle* ppHandle);
int destroyCameraOSMediaService(PCameraOSMediaServiceHandle pHandle);

int startCameraOSMediaService(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);
int stopCameraOSMediaService(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);

int registerCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceFrameSink pSink);
int unregisterCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, const char* pSinkName);

MEDIA_SERVICE_STATE_E getCameraOSMediaServiceState(PCameraOSMediaServiceHandle pHandle);
int getCameraOSMediaServiceConfig(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceVideoConfig pConfig);

#ifdef __cplusplus
}
#endif

#endif

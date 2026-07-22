#ifndef __CAMERAOS_MEDIA_SERVICE_H__
#define __CAMERAOS_MEDIA_SERVICE_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "../../core/osal/include/osapi-common.h"
#include "../../core/osal/include/osapi-error.h"

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
} MEDIA_FRAME_SOURCE_E;

typedef enum {
    MEDIA_PIPELINE_TYPE_MAIN = 0,
    MEDIA_PIPELINE_TYPE_SUB,
    MEDIA_PIPELINE_TYPE_AUX,
} MEDIA_PIPELINE_TYPE_E;

typedef struct __CameraOSMediaServicePipelineConfig CameraOSMediaServicePipelineConfig;
struct __CameraOSMediaServicePipelineConfig {
    uint32_t u32PipelineId;
    MEDIA_PIPELINE_TYPE_E enPipelineType;
    bool bEnabled;
    bool bEnableVideoInputFrame;
    bool bEnableVideoEncodeFrame;
    bool bWriteEncodeToFile;
    const char* pEncodeOutputFilePath;
    CameraOSHalVideoInputConfig stVideoInputConfig;
    CameraOSHalVideoEncodeConfig stVideoEncodeConfig;
};
typedef struct __CameraOSMediaServicePipelineConfig* PCameraOSMediaServicePipelineConfig;

typedef struct __CameraOSMediaServiceConfig CameraOSMediaServiceConfig;
struct __CameraOSMediaServiceConfig {
    uint32_t u32PipelineCount;
    CameraOSMediaServicePipelineConfig astPipelineConfig[CAMERAOS_MEDIA_MAX_PIPELINES];
};
typedef struct __CameraOSMediaServiceConfig* PCameraOSMediaServiceConfig;

typedef struct __CameraOSMediaServiceEncodeFrameInfo CameraOSMediaServiceEncodeFrameInfo;
struct __CameraOSMediaServiceEncodeFrameInfo {
    uint32_t u32PipelineId;
    uint32_t u32VencChnId;
    const CameraOSHalVideoEncodeFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceEncodeFrameInfo* PCCameraOSMediaServiceEncodeFrameInfo;

typedef struct __CameraOSMediaServiceRawFrameInfo CameraOSMediaServiceRawFrameInfo;
struct __CameraOSMediaServiceRawFrameInfo {
    uint32_t u32PipelineId;
    uint32_t u32ViChnId;
    const CameraOSHalVideoInputFrame* pFrame;
};
typedef const struct __CameraOSMediaServiceRawFrameInfo* PCCameraOSMediaServiceRawFrameInfo;

typedef struct __CameraOSMediaServiceFrameSink CameraOSMediaServiceFrameSink;
struct __CameraOSMediaServiceFrameSink {
    const char* pName;
    bool bEnableEncodedFrame;
    bool bEnableRawFrame;
    int (*onEncodedFrame)(PCCameraOSMediaServiceEncodeFrameInfo pFrameInfo, void* pUserData);
    int (*onRawFrame)(PCCameraOSMediaServiceRawFrameInfo pFrameInfo, void* pUserData);
    void* pUserData;
};
typedef struct __CameraOSMediaServiceFrameSink* PCameraOSMediaServiceFrameSink;

int createCameraOSMediaService(PCameraOSMediaServiceConfig pConfig, PCameraOSMediaServiceHandle* ppHandle);
int destroyCameraOSMediaService(PCameraOSMediaServiceHandle pHandle);

int startCameraOSMediaServicePipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);
int stopCameraOSMediaServicePipeline(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId);

int registerCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, PCameraOSMediaServiceFrameSink pSink);
int unregisterCameraOSMediaServiceSink(PCameraOSMediaServiceHandle pHandle, const char* pSinkName);

MEDIA_SERVICE_STATE_E getCameraOSMediaServiceState(PCameraOSMediaServiceHandle pHandle);
int getCameraOSMediaServiceConfig(PCameraOSMediaServiceHandle pHandle, uint32_t u32PipelineId, PCameraOSMediaServicePipelineConfig pConfig);

#ifdef __cplusplus
}
#endif

#endif

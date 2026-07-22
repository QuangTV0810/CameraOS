#ifndef __CAMERAOS_HAL_H__
#define __CAMERAOS_HAL_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "CommonDefs.h"
#include "PlatformUtils.h"

typedef enum {
    HAL_ERR_NONE = 0,
    HAL_ERR_INVALID_ARG = -1,
    HAL_ERR_NOMEM = -2,
    HAL_ERR_NOT_FOUND = -3,
    HAL_ERR_INVALID_STATE = -4,
    HAL_ERR_SYS = -5,
} HAL_ERROR_E;

typedef enum {
    HAL_VIDEO_CODEC_NONE = 0,
    HAL_VIDEO_CODEC_H264 = 1,
    HAL_VIDEO_CODEC_H265 = 2,
    HAL_VIDEO_CODEC_JPEG = 3,
} HAL_VIDEO_CODEC_E;

typedef enum {
    HAL_AUDIO_CODEC_NONE = 0,
    HAL_AUDIO_CODEC_G711A = 1,
    HAL_AUDIO_CODEC_G711U = 2,
} HAL_AUDIO_CODEC_E;

typedef enum {
    HAL_VIDEO_ENCODE_RC_MODE_NONE = 0,
    HAL_VIDEO_ENCODE_RC_MODE_H264_CBR,
    HAL_VIDEO_ENCODE_RC_MODE_H264_VBR,
    HAL_VIDEO_ENCODE_RC_MODE_H265_CBR,
    HAL_VIDEO_ENCODE_RC_MODE_H265_VBR,
    HAL_VIDEO_ENCODE_RC_MODE_MJPEG_CBR,
} HAL_VIDEO_ENCODE_RC_MODE_E;

typedef enum {
    HAL_VIDEO_FORMAT_NONE = 0,

    HAL_VIDEO_FORMAT_YUYV,
    HAL_VIDEO_FORMAT_YVYU,
    HAL_VIDEO_FORMAT_NV12,
    HAL_VIDEO_FORMAT_NV16,
    HAL_VIDEO_FORMAT_YUV420P,
    HAL_VIDEO_FORMAT_YUV420SP,
    HAL_VIDEO_FORMAT_YUV422SP,

    HAL_VIDEO_FORMAT_GRAY8,

    HAL_VIDEO_FORMAT_RGB888,
    HAL_VIDEO_FORMAT_BGR888,
    HAL_VIDEO_FORMAT_ARGB8888,
    HAL_VIDEO_FORMAT_RGBA8888,

    HAL_VIDEO_FORMAT_MJPEG,
    HAL_VIDEO_FORMAT_H264,
    HAL_VIDEO_FORMAT_H265,
} HAL_VIDEO_FORMAT_E;

typedef enum {
    HAL_VIDEO_ROTATION_0 = 0,
    HAL_VIDEO_ROTATION_90R,
    HAL_VIDEO_ROTATION_90L,
    HAL_VIDEO_ROTATION_180,
} HAL_VIDEO_ROTATION_E;

typedef enum {
    HAL_VIDEO_MIRROR_NONE = 0,
    HAL_VIDEO_MIRROR_VERTICAL,
    HAL_VIDEO_MIRROR_HORIZONTAL,
    HAL_VIDEO_MIRROR_BOTH,
} HAL_VIDEO_MIRROR_E;

typedef enum {
    /* H.264 */
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H264_SPS = 0x10,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H264_PPS = 0x11,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H264_SEI = 0x12,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H264_IDR = 0x13,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H264_SLICE = 0x14,

    /* H.265 */
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_VPS = 0x20,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_SPS = 0x21,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_PPS = 0x22,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_SEI = 0x23,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_IDR = 0x24,
    HAL_VIDEO_ENCODE_NALU_TYPE_E_H265_SLICE = 0x25,

    /* JPEG */
    HAL_VIDEO_ENCODE_NALU_TYPE_E_JPEG_FRAME = 0x30,

    HAL_VIDEO_ENCODE_NALU_TYPE_E_UNKNOWN = 0xFF,
} HAL_VIDEO_ENCODE_NALU_TYPE_E;

typedef struct __CameraOSHalHandle CameraOSHalHandle;
struct __CameraOSHalHandle {
    uint32_t version;
};
typedef struct __CameraOSHalHandle* PCameraOSHalHandle;

typedef struct __CameraOSHalSystemConfig CameraOSHalSystemConfig;
struct __CameraOSHalSystemConfig {
    uint32_t u32CameraId;
    uint32_t u32ViDevId;
    uint32_t u32ViPipeId;
    bool bEnableIsp;
    const char* pIqDir;
};
typedef struct __CameraOSHalSystemConfig* PCameraOSHalSystemConfig;

typedef struct __CameraOSHalVideoInputConfig CameraOSHalVideoInputConfig;
struct __CameraOSHalVideoInputConfig {
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32Fps;
    uint32_t u32Depth;
    uint32_t u32BufferCount;
    uint32_t u32PixelFormat;
    bool bMirror;
    bool bFlip;
};
typedef struct __CameraOSHalVideoInputConfig* PCameraOSHalVideoInputConfig;

typedef struct __CameraOSHalVideoEncodeConfig CameraOSHalVideoEncodeConfig;
struct __CameraOSHalVideoEncodeConfig {
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32Fps;
    uint32_t u32Gop;
    uint32_t u32BitRate;
    HAL_VIDEO_ROTATION_E enRotation;
    HAL_VIDEO_MIRROR_E enMirror;
    bool bMirror;
    bool bFlip;
    HAL_VIDEO_CODEC_E enCodecType;
    HAL_VIDEO_ENCODE_RC_MODE_E enRcMode;
};
typedef struct __CameraOSHalVideoEncodeConfig* PCameraOSHalVideoEncodeConfig;

typedef struct __CameraOSHalVideoInputFrame CameraOSHalVideoInputFrame;
struct __CameraOSHalVideoInputFrame {
    const uint8_t* data;
    uint32_t length;
    int64_t timestamp;
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32PixelFormat;
};
typedef struct __CameraOSHalVideoInputFrame* PCameraOSHalVideoInputFrame;

typedef struct __CameraOSHalVideoEncodeFrame CameraOSHalVideoEncodeFrame;
struct __CameraOSHalVideoEncodeFrame {
    const uint8_t* data;
    uint32_t length;
    int64_t timestamp;
    bool bKeyFrame;
    uint8_t u8NaluType;
    HAL_VIDEO_ENCODE_NALU_TYPE_E enNaluType;
};
typedef struct __CameraOSHalVideoEncodeFrame* PCameraOSHalVideoEncodeFrame;

typedef struct __CameraOSHalAudioEncodeFrame CameraOSHalAudioEncodeFrame;
struct __CameraOSHalAudioEncodeFrame {
    const uint8_t* data;
    uint32_t length;
    int64_t timestamp;
};
typedef struct __CameraOSHalAudioEncodeFrame* PCameraOSHalAudioEncodeFrame;

typedef struct __CameraOSHalOps CameraOSHalOps;
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
};
typedef struct __CameraOSHalOps* PCameraOSHalOps;

int createCameraOSHal(PCameraOSHalSystemConfig pSystemCfg, PCameraOSHalOps pOps, PCameraOSHalHandle* ppHandle);
int destroyCameraOSHal(PCameraOSHalHandle pHandle);

int createCameraOSHalVideoInputChannel(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputConfig pCfg);
int destroyCameraOSHalVideoInputChannel(PCameraOSHalHandle pHandle, uint32_t u32ViChnId);
int getCameraOSHalVideoInputFrame(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame, int32_t s32TimeoutMs);
int releaseCameraOSHalVideoInputFrame(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame);

int createCameraOSHalVideoEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeConfig pCfg);
int destroyCameraOSHalVideoEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32VencChnId);
int getCameraOSHalVideoEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame, int32_t s32TimeoutMs);
int releaseCameraOSHalVideoEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame);

int bindCameraOSHalVideoInputToVideoEncode(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId);
int unbindCameraOSHalVideoEncodeFromeVideoInput(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId);

#ifdef __cplusplus
}
#endif

#endif

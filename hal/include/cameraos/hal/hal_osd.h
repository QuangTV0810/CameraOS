#ifndef __CAMERAOS_HAL_OSD_H__
#define __CAMERAOS_HAL_OSD_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cameraos/hal/hal_common.h"

#define HAL_OSD_MAX_TITLE_LENGTH 64U

typedef enum {
    HAL_OSD_DATE_FORMAT_YYYY_MM_DD = 1,
    HAL_OSD_DATE_FORMAT_DD_MM_YYYY = 2,
    HAL_OSD_DATE_FORMAT_MM_DD_YYYY = 3,
} HAL_OSD_DATE_FORMAT_E;

typedef enum {
    HAL_OSD_BLOCK_ID_TIME = 0,
    HAL_OSD_BLOCK_ID_DATE = 1,
    HAL_OSD_BLOCK_ID_TITLE = 2,
} HAL_OSD_BLOCK_ID_E;

typedef struct __CameraOSHalOsdPoint CameraOSHalOsdPoint;
struct __CameraOSHalOsdPoint {
    uint32_t u32X;
    uint32_t u32Y;
};
typedef struct __CameraOSHalOsdPoint* PCameraOSHalOsdPoint;

typedef struct __CameraOSHalOsdTitleConfig CameraOSHalOsdTitleConfig;
struct __CameraOSHalOsdTitleConfig {
    char acTitle[HAL_OSD_MAX_TITLE_LENGTH];
};
typedef struct __CameraOSHalOsdTitleConfig* PCameraOSHalOsdTitleConfig;

int createCameraOSHalOsd(PCameraOSHalHandle pHandle, uint32_t u32ViChnId);
int destroyCameraOSHalOsd(PCameraOSHalHandle pHandle, uint32_t u32ViChnId);

int setCameraOSHalOsdDateTimeEnable(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, bool bEnable);
int setCameraOSHalOsdTitleEnable(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, bool bEnable);

int setCameraOSHalOsdTitle(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalOsdTitleConfig pConfig);
int setCameraOSHalOsdDateFormat(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, HAL_OSD_DATE_FORMAT_E enFormat);
int setCameraOSHalOsdBlockPosition(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, HAL_OSD_BLOCK_ID_E enBlockId, PCameraOSHalOsdPoint pPoint);

#ifdef __cplusplus
}
#endif

#endif

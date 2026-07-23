#ifndef __CAMERAOS_HAL_SYSTEM_H__
#define __CAMERAOS_HAL_SYSTEM_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cameraos/hal/hal_common.h"

typedef struct __CameraOSHalSystemConfig CameraOSHalSystemConfig;
struct __CameraOSHalSystemConfig {
    uint32_t u32CameraId;
    uint32_t u32ViDevId;
    uint32_t u32ViPipeId;
    bool bEnableIsp;
    const char* pIqDir;
};
typedef struct __CameraOSHalSystemConfig* PCameraOSHalSystemConfig;

typedef struct __CameraOSHalOps CameraOSHalOps;
typedef struct __CameraOSHalOps* PCameraOSHalOps;

int createCameraOSHal(PCameraOSHalSystemConfig pSystemCfg, PCameraOSHalOps pOps, PCameraOSHalHandle* ppHandle);
int destroyCameraOSHal(PCameraOSHalHandle pHandle);

#ifdef __cplusplus
}
#endif

#endif

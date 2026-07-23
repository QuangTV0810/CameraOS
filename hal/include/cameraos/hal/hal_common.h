#ifndef __CAMERAOS_HAL_COMMON_H__
#define __CAMERAOS_HAL_COMMON_H__

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

typedef struct __CameraOSHalHandle CameraOSHalHandle;
struct __CameraOSHalHandle {
    uint32_t version;
};
typedef struct __CameraOSHalHandle* PCameraOSHalHandle;

#ifdef __cplusplus
}
#endif

#endif

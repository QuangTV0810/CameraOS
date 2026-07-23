#ifndef __CAMERAOS_HAL_INTERNAL_H__
#define __CAMERAOS_HAL_INTERNAL_H__

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_CLASS "hal-interface"
#include "cameraos/hal/hal.h"

#if !defined(CAMERAOS_HAL_LOG_ENABLE)
#undef DLOGE
#undef DLOGW
#undef DLOGI
#undef DLOGD
#undef DLOGV
#undef DLOGP
#undef ENTER
#undef LEAVE
#define DLOGE(...)
#define DLOGW(...)
#define DLOGI(...)
#define DLOGD(...)
#define DLOGV(...)
#define DLOGP(...)
#define ENTER()
#define LEAVE()
#endif

typedef struct __CameraOSHalWrapper CameraOSHalWrapper;
struct __CameraOSHalWrapper {
    CameraOSHalOps ops;
    PCameraOSHalHandle pHandle;
};
typedef struct __CameraOSHalWrapper* PCameraOSHalWrapper;

#endif

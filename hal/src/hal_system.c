#include "hal_internal.h"

int createCameraOSHal(PCameraOSHalSystemConfig pSystemCfg, PCameraOSHalOps pOps, PCameraOSHalHandle* ppHandle)
{
    int ret;

    ENTER();
    if (pSystemCfg == NULL || pOps == NULL || ppHandle == NULL || pOps->createCameraOSHal == NULL) {
        DLOGE("invalid create arguments");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    PCameraOSHalWrapper pHal = calloc(1, sizeof(CameraOSHalWrapper));
    if (pHal == NULL) {
        DLOGE("calloc wrapper failed");
        LEAVE();
        return HAL_ERR_NOMEM;
    }

    DLOGI("wrapper allocated: [%p] size=%zu", (void*) pHal, sizeof(CameraOSHalWrapper));
    memcpy(&pHal->ops, pOps, sizeof(CameraOSHalOps));

    ret = pHal->ops.createCameraOSHal(pSystemCfg, &pHal->pHandle);
    if (ret != HAL_ERR_NONE) {
        DLOGE("create impl failed ret=%d", ret);
        free(pHal);
        LEAVE();
        return ret;
    }

    DLOGI("impl handle: [%p]", (void*) pHal->pHandle);
    *ppHandle = (PCameraOSHalHandle) pHal;
    LEAVE();
    return HAL_ERR_NONE;
}

int destroyCameraOSHal(PCameraOSHalHandle pHandle)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL) {
        DLOGE("destroy called with NULL handle");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    DLOGI("destroy wrapper=[%p] impl=[%p]", (void*) pHal, (void*) pHal->pHandle);
    if (pHal->ops.destroyCameraOSHal != NULL) {
        pHal->ops.destroyCameraOSHal(pHal->pHandle);
    }
    free(pHal);
    LEAVE();
    return HAL_ERR_NONE;
}

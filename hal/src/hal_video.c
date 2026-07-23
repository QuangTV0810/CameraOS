#include "hal_internal.h"

int createCameraOSHalVideoInputChannel(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputConfig pCfg)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pCfg == NULL || pHal->ops.createVideoInputChannel == NULL) {
        DLOGE("create VI channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.createVideoInputChannel(pHal->pHandle, u32ViChnId, pCfg);
    LEAVE();
    return ret;
}

int destroyCameraOSHalVideoInputChannel(PCameraOSHalHandle pHandle, uint32_t u32ViChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.destroyVideoInputChannel == NULL) {
        DLOGE("destroy VI channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.destroyVideoInputChannel(pHal->pHandle, u32ViChnId);
    LEAVE();
    return ret;
}

int getCameraOSHalVideoInputFrame(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame, int32_t s32TimeoutMs)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.getVideoInputFrame == NULL) {
        DLOGE("get VI frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.getVideoInputFrame(pHal->pHandle, u32ViChnId, pFrame, s32TimeoutMs);
    LEAVE();
    return ret;
}

int releaseCameraOSHalVideoInputFrame(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalVideoInputFrame pFrame)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.releaseVideoInputFrame == NULL) {
        DLOGE("release VI frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.releaseVideoInputFrame(pHal->pHandle, u32ViChnId, pFrame);
    LEAVE();
    return ret;
}

int createCameraOSHalVideoEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeConfig pCfg)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pCfg == NULL || pHal->ops.createVideoEncodeChannel == NULL) {
        DLOGE("create VENC channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.createVideoEncodeChannel(pHal->pHandle, u32VencChnId, pCfg);
    LEAVE();
    return ret;
}

int destroyCameraOSHalVideoEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32VencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.destroyVideoEncodeChannel == NULL) {
        DLOGE("destroy VENC channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.destroyVideoEncodeChannel(pHal->pHandle, u32VencChnId);
    LEAVE();
    return ret;
}

int getCameraOSHalVideoEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame, int32_t s32TimeoutMs)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.getVideoEncodeFrame == NULL) {
        DLOGE("get VENC frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.getVideoEncodeFrame(pHal->pHandle, u32VencChnId, pFrame, s32TimeoutMs);
    LEAVE();
    return ret;
}

int releaseCameraOSHalVideoEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32VencChnId, PCameraOSHalVideoEncodeFrame pFrame)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.releaseVideoEncodeFrame == NULL) {
        DLOGE("release VENC frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.releaseVideoEncodeFrame(pHal->pHandle, u32VencChnId, pFrame);
    LEAVE();
    return ret;
}

int bindCameraOSHalVideoInputToVideoEncode(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.bindVideoInputToVideoEncode == NULL) {
        DLOGE("bind not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.bindVideoInputToVideoEncode(pHal->pHandle, u32ViChnId, u32VencChnId);
    LEAVE();
    return ret;
}

int unbindCameraOSHalVideoEncodeFromeVideoInput(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, uint32_t u32VencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.unbindVideoEncodeFromVideoInput == NULL) {
        DLOGE("unbind not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.unbindVideoEncodeFromVideoInput(pHal->pHandle, u32ViChnId, u32VencChnId);
    LEAVE();
    return ret;
}

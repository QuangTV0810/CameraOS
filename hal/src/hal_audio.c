#include "hal_internal.h"

int createCameraOSHalAudioInputChannel(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputConfig pCfg)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pCfg == NULL || pHal->ops.createAudioInputChannel == NULL) {
        DLOGE("create AI channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.createAudioInputChannel(pHal->pHandle, u32AiChnId, pCfg);
    LEAVE();
    return ret;
}

int destroyCameraOSHalAudioInputChannel(PCameraOSHalHandle pHandle, uint32_t u32AiChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.destroyAudioInputChannel == NULL) {
        DLOGE("destroy AI channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.destroyAudioInputChannel(pHal->pHandle, u32AiChnId);
    LEAVE();
    return ret;
}

int getCameraOSHalAudioInputFrame(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame, int32_t s32TimeoutMs)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.getAudioInputFrame == NULL) {
        DLOGE("get AI frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.getAudioInputFrame(pHal->pHandle, u32AiChnId, pFrame, s32TimeoutMs);
    LEAVE();
    return ret;
}

int releaseCameraOSHalAudioInputFrame(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, PCameraOSHalAudioInputFrame pFrame)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.releaseAudioInputFrame == NULL) {
        DLOGE("release AI frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.releaseAudioInputFrame(pHal->pHandle, u32AiChnId, pFrame);
    LEAVE();
    return ret;
}

int createCameraOSHalAudioEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeConfig pCfg)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pCfg == NULL || pHal->ops.createAudioEncodeChannel == NULL) {
        DLOGE("create AENC channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.createAudioEncodeChannel(pHal->pHandle, u32AencChnId, pCfg);
    LEAVE();
    return ret;
}

int destroyCameraOSHalAudioEncodeChannel(PCameraOSHalHandle pHandle, uint32_t u32AencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.destroyAudioEncodeChannel == NULL) {
        DLOGE("destroy AENC channel not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.destroyAudioEncodeChannel(pHal->pHandle, u32AencChnId);
    LEAVE();
    return ret;
}

int getCameraOSHalAudioEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame, int32_t s32TimeoutMs)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.getAudioEncodeFrame == NULL) {
        DLOGE("get AENC frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.getAudioEncodeFrame(pHal->pHandle, u32AencChnId, pFrame, s32TimeoutMs);
    LEAVE();
    return ret;
}

int releaseCameraOSHalAudioEncodeFrame(PCameraOSHalHandle pHandle, uint32_t u32AencChnId, PCameraOSHalAudioEncodeFrame pFrame)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pFrame == NULL || pHal->ops.releaseAudioEncodeFrame == NULL) {
        DLOGE("release AENC frame not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.releaseAudioEncodeFrame(pHal->pHandle, u32AencChnId, pFrame);
    LEAVE();
    return ret;
}

int bindCameraOSHalAudioInputToAudioEncode(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.bindAudioInputToAudioEncode == NULL) {
        DLOGE("bind audio not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.bindAudioInputToAudioEncode(pHal->pHandle, u32AiChnId, u32AencChnId);
    LEAVE();
    return ret;
}

int unbindCameraOSHalAudioEncodeFromAudioInput(PCameraOSHalHandle pHandle, uint32_t u32AiChnId, uint32_t u32AencChnId)
{
    ENTER();
    PCameraOSHalWrapper pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.unbindAudioEncodeFromAudioInput == NULL) {
        DLOGE("unbind audio not available");
        LEAVE();
        return -1;
    }

    int ret = pHal->ops.unbindAudioEncodeFromAudioInput(pHal->pHandle, u32AiChnId, u32AencChnId);
    LEAVE();
    return ret;
}

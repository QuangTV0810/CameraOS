#include "hal_internal.h"

int createCameraOSHalOsd(PCameraOSHalHandle pHandle, uint32_t u32ViChnId)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.createOsd == NULL) {
        DLOGE("create OSD not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.createOsd(pHal->pHandle, u32ViChnId);
}

int destroyCameraOSHalOsd(PCameraOSHalHandle pHandle, uint32_t u32ViChnId)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.destroyOsd == NULL) {
        DLOGE("destroy OSD not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.destroyOsd(pHal->pHandle, u32ViChnId);
}

int setCameraOSHalOsdDateTimeEnable(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, bool bEnable)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.setOsdDateTimeEnable == NULL) {
        DLOGE("set OSD date time enable not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.setOsdDateTimeEnable(pHal->pHandle, u32ViChnId, bEnable);
}

int setCameraOSHalOsdTitleEnable(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, bool bEnable)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.setOsdTitleEnable == NULL) {
        DLOGE("set OSD title enable not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.setOsdTitleEnable(pHal->pHandle, u32ViChnId, bEnable);
}

int setCameraOSHalOsdTitle(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, PCameraOSHalOsdTitleConfig pConfig)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pConfig == NULL || pHal->ops.setOsdTitle == NULL) {
        DLOGE("set OSD title not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.setOsdTitle(pHal->pHandle, u32ViChnId, pConfig);
}

int setCameraOSHalOsdDateFormat(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, HAL_OSD_DATE_FORMAT_E enFormat)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pHal->ops.setOsdDateFormat == NULL) {
        DLOGE("set OSD date format not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.setOsdDateFormat(pHal->pHandle, u32ViChnId, enFormat);
}

int setCameraOSHalOsdBlockPosition(PCameraOSHalHandle pHandle, uint32_t u32ViChnId, HAL_OSD_BLOCK_ID_E enBlockId, PCameraOSHalOsdPoint pPoint)
{
    PCameraOSHalWrapper pHal;

    ENTER();
    pHal = (PCameraOSHalWrapper) pHandle;
    if (pHal == NULL || pPoint == NULL || pHal->ops.setOsdBlockPosition == NULL) {
        DLOGE("set OSD block position not available");
        LEAVE();
        return HAL_ERR_INVALID_ARG;
    }

    LEAVE();
    return pHal->ops.setOsdBlockPosition(pHal->pHandle, u32ViChnId, enBlockId, pPoint);
}

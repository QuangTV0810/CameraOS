#ifndef __CAMERAOS_IPCAMERA_APP_H__
#define __CAMERAOS_IPCAMERA_APP_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int initApp(void);
void startApp(void);
void deinitApp(void);
void stopApp(void);

#ifdef __cplusplus
}
#endif

#endif

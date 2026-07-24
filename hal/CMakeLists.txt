if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(cameraos_hal C)
endif()

set(CAMERAOS_HAL_VENDOR "rv1106" CACHE STRING "HAL vendor to build")
set_property(CACHE CAMERAOS_HAL_VENDOR PROPERTY STRINGS rv1106 rts3917n)
option(CAMERAOS_HAL_LOG_ENABLE "Enable HAL logs" ON)

set(HAL_VENDOR_SRC)
set(HAL_VENDOR_INCLUDE_DIR)
set(HAL_VENDOR_LINK_DIRS)
set(HAL_VENDOR_LINK_LIBS)
set(HAL_VENDOR_COMPILE_DEFINITIONS)

if(CAMERAOS_HAL_VENDOR STREQUAL "rv1106")
    set(HAL_VENDOR_SRC
        vendors/rockchip/rv1106/src/rv1106_hal.c
        vendors/rockchip/rv1106/src/rv1106_hal_system.c
        vendors/rockchip/rv1106/src/rv1106_hal_video.c
        vendors/rockchip/rv1106/src/rv1106_hal_isp.c
    )
    set(HAL_VENDOR_INCLUDE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/vendors/rockchip/rv1106/include
    )
    set(HAL_VENDOR_LINK_DIRS
        /home/quangtv/Workspace/Rockchip/SDK/luckfox-pico/output/out/media_out/lib
    )
    set(HAL_VENDOR_LINK_LIBS
        rockit
        rkaiq
    )
    set(HAL_VENDOR_COMPILE_DEFINITIONS
        CAMERAOS_HAL_VENDOR_RV1106=1
    )

    set(RV1106_SDK_ROOT /home/quangtv/Workspace/Rockchip/SDK/luckfox-pico)
    set(RV1106_MEDIA_INCLUDE ${RV1106_SDK_ROOT}/output/out/media_out/include)
    set(RV1106_SAMPLE_COMMON_INCLUDE
        ${RV1106_SDK_ROOT}/media/samples/example/common
    )
    set(RV1106_SAMPLE_ISP3_INCLUDE
        ${RV1106_SDK_ROOT}/media/samples/example/common/isp3.x
    )
    set(RV1106_RKAIQ_INCLUDE
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include
    )
    set(RV1106_RKAIQ_UAPI_INCLUDE
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include/rkaiq/uAPI
    )
    set(RV1106_RKAIQ_UAPI2_INCLUDE
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/uAPI2
    )
    set(RV1106_RKAIQ_INCLUDE_ROOT
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include/rkaiq
    )
    set(RV1106_RKAIQ_INCLUDE_COMMON
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include/rkaiq/common
    )
    set(RV1106_RKAIQ_INCLUDE_XCORE
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include/rkaiq/xcore
    )
    set(RV1106_RKAIQ_INCLUDE_ALGOS
        ${RV1106_SDK_ROOT}/media/isp/release_camera_engine_rkaiq_rv1126_arm-rockchip830-linux-gnueabihf/include/rkaiq/algos
    )

    list(APPEND HAL_VENDOR_INCLUDE_DIR
        ${RV1106_MEDIA_INCLUDE}
        ${RV1106_SAMPLE_COMMON_INCLUDE}
        ${RV1106_SAMPLE_ISP3_INCLUDE}
        ${RV1106_RKAIQ_INCLUDE}
        ${RV1106_RKAIQ_UAPI_INCLUDE}
        ${RV1106_RKAIQ_UAPI2_INCLUDE}
        ${RV1106_RKAIQ_INCLUDE_ROOT}
        ${RV1106_RKAIQ_INCLUDE_COMMON}
        ${RV1106_RKAIQ_INCLUDE_XCORE}
        ${RV1106_RKAIQ_INCLUDE_ALGOS}
    )
elseif(CAMERAOS_HAL_VENDOR STREQUAL "rts3917n")
    set(HAL_VENDOR_SRC
        vendors/realtek/rts3917n/rts3917n_hal.c
        vendors/realtek/rts3917n/rts3917n_hal_system.c
        vendors/realtek/rts3917n/rts3917n_hal_video.c
        vendors/realtek/rts3917n/rts3917n_hal_audio.c
        vendors/realtek/rts3917n/rts3917n_hal_osd.c
    )
    set(HAL_VENDOR_INCLUDE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/vendors/realtek/rts3917n
        /home/quangtv/Workspace/IPCam/Source/IP_Cam/out/rts3917n/staging/usr/include
        /home/quangtv/Workspace/IPCam/Source/IP_Cam/out/rts3917n/staging/usr/include/freetype2
    )
    set(HAL_VENDOR_LINK_DIRS
        /home/quangtv/Workspace/IPCam/Source/IP_Cam/out/rts3917n/staging/usr/lib
    )
    set(HAL_VENDOR_LINK_LIBS
        rtstream
        rtsvideo
        rtsaudio
        asound
        soxr
        avutil
        freetype
        z
    )
    set(HAL_VENDOR_COMPILE_DEFINITIONS
        CAMERAOS_HAL_VENDOR_RTS3917N=1
    )
else()
    message(FATAL_ERROR "Unsupported CAMERAOS_HAL_VENDOR=${CAMERAOS_HAL_VENDOR}")
endif()

add_library(cameraos_hal
    src/hal_system.c
    src/hal_video.c
    src/hal_audio.c
    src/hal_osd.c
    src/hal_isp.c
    ${HAL_VENDOR_SRC}
    ../platform/PlatformUtils.c
)

target_include_directories(cameraos_hal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${HAL_VENDOR_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)

target_link_directories(cameraos_hal
    PUBLIC
        ${HAL_VENDOR_LINK_DIRS}
)

target_link_libraries(cameraos_hal
    PUBLIC
        ${HAL_VENDOR_LINK_LIBS}
)

target_compile_definitions(cameraos_hal
    PUBLIC
        ${HAL_VENDOR_COMPILE_DEFINITIONS}
        $<$<BOOL:${CAMERAOS_HAL_LOG_ENABLE}>:CAMERAOS_HAL_LOG_ENABLE=1>
)

add_executable(cameraos_hal_test
    test/main.c
)

target_include_directories(cameraos_hal_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../platform
        ${HAL_VENDOR_INCLUDE_DIR}
)

target_link_libraries(cameraos_hal_test
    PRIVATE
        cameraos_hal
        ${HAL_VENDOR_LINK_LIBS}
)

target_compile_definitions(cameraos_hal_test
    PRIVATE
        ${HAL_VENDOR_COMPILE_DEFINITIONS}
        $<$<BOOL:${CAMERAOS_HAL_LOG_ENABLE}>:CAMERAOS_HAL_LOG_ENABLE=1>
)

target_link_directories(cameraos_hal_test
    PRIVATE
        ${HAL_VENDOR_LINK_DIRS}
)

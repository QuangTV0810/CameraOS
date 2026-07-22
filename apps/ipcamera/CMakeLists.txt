add_executable(ipcamera
    main.c
    ../../services/media/src/osal_bsp_shim.c
)

target_include_directories(ipcamera
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../platform
        ${CMAKE_CURRENT_SOURCE_DIR}/../../core/osal/include
)

target_link_libraries(ipcamera
    PRIVATE
        cameraos_media_service
)

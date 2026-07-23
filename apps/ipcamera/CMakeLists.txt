set(CAMERAOS_OSAL_INSTALL_DIR
    ${CMAKE_SOURCE_DIR}/third_party/osal/install
)

add_executable(ipcamera
    main.c
    app.c
)

target_include_directories(ipcamera
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../platform
        ${CAMERAOS_OSAL_INSTALL_DIR}/include/osal
)

target_link_libraries(ipcamera
    PRIVATE
        cameraos_media_service
        ${CAMERAOS_OSAL_INSTALL_DIR}/lib/libosal.a
        ${CAMERAOS_OSAL_INSTALL_DIR}/lib/libosal_bsp.a
)

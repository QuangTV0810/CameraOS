set(CAMERAOS_OSAL_INSTALL_DIR
    ${CMAKE_SOURCE_DIR}/third_party/osal/install
)

add_library(cameraos_media_service
    src/media_service.c
)

target_include_directories(cameraos_media_service
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../hal/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../platform
        ${CAMERAOS_OSAL_INSTALL_DIR}/include/osal
)

target_link_libraries(cameraos_media_service
    PUBLIC
        cameraos_hal
)

cmake_minimum_required(VERSION 3.16)

project(CameraOS C)

add_subdirectory(hal)
add_subdirectory(services/media)
add_subdirectory(apps/ipcamera)

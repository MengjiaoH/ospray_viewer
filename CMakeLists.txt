cmake_minimum_required(VERSION 3.11)
project(OSPRAY_VIEWER)

set(CMAKE_CXX_STANDARD 11)

## path to glfw3
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/glfw-3.3.2")
include_directories("${CMAKE_SOURCE_DIR}/externals/glfw-3.3.2/include")

## path to gl3w
include_directories("${CMAKE_SOURCE_DIR}/externals/gl3w/include")
add_library(gl3w ${CMAKE_SOURCE_DIR}/externals/gl3w/src/gl3w.c)
set_target_properties(gl3w PROPERTIES COMPILE_FLAGS "-w")
target_include_directories(gl3w PUBLIC "$<BUILD_INTERFACE:""externals/gl3w/include;"">")

# find OpenGL
find_package(OpenGL REQUIRED)

## find rkcommon
find_package(rkcommon REQUIRED)
## find ospray
find_package(ospray REQUIRED)

## find vtk-m
find_package(VTKm REQUIRED QUIET)

## find vtk
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})


# build imgui
add_subdirectory("${CMAKE_SOURCE_DIR}/externals/imgui")
include_directories("${CMAKE_SOURCE_DIR}/externals/imgui")

#build utils
add_subdirectory("${CMAKE_SOURCE_DIR}/utils")
include_directories("${CMAKE_SOURCE_DIR}/utils")

add_executable(ospray_viewer viewer.cpp)
set_target_properties(ospray_viewer PROPERTIES 
                                    CXX_STANDARD 11
                                    CXX_STANDARD_REQUIRED ON)

target_link_libraries(ospray_viewer glfw 
                                    gl3w 
                                    imgui
                                    util
                                    rkcommon::rkcommon
                                    ospray::ospray)

add_executable(multi_ts_gen_images multi_ts_gen_images.cpp)
set_target_properties(multi_ts_gen_images PROPERTIES 
                                    CXX_STANDARD 11
                                    CXX_STANDARD_REQUIRED ON)

target_link_libraries(multi_ts_gen_images glfw 
                                          gl3w 
                                          imgui
                                          util
                                          rkcommon::rkcommon
                                          ospray::ospray
                                          ${VTK_LIBRARIES})

target_include_directories(multi_ts_gen_images PRIVATE ${VTKm_INCLUDE_DIRS})

add_executable(unstructed_viewer testUnstructured.cpp)
set_target_properties(unstructed_viewer PROPERTIES 
                                    CXX_STANDARD 11
                                    CXX_STANDARD_REQUIRED ON)

target_include_directories(unstructed_viewer PRIVATE ${VTKm_INCLUDE_DIRS})
target_link_libraries(unstructed_viewer glfw 
                                    gl3w 
                                    imgui
                                    util
                                    rkcommon::rkcommon
                                    ospray::ospray
                                    vtkm_cont 
                                    vtkm_io
                                    ${VTKm_LIBRARIES}
                                    ${VTK_LIBRARIES})
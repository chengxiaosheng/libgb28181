cmake_minimum_required(VERSION 3.15)
project(gb28181)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME})
file(GLOB GB28181_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${GB28181_SRC} main.cpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
add_subdirectory(3rdpart)
target_link_libraries(${PROJECT_NAME} PRIVATE sip)

if(TARGET ZLToolKit)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit)
elseif (TARGET ZLToolKit_SHARED)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_SHARED)
elseif (TARGET ZLToolKit_static)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_static)
else ()
    message(STATUS "zltoolkit target not found. Fetching from Git...")
    include(FetchContent)
    FetchContent_Declare(
            ZLToolKit
            GIT_REPOSITORY https://gitee.com/xia-chu/ZLToolKit.git  # 替换为实际的仓库地址
            GIT_TAG master                                           # 替换为实际的分支或标签
    )
    FetchContent_MakeAvailable(ZLToolKit)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_static)
endif ()

add_executable(gb28181-exe main.cpp)

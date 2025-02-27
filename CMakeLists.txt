cmake_minimum_required(VERSION 3.15)
project(gb28181)

set(CMAKE_CXX_STANDARD 17)


if (MSVC)
    add_compile_options("/utf-8")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif ()

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
target_compile_definitions(${PROJECT_NAME} PUBLIC -DENABLE_OPENSSL)

find_package(OpenSSL 1.1 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::Crypto)

find_package(Iconv REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)

if(TARGET ZLToolKit)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit)
elseif (TARGET ZlToolKit)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZlToolKit)
elseif (TARGET ZLToolKit_SHARED)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_SHARED)
elseif (TARGET ZLToolKit_static)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_static)
else ()
    include(FetchContent)
    message(STATUS "zltoolkit target not found. Fetching from Git...")
    FetchContent_Declare(
            ZLToolKit
            GIT_REPOSITORY https://gitee.com/xia-chu/ZLToolKit.git
            GIT_TAG master
    )
    FetchContent_MakeAvailable(ZLToolKit)
    target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit_static)
endif ()
if (NOT TARGET tinyxml2)
    add_subdirectory(3rdpart/tinyxml2)
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC tinyxml2)


add_executable(gb28181-exe main.cpp)
target_link_libraries(gb28181-exe PRIVATE ${PROJECT_NAME})
target_include_directories(gb28181-exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


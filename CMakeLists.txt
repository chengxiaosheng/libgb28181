cmake_minimum_required(VERSION 3.15)
project(gb28181 VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 17)
option(BUILD_SHARED_LIBS "Build gb28181 as a shared lib" OFF)
option(ENABLE_MSVC_MT "Enable MSVC Mt/Mtd lib" ON)
option(STRIP_SYMBOL "strip symbol on release build" ON)
option(FORCE_USER_AGENT "Force user agent" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(GenerateExportHeader)

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Installation directory for header files")
set(DEF_INSTALL_GB28181_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Onvif)
set(INSTALL_GB28181_CMAKE_DIR ${DEF_INSTALL_GB28181_CMAKE_DIR}
        CACHE PATH "Installation directory for cmake files")

add_library(${PROJECT_NAME})
add_library(gb28181::gb28181 ALIAS ${PROJECT_NAME})

generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/exports/gb28181/exports.h)

if (BUILD_SHARED_LIBS)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
            "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}" isSystemDir)
    if ("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}")
    endif ("${isSystemDir}" STREQUAL "-1")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            VERSION ${PROJECT_VERSION}          # 完整版本号
            SOVERSION ${PROJECT_VERSION_MAJOR}  # 主版本号
    )
    if (WIN32)
        target_compile_options(${PROJECT_NAME} PRIVATE /wd4251 /wd4275)
    endif ()
endif ()

add_library(${PROJECT_NAME}_interface INTERFACE)

if (WIN32)
    if (MSVC)
        target_compile_options(${PROJECT_NAME}_interface INTERFACE "/utf-8")
        target_compile_options(${PROJECT_NAME}_interface INTERFACE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        target_compile_options(${PROJECT_NAME}_interface INTERFACE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif ()
else ()
    target_compile_options(${PROJECT_NAME}_interface INTERFACE
            "-fPIC"
            "-Wall"
            "-Wextra"
            "-Wno-unused-function"
            "-Wno-unused-parameter"
            "-Wno-unused-variable"
            "-Wno-error=extra"
            "-Wno-error=missing-field-initializers"
            "-Wno-error=type-limits"
            "-Wno-comment")
endif ()

file(GLOB GB28181_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${GB28181_SRC})
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/exports>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
add_subdirectory(3rdpart)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_interface)
if(FORCE_USER_AGENT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DFORCE_USER_AGENT)
endif ()
find_package(Iconv REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)

if (NOT TARGET ZLToolKit)
    find_package(ZLToolKit QUIET)
    if (NOT ZLToolKit_FOUND)
        include(FetchContent)
        FetchContent_Declare(ZLToolKit
                GIT_REPOSITORY https://gitee.com/xia-chu/ZLToolKit.git
                GIT_TAG master  # 指定分支
                CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies
                -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
                -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
                -DENABLE_MSVC_MT=${ENABLE_MSVC_MT}
                -DSOCKET_DEFAULT_BUF_SIZE=${SOCKET_DEFAULT_BUF_SIZE}
                -DENABLE_OPENSSL=ON
        )
        FetchContent_MakeAvailable(ZLToolKit)
        add_library(ZLToolKit ALIAS ZLToolKit_static)
        find_package(OpenSSL QUIET)
        if (OPENSSL_FOUND)
            target_compile_definitions(${PROJECT_NAME} PUBLIC -DENABLE_OPENSSL)
            target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::Crypto)
        endif ()
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            target_compile_definitions(${PROJECT_NAME} PUBLIC -DWIN32_LEAN_AND_MEAN)
        endif ()
    endif ()
endif ()
if (NOT TARGET ZLToolKit)
    message(FATAL_ERROR "not found ZLToolKit")
    return()
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC ZLToolKit)

if (NOT TARGET tinyxml2)
    add_subdirectory(3rdpart/tinyxml2)
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC tinyxml2)
target_link_libraries(${PROJECT_NAME} PRIVATE ireader_sip )

if(BUILD_SHARED_LIBS AND CMAKE_BUILD_TYPE STREQUAL "Release" AND UNIX AND STRIP_SYMBOL)
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:${PROJECT_NAME}> "$<TARGET_FILE:${PROJECT_NAME}>.sym"
            COMMAND objcopy --strip-all $<TARGET_FILE:${PROJECT_NAME}>
            COMMAND objcopy --add-gnu-debuglink="$<TARGET_FILE:${PROJECT_NAME}>.sym" $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Stripping symbols from ${PROJECT_NAME} and saving"
    )
endif ()

if (NOT TARGET ZLToolKit_static)
install(TARGETS ${PROJECT_NAME} ZLToolKit ireader_sip gb28181_interface
        EXPORT gb28181Targets
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/exports/exports.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/gb28181)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

include(CMakePackageConfigHelpers)
# ... for the install tree
configure_package_config_file(gb28181Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/gb28181Config.cmake
        INSTALL_DESTINATION ${INSTALL_GB28181_CMAKE_DIR}
)

# version
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/gb28181ConfigVersion.cmake
        VERSION ${Gb28181_VERSION}
        COMPATIBILITY SameMajorVersion
)

# Install the TrantorConfig.cmake and TrantorConfigVersion.cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/gb28181Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/gb28181ConfigVersion.cmake"
        DESTINATION "${INSTALL_GB28181_CMAKE_DIR}"
        COMPONENT dev
)

# Install the export set for use with the install-tree
install(
        EXPORT gb28181Targets
        DESTINATION "${INSTALL_GB28181_CMAKE_DIR}"
        NAMESPACE gb28181::
        COMPONENT dev
)
endif ()


if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    add_executable(gb28181_exe main.cpp)
    target_link_libraries(gb28181_exe -Wl,--start-group ireader_sip ${PROJECT_NAME} -Wl,--end-group)
endif ()

cmake_minimum_required(VERSION 3.24)

project(DoziMastering VERSION 0.14.0 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures")

option(DOZI_BUILD_APP "Build the Dozi Mastering desktop application" ON)
option(DOZI_BUILD_TESTS "Build Dozi Mastering tests" ON)
option(DOZI_BUILD_PROBE "Build the command-line mastering probe" ON)
set(JUCE_SOURCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(DOZI_BUILD_APP)
    if(JUCE_SOURCE_DIR)
        add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.10
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(juce)
    endif()
endif()

add_subdirectory(src/core)

if(DOZI_BUILD_APP)
    add_subdirectory(src/mastering_app)
endif()

if(DOZI_BUILD_PROBE)
    add_subdirectory(src/mastering_probe)
endif()

if(DOZI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

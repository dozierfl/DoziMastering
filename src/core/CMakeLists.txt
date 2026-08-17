find_path(SNDFILE_INCLUDE_DIR sndfile.h HINTS /opt/homebrew/include /usr/local/include REQUIRED)
find_library(SNDFILE_LIBRARY sndfile HINTS /opt/homebrew/lib /usr/local/lib REQUIRED)
find_path(EBUR128_INCLUDE_DIR ebur128.h HINTS /opt/homebrew/include /usr/local/include REQUIRED)
find_library(EBUR128_LIBRARY ebur128 HINTS /opt/homebrew/lib /usr/local/lib REQUIRED)
find_path(LAME_INCLUDE_DIR lame/lame.h HINTS /opt/homebrew/opt/lame/include /usr/local/include REQUIRED)
find_library(LAME_LIBRARY mp3lame HINTS /opt/homebrew/opt/lame/lib /usr/local/lib REQUIRED)

add_library(dozi_mastering_core STATIC
    src/MasteringAnalyzer.cpp
    src/MasteringDecisionEngine.cpp
    src/MasteringDsp.cpp
    src/MasteringHealth.cpp
    src/MasteringMp3Encoder.cpp
    src/MasteringProjectStore.cpp
    src/MasteringReference.cpp
    src/MasteringRenderer.cpp
    src/MasteringReport.cpp
    src/MasteringStereoProcessor.cpp
    src/MasteringStyles.cpp
    src/MasteringTargets.cpp
    src/MasteringVersionStore.cpp)

add_library(Dozi::Core ALIAS dozi_mastering_core)
target_include_directories(dozi_mastering_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SNDFILE_INCLUDE_DIR}"
    "${EBUR128_INCLUDE_DIR}"
    "${LAME_INCLUDE_DIR}")
target_link_libraries(dozi_mastering_core PUBLIC
    "${SNDFILE_LIBRARY}"
    "${EBUR128_LIBRARY}"
    "${LAME_LIBRARY}")
target_compile_options(dozi_mastering_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

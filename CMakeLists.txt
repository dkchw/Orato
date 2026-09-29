cmake_minimum_required(VERSION 3.16)
project(orato VERSION 1.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    Network
    Svg
)

# -----------------------------------------------------------------------------
# Whisper.cpp integration
# -----------------------------------------------------------------------------
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Do not build tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Do not build examples" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/CMakeLists.txt")
    message(STATUS "Using vendored whisper.cpp from third_party/whisper.cpp")
    add_subdirectory(third_party/whisper.cpp EXCLUDE_FROM_ALL)
else()
    message(STATUS "Fetching whisper.cpp via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        whisper
        GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(whisper)
endif()

# -----------------------------------------------------------------------------
# Application Sources
# -----------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/models/SessionData.h
    src/audio/AudioUtils.h
    src/audio/AudioUtils.cpp
    src/audio/AudioRecorder.h
    src/audio/AudioRecorder.cpp
    src/audio/AudioPlayer.h
    src/audio/AudioPlayer.cpp
    src/whisper/WhisperEngine.h
    src/whisper/WhisperEngine.cpp
    src/whisper/ModelManager.h
    src/whisper/ModelManager.cpp
    src/tts/PocketTTSEngine.h
    src/tts/PocketTTSEngine.cpp
    src/session/SessionManager.h
    src/session/SessionManager.cpp
    src/widgets/AudioLevelMeter.h
    src/widgets/AudioLevelMeter.cpp
    src/widgets/WaveformWidget.h
    src/widgets/WaveformWidget.cpp
    src/widgets/SentenceListView.h
    src/widgets/SentenceListView.cpp
    src/widgets/MarkdownNoteEditor.h
    src/widgets/MarkdownNoteEditor.cpp
    src/widgets/ModelManagerDialog.h
    src/widgets/ModelManagerDialog.cpp
    src/widgets/ConfigDialog.h
    src/widgets/ConfigDialog.cpp
    src/widgets/SessionItemDelegate.h
    src/widgets/SessionItemDelegate.cpp
    resources/resources.qrc
)

add_executable(orato ${SOURCES})

target_include_directories(orato PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/ggml/include
)

target_link_libraries(orato PRIVATE
    whisper
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Qt6::Svg
)

install(TARGETS orato DESTINATION bin)

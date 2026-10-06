cmake_minimum_required(VERSION 3.22)
# Must be set BEFORE project(): afterwards CMake ignores it and builds for the host OS version.
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0 CACHE STRING "Minimum macOS version" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
project(OscillaFX VERSION 0.4.0 LANGUAGES C CXX OBJCXX)

# Keep build-machine paths (and user names) out of the binaries (__FILE__ in asserts and logs).
add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/JUCE)
add_subdirectory(engine)

# App icon master (1024x1024 RGBA). Falls back to the pre-rename icon so the build never breaks.
set(OSCILLA_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/OscillaFX.png")
if(NOT EXISTS "${OSCILLA_APP_ICON}")
    message(WARNING "packaging/icon/OscillaFX.png missing; using packaging/icon/Oscilla.png")
    set(OSCILLA_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/Oscilla.png")
endif()

juce_add_gui_app(OscillaFX
    PRODUCT_NAME "OscillaFX"
    COMPANY_NAME "OscillaFX"
    BUNDLE_ID com.oscilla.app
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "OscillaFX captures the OscillaFX Audio virtual device to apply sound enhancement."
    PLIST_TO_MERGE "<plist><dict><key>NSSupportsAutomaticGraphicsSwitching</key><true/></dict></plist>"
    ICON_BIG "${OSCILLA_APP_ICON}")

file(GLOB_RECURSE OSCILLA_APP_SOURCES CONFIGURE_DEPENDS
    app/Source/*.cpp app/Source/*.mm app/Source/*.h)
target_sources(OscillaFX PRIVATE ${OSCILLA_APP_SOURCES})
target_include_directories(OscillaFX PRIVATE app/Source)

target_compile_definitions(OscillaFX PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OscillaFX,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OscillaFX,JUCE_VERSION>")

target_link_libraries(OscillaFX PRIVATE
    oscilla_dsp
    juce::juce_audio_devices juce::juce_audio_utils juce::juce_gui_extra juce::juce_opengl
    "-framework CoreAudio" "-framework AudioToolbox"
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# The target was called Oscilla before the rename; keep `--target Oscilla` working.
add_custom_target(Oscilla DEPENDS OscillaFX)

# ==== bundled presets =====================================
# Copies OscillaFX's own presets (presets/, written by engine make_preset) and the FxSound bonus
# .fac presets into OscillaFX.app/Contents/Resources/Presets (see core/Presets.cpp).
set(OSCILLA_BONUS_PRESET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/presets_fxsound"
    CACHE PATH "Directory with .fac presets bundled into the app")
file(GLOB OSCILLA_BONUS_PRESETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/presets/*.fac" "${OSCILLA_BONUS_PRESET_DIR}/*.fac")
if(OSCILLA_BONUS_PRESETS)
    add_custom_command(TARGET OscillaFX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Resources/Presets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OSCILLA_BONUS_PRESETS}
            "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Resources/Presets"
        VERBATIM)
else()
    message(WARNING "No .fac presets in presets/ or ${OSCILLA_BONUS_PRESET_DIR}; app bundle ships without presets")
endif()
# ==== end bundled presets =====================================================

# ==== crash watchdog + login item  ===================
# helper/OscillaWatchdog -> OscillaFX.app/Contents/Helpers (spawned by audio/CrashWatchdog.cpp).
add_subdirectory(helper)
add_dependencies(OscillaFX OscillaWatchdog)
add_custom_command(TARGET OscillaFX POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Helpers"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:OscillaWatchdog>"
        "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Helpers/OscillaWatchdog"
    VERBATIM)
target_link_libraries(OscillaFX PRIVATE "-framework ServiceManagement" "-framework AVFoundation") # core/LoginItem.mm, core/MicrophoneAccess.mm
# ==== end crash watchdog + login item =========================================

# ==== gallery images  =======================================
# assets/gallery/* -> OscillaFX.app/Contents/Resources/Gallery. Missing or empty dir is fine.
file(GLOB OSCILLA_GALLERY_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/gallery/*")
if(OSCILLA_GALLERY_FILES)
    add_custom_command(TARGET OscillaFX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Resources/Gallery"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OSCILLA_GALLERY_FILES}
            "$<TARGET_BUNDLE_CONTENT_DIR:OscillaFX>/Resources/Gallery"
        VERBATIM)
endif()
# ==== end gallery images ======================================================

# ==== engine_selftest  ==============================
# Headless checks: device listing, ProfileStore, ring buffer, drift resampler, DSP mapping.
# Builds every non-UI app source so they compile even before the UI target links.
juce_add_console_app(engine_selftest PRODUCT_NAME "engine_selftest")
file(GLOB OSCILLA_SELFTEST_SOURCES CONFIGURE_DEPENDS
    app/Source/audio/*.cpp app/Source/core/*.cpp app/Source/core/*.mm app/Source/profiles/*.cpp)
target_sources(engine_selftest PRIVATE app/Tests/EngineSelfTest.cpp ${OSCILLA_SELFTEST_SOURCES})
target_include_directories(engine_selftest PRIVATE app/Source)
target_compile_definitions(engine_selftest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(engine_selftest PRIVATE
    oscilla_dsp juce::juce_core juce::juce_events
    "-framework CoreAudio" "-framework AudioToolbox" "-framework AppKit" "-framework ServiceManagement" "-framework AVFoundation"
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
enable_testing()
add_test(NAME engine_selftest COMMAND engine_selftest)
# ==== end engine_selftest =====================================================

# ==== backend: gallery import + prefs =================
# core/ImageValidation.mm sniffs image content with ImageIO.
target_link_libraries(OscillaFX PRIVATE "-framework ImageIO")
target_link_libraries(engine_selftest PRIVATE "-framework ImageIO")
target_compile_definitions(engine_selftest PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1) # tests pump the message loop
# ==== end backend ==========================================================

# ==== UI preview  ================================================
# cmake -S . -B build-ui -DOSCILLA_UI_STUB=ON && cmake --build build-ui --target OscillaFX
# Swaps the real CoreAudio engine for app/Source/ui/dev/ControllerStub.cpp (fake signal) and the
# microphone permission check for ui/dev/MicrophoneAccessStub.cpp (OSCILLA_STUB_MIC_DENIED=1 -> denied).
if(OSCILLA_UI_STUB)
    target_compile_definitions(OscillaFX PRIVATE OSCILLA_UI_STUB=1)
    # Real sources left out of the UI preview build (stand-ins live in app/Source/ui/dev).
    set(OSCILLA_UI_STUB_EXCLUDED app/Source/audio/AudioEngine.cpp app/Source/core/MicrophoneAccess.mm)
    set_source_files_properties(${OSCILLA_UI_STUB_EXCLUDED} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()
# ==== end UI preview ==========================================================

# ==== crt_trace_test  ===========================================
# Offline check of the CRT's wave / X-Y trace generation (ui/crt/CrtTraces): a fuzzy trace fails.
juce_add_console_app(crt_trace_test PRODUCT_NAME "crt_trace_test")
target_sources(crt_trace_test PRIVATE app/Tests/CrtTraceTest.cpp app/Source/ui/crt/CrtTraces.cpp)
target_include_directories(crt_trace_test PRIVATE app/Source)
target_compile_definitions(crt_trace_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(crt_trace_test PRIVATE
    juce::juce_gui_basics
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME crt_trace_test COMMAND crt_trace_test)
# ==== end crt_trace_test ======================================================

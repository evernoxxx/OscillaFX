cmake_minimum_required(VERSION 3.20)
project(OscillaDriver C)

set(OSCILLA_DRIVER_NAME "Oscilla" CACHE STRING "bundle/executable name")
set(OSCILLA_DEVICE_NAME "OscillaFX Audio" CACHE STRING "device name shown in CoreAudio")
set(OSCILLA_BUNDLE_ID "com.oscilla.audio" CACHE STRING "CFBundleIdentifier")
set(OSCILLA_CHANNELS 2 CACHE STRING "channel count")
set(OSCILLA_SAMPLE_RATES "44100, 48000, 88200, 96000, 176400, 192000" CACHE STRING "supported rates")

# Thin wrapper: build.sh is the source of truth (bundle layout + ad-hoc signing).
add_custom_target(oscilla_driver ALL
  COMMAND ${CMAKE_COMMAND} -E env
    DRIVER_NAME=${OSCILLA_DRIVER_NAME} DEVICE_NAME=${OSCILLA_DEVICE_NAME}
    BUNDLE_ID=${OSCILLA_BUNDLE_ID} CHANNELS=${OSCILLA_CHANNELS}
    "SAMPLE_RATES=${OSCILLA_SAMPLE_RATES}" BUILD_DIR=${CMAKE_BINARY_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/build.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

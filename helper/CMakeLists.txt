# Crash watchdog: restores the system output if OscillaFX dies while routing. No JUCE.
add_executable(OscillaWatchdog OscillaWatchdog.cpp)
target_compile_options(OscillaWatchdog PRIVATE -Wall -Wextra)
target_link_libraries(OscillaWatchdog PRIVATE "-framework CoreAudio" "-framework CoreFoundation")

#pragma once

#include "core/DspSettings.h"
#include <functional>
#include <juce_core/juce_core.h>

struct OutputDevice
{
    juce::String uid;  // CoreAudio kAudioDevicePropertyDeviceUID, stable across reconnects
    juce::String name;
};

// Captures the "OscillaFX Audio" virtual device, runs DfxDsp, renders to the real output device.
// All public methods are called from the message thread unless noted.
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    bool start();               // false + lastError() if the virtual device is missing
    void stop();                // restores the user's real device as system default
    juce::String lastError() const;

    void applySettings (const DspSettings&); // lock-free hand-off to the audio thread

    juce::Array<OutputDevice> outputDevices() const;  // excludes the virtual device
    OutputDevice currentOutputDevice() const;
    void setOutputDevice (const juce::String& uid);

    // Fired on the message thread when the real output changes (plug/unplug, user switch).
    std::function<void (const OutputDevice&)> onOutputDeviceChanged;

    // Visualiser taps, safe to call from the UI thread at 60 Hz.
    void getSpectrum (float* bands10) const;
    int  getScopeSamples (float* left, float* right, int maxSamples) const; // most recent post-DSP samples

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

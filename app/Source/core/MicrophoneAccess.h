#pragma once

#include <functional>

// Microphone (TCC) permission: capturing the virtual device needs it, and without it the capture
// delivers silence, so OscillaFX must not take over the system output before it is granted.
namespace MicrophoneAccess
{
enum class Status { granted, undetermined, denied }; // denied includes "restricted"

Status status();

// Shows the system prompt; `onResult` runs later on the message thread.
void request (std::function<void (bool granted)> onResult);

void openSystemSettings(); // Privacy & Security > Microphone
} // namespace MicrophoneAccess

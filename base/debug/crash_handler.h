#ifndef BASE_DEBUG_CRASH_HANDLER_H_
#define BASE_DEBUG_CRASH_HANDLER_H_

#include <cstddef>

namespace base {
namespace debug {

// Maximum number of recent log lines kept in the crash-safe ring buffer.
// This buffer is pre-allocated in BSS (no heap) so it's always accessible
// even when the heap is corrupted during a crash.
constexpr size_t kCrashRingSize = 512;
constexpr size_t kCrashLineMax = 256;

// Open crash log file. Called once at startup before any crash.
// The file descriptor is stored internally and used by the signal handler.
void OpenCrashLogFile(const char* path);

// Append a log line to the crash-safe ring buffer.
// This is called from LogMessage::~LogMessage() during normal execution.
// It copies the string into a pre-allocated fixed-size slot.
void AppendCrashLog(const char* msg, size_t len);

// Install signal handlers for SIGSEGV, SIGABRT, SIGFPE, SIGILL.
// Must be called after OpenCrashLogFile().
void InstallCrashHandlers();

}  // namespace debug
}  // namespace base

#endif  // !BASE_DEBUG_CRASH_HANDLER_H_

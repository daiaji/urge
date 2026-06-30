#ifndef BASE_PLATFORM_CONSOLE_H_
#define BASE_PLATFORM_CONSOLE_H_

namespace platform {

// Ensure a console is available for stdout/stderr output.
// On Windows GUI apps, this allocates a console and redirects stdio.
// On other platforms, this is a no-op (terminal is always available).
void EnsureConsole();

// Show or hide the console window (Windows only).
// On other platforms, this is a no-op.
void SetConsoleVisible(bool visible);

}  // namespace platform

#endif  // BASE_PLATFORM_CONSOLE_H_

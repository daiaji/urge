#include "base/platform/console.h"

#include "base/buildflags/build.h"

#if defined(OS_WIN)
#include <cstdio>
#include <windows.h>
#endif

namespace platform {

void EnsureConsole() {
#if defined(OS_WIN)
  if (::GetConsoleWindow())
    return;

  if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
    ::AllocConsole();
    ::SetConsoleCP(CP_UTF8);
    ::SetConsoleOutputCP(CP_UTF8);
  }

  // CRT stdout/stderr handles are invalid for WIN32 GUI apps until freopen
  std::freopen("CONIN$", "rb", stdin);
  std::freopen("CONOUT$", "wb", stdout);
  std::freopen("CONOUT$", "wb", stderr);
#endif
}

void SetConsoleVisible(bool visible) {
#if defined(OS_WIN)
  HWND hwnd = ::GetConsoleWindow();
  if (hwnd)
    ::ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
#endif
}

}  // namespace platform

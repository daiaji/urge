#include "base/debug/crash_handler.h"

#include "base/buildflags/build.h"

#if defined(OS_WIN)
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <signal.h>
#elif defined(OS_LINUX)
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <fcntl.h>
#include <ucontext.h>
#else
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cstdint>

namespace base {
namespace debug {

// ---------------------------------------------------------------------------
// Crash-safe ring buffer (pre-allocated in BSS, no heap)
// ---------------------------------------------------------------------------

namespace {

static char g_ring_buffer[kCrashRingSize][kCrashLineMax];
static int g_ring_lens[kCrashRingSize];
static std::atomic<int> g_ring_write_cursor{0};
static std::atomic<int> g_ring_total{0};
static int g_crash_fd = -1;

// Saved old signal actions for chaining to previous handlers (e.g. Ruby's sigsegv)
#if !defined(OS_WIN)
static constexpr int kMaxSignal = 64;
static struct sigaction g_old_sigactions[kMaxSignal];
static bool g_sigactions_saved[kMaxSignal] = {};
#endif

#if !defined(OS_WIN)
static int g_orig_stderr = -1;
#endif

#if defined(OS_WIN)
static const int kInvalidFd = -1;
#else
static const int kInvalidFd = -1;
#endif

const char* SignalName(int sig) {
  switch (sig) {
#if !defined(OS_WIN)
    case SIGSEGV:  return "SIGSEGV (Segmentation Fault)";
    case SIGFPE:   return "SIGFPE (Floating-point Exception)";
    case SIGILL:   return "SIGILL (Illegal Instruction)";
    case SIGBUS:   return "SIGBUS (Bus Error)";
#endif
    case SIGABRT:  return "SIGABRT (Aborted)";
    default:       return "Unknown Signal";
  }
}

#if defined(OS_WIN)
const char* ExceptionCodeName(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:          return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:     return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:                return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:     return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:        return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_ILLEGAL_INSTRUCTION:       return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:        return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_STACK_OVERFLOW:            return "EXCEPTION_STACK_OVERFLOW";
    default:                                  return "Unknown Exception";
  }
}
#endif

void WriteLiteral(int fd, const char* str) {
  size_t len = strlen(str);
  size_t written = 0;
  while (written < len) {
#if defined(OS_WIN)
    int ret = _write(fd, str + written, static_cast<unsigned>(len - written));
#else
    ssize_t ret = write(fd, str + written, len - written);
#endif
    if (ret <= 0) break;
    written += static_cast<size_t>(ret);
  }
}

void WriteDecimal(int fd, int val) {
  char buf[12];
  int pos = sizeof(buf) - 1;
  buf[pos] = '\0';
  bool neg = val < 0;
  if (neg) val = -val;
  do {
    buf[--pos] = '0' + (val % 10);
    val /= 10;
  } while (val > 0);
  if (neg) buf[--pos] = '-';
  WriteLiteral(fd, buf + pos);
}

void WriteHex(int fd, uintptr_t val) {
  char buf[19] = "0x";
  static const char hex[] = "0123456789abcdef";
  for (int i = 15; i >= 0; --i) {
    buf[2 + i] = hex[val & 0xf];
    val >>= 4;
  }
  int start = 2;
  while (start < 17 && buf[start] == '0') ++start;
  if (start == 17) buf[17] = '0';
  WriteLiteral(fd, buf + start - 2);
}

void WriteCrashHeader(int fd, int sig, const char* sig_name) {
#if defined(OS_WIN)
  SYSTEMTIME st;
  GetLocalTime(&st);
  WriteLiteral(fd, "\n--- ");
  WriteDecimal(fd, st.wYear);
  WriteLiteral(fd, "-");
  WriteDecimal(fd, st.wMonth);
  WriteLiteral(fd, "-");
  WriteDecimal(fd, st.wDay);
  WriteLiteral(fd, " ");
  WriteDecimal(fd, st.wHour);
  WriteLiteral(fd, ":");
  WriteDecimal(fd, st.wMinute);
  WriteLiteral(fd, ":");
  WriteDecimal(fd, st.wSecond);
  WriteLiteral(fd, " ---\n");
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME_COARSE, &ts);
  WriteLiteral(fd, "\n--- ");
  WriteDecimal(fd, static_cast<int>(ts.tv_sec));
  WriteLiteral(fd, " ---\n");
#endif
  WriteLiteral(fd,
    "================================================================\n"
    "  URGE CRASH - Unhandled signal detected\n"
    "================================================================\n");
  WriteLiteral(fd, "  Signal:    "); WriteLiteral(fd, sig_name); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "  Signal #:  "); WriteDecimal(fd, sig); WriteLiteral(fd, "\n");
}

void WriteRecentLogs(int fd) {
  int total = g_ring_total.load(std::memory_order_acquire);
  int count = std::min(total, static_cast<int>(kCrashRingSize));
  if (count <= 0)
    return;

  WriteLiteral(fd, "\n  Recent engine log lines:\n");
  int start = total - count;
  for (int i = 0; i < count; ++i) {
    int idx = (start + i) % static_cast<int>(kCrashRingSize);
    int len = g_ring_lens[idx];
    if (len <= 0)
      continue;
#if defined(OS_WIN)
    _write(fd, g_ring_buffer[idx], static_cast<unsigned>(len));
    _write(fd, "\n", 1);
#else
    write(fd, g_ring_buffer[idx], static_cast<size_t>(len));
    write(fd, "\n", 1);
#endif
  }
}

void PublishRingTotal(int total) {
  int published = g_ring_total.load(std::memory_order_relaxed);
  while (published < total &&
         !g_ring_total.compare_exchange_weak(published, total,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)) {
  }
}

}  // namespace

void OpenCrashLogFile(const char* path) {
  if (g_crash_fd >= 0) {
#if defined(OS_WIN)
    _close(g_crash_fd);
#else
    close(g_crash_fd);
#endif
    g_crash_fd = -1;
  }
#if defined(OS_WIN)
  g_crash_fd = _open(path, _O_WRONLY | _O_CREAT | _O_APPEND, _S_IREAD | _S_IWRITE);
#else
  g_crash_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (g_orig_stderr < 0)
    g_orig_stderr = dup(STDERR_FILENO);
#endif
}

void AppendCrashLog(const char* msg, size_t len) {
  int total = g_ring_write_cursor.fetch_add(1, std::memory_order_relaxed);
  int idx = total % static_cast<int>(kCrashRingSize);
  size_t copy_len = std::min(len, kCrashLineMax - 1);
  g_ring_lens[idx] = 0;
  memcpy(g_ring_buffer[idx], msg, copy_len);
  g_ring_buffer[idx][copy_len] = '\0';
  g_ring_lens[idx] = static_cast<int>(copy_len);
  PublishRingTotal(total + 1);
}

// ---------------------------------------------------------------------------
// Signal / exception handler
// ---------------------------------------------------------------------------

namespace {

void FlushAndRaise(int sig) {
  if (g_crash_fd >= 0) {
#if defined(OS_WIN)
    _close(g_crash_fd);
#else
    close(g_crash_fd);
#endif
    g_crash_fd = -1;
  }
#if defined(OS_WIN)
  _exit(sig);
#else
  signal(sig, SIG_DFL);
  raise(sig);
#endif
}

#if !defined(OS_WIN)

void DumpRegisters(int fd, ucontext_t* uc) {
#if defined(OS_LINUX) && defined(__x86_64__)
  auto* mctx = &uc->uc_mcontext;
  WriteLiteral(fd, "\n  Registers (x86_64):\n");
  WriteLiteral(fd, "    RAX: "); WriteHex(fd, mctx->gregs[REG_RAX]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RBX: "); WriteHex(fd, mctx->gregs[REG_RBX]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RCX: "); WriteHex(fd, mctx->gregs[REG_RCX]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RDX: "); WriteHex(fd, mctx->gregs[REG_RDX]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RSI: "); WriteHex(fd, mctx->gregs[REG_RSI]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RDI: "); WriteHex(fd, mctx->gregs[REG_RDI]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RBP: "); WriteHex(fd, mctx->gregs[REG_RBP]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RSP: "); WriteHex(fd, mctx->gregs[REG_RSP]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RIP: "); WriteHex(fd, mctx->gregs[REG_RIP]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R8:  "); WriteHex(fd, mctx->gregs[REG_R8]);  WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R9:  "); WriteHex(fd, mctx->gregs[REG_R9]);  WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R10: "); WriteHex(fd, mctx->gregs[REG_R10]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R11: "); WriteHex(fd, mctx->gregs[REG_R11]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R12: "); WriteHex(fd, mctx->gregs[REG_R12]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R13: "); WriteHex(fd, mctx->gregs[REG_R13]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R14: "); WriteHex(fd, mctx->gregs[REG_R14]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R15: "); WriteHex(fd, mctx->gregs[REG_R15]); WriteLiteral(fd, "\n");
#elif defined(OS_LINUX) && defined(__aarch64__)
  auto* mctx = &uc->uc_mcontext;
  WriteLiteral(fd, "\n  Registers (AArch64):\n");
  for (int i = 0; i < 30; ++i) {
    WriteLiteral(fd, "    X"); WriteDecimal(fd, i);
    WriteLiteral(fd, ": "); WriteHex(fd, mctx->regs[i]); WriteLiteral(fd, "\n");
  }
  WriteLiteral(fd, "    LR: "); WriteHex(fd, mctx->regs[30]); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    SP: "); WriteHex(fd, mctx->sp);      WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    PC: "); WriteHex(fd, mctx->pc);      WriteLiteral(fd, "\n");
#else
  (void)fd;
  (void)uc;
#endif
}

static void WriteCrashDumpToFd(int fd, int sig, siginfo_t* info,
                               void* ucontext) {
  WriteCrashHeader(fd, sig, SignalName(sig));
  if (info) {
    WriteLiteral(fd, "  SI_CODE:   ");
    WriteDecimal(fd, info->si_code);
    WriteLiteral(fd, "\n");
    if (sig == SIGSEGV) {
      WriteLiteral(fd, "  Fault Addr: ");
      WriteHex(fd, reinterpret_cast<uintptr_t>(info->si_addr));
      WriteLiteral(fd, "\n");
    }
  }
  if (ucontext)
    DumpRegisters(fd, static_cast<ucontext_t*>(ucontext));
  WriteLiteral(fd, "\n  Backtrace unavailable in signal handler.\n");
}

void OnCrashSignal(int sig, siginfo_t* info, void* ucontext) {
  const int crash_fd = g_crash_fd >= 0 ? g_crash_fd : STDERR_FILENO;
  WriteCrashDumpToFd(crash_fd, sig, info, ucontext);
  WriteRecentLogs(crash_fd);
  if (g_orig_stderr >= 0 && g_orig_stderr != crash_fd)
    WriteCrashDumpToFd(g_orig_stderr, sig, info, ucontext);

  if (g_crash_fd >= 0)
    dup2(g_crash_fd, STDERR_FILENO);

  // Chain to saved old handler (e.g. Ruby's sigsegv).
  struct sigaction* old = &g_old_sigactions[sig];
  if (g_sigactions_saved[sig] && old->sa_sigaction &&
      old->sa_handler != SIG_DFL && old->sa_handler != SIG_IGN) {
    if (old->sa_flags & SA_SIGINFO)
      old->sa_sigaction(sig, info, ucontext);
    else
      old->sa_handler(sig);
  }

  FlushAndRaise(sig);
}

#endif  // !OS_WIN

#if defined(OS_WIN)

constexpr DWORD kDbgPrintExceptionC = 0x40010006;
constexpr DWORD kDbgPrintExceptionWideC = 0x4001000A;

void DumpWinRegisters(int fd, PCONTEXT ctx) {
  WriteLiteral(fd, "\n  Registers (x86_64):\n");
  WriteLiteral(fd, "    RAX: "); WriteHex(fd, ctx->Rax); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RBX: "); WriteHex(fd, ctx->Rbx); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RCX: "); WriteHex(fd, ctx->Rcx); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RDX: "); WriteHex(fd, ctx->Rdx); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RSI: "); WriteHex(fd, ctx->Rsi); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RDI: "); WriteHex(fd, ctx->Rdi); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RBP: "); WriteHex(fd, ctx->Rbp); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RSP: "); WriteHex(fd, ctx->Rsp); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    RIP: "); WriteHex(fd, ctx->Rip); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R8:  "); WriteHex(fd, ctx->R8);  WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R9:  "); WriteHex(fd, ctx->R9);  WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R10: "); WriteHex(fd, ctx->R10); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R11: "); WriteHex(fd, ctx->R11); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R12: "); WriteHex(fd, ctx->R12); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R13: "); WriteHex(fd, ctx->R13); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R14: "); WriteHex(fd, ctx->R14); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "    R15: "); WriteHex(fd, ctx->R15); WriteLiteral(fd, "\n");
}

#pragma warning(push)
#pragma warning(disable: 4702)
static LONG CALLBACK WinVectoredHandler(PEXCEPTION_POINTERS ep) {
  EXCEPTION_RECORD* er = ep->ExceptionRecord;

  // Ignore non-fatal exceptions that are expected (e.g. thread naming)
  if (er->ExceptionCode == 0x406D1388)  // MS_VC_EXCEPTION (SetThreadDescription)
    return EXCEPTION_CONTINUE_SEARCH;
  if (er->ExceptionCode == kDbgPrintExceptionC ||
      er->ExceptionCode == kDbgPrintExceptionWideC)
    return EXCEPTION_CONTINUE_SEARCH;

  int fd = g_crash_fd >= 0 ? g_crash_fd : -1;
  if (fd < 0)
    fd = _fileno(stderr);

  WriteCrashHeader(fd, static_cast<int>(er->ExceptionCode),
                   ExceptionCodeName(er->ExceptionCode));
  WriteLiteral(fd, "  Flags:     "); WriteHex(fd, er->ExceptionFlags); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "  Fault Addr: ");
  WriteHex(fd, reinterpret_cast<uintptr_t>(er->ExceptionAddress));
  WriteLiteral(fd, "\n");

  if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
    WriteLiteral(fd, "  Access:    ");
    WriteLiteral(fd, er->ExceptionInformation[0] ? "WRITE" : "READ");
    WriteLiteral(fd, "\n");
    WriteLiteral(fd, "  Target:    ");
    WriteHex(fd, static_cast<uintptr_t>(er->ExceptionInformation[1]));
    WriteLiteral(fd, "\n");
  }

  if (ep->ContextRecord)
    DumpWinRegisters(fd, ep->ContextRecord);

  void* callstack[128];
  WORD frames = CaptureStackBackTrace(0, 128, callstack, nullptr);
  WriteLiteral(fd, "\n  Backtrace ("); WriteDecimal(fd, frames); WriteLiteral(fd, " frames):\n");
  for (WORD i = 0; i < frames; ++i) {
    WriteHex(fd, reinterpret_cast<uintptr_t>(callstack[i]));
    WriteLiteral(fd, "\n");
  }

  // _exit() will be called; return value is never used.
  LONG result = EXCEPTION_CONTINUE_SEARCH;
  FlushAndRaise(0);
  return result;
}
#pragma warning(pop)

void WinAbortHandler(int) {
  int fd = g_crash_fd >= 0 ? g_crash_fd : _fileno(stderr);
  WriteCrashHeader(fd, SIGABRT, SignalName(SIGABRT));

  void* callstack[128];
  WORD frames = CaptureStackBackTrace(0, 128, callstack, nullptr);
  WriteLiteral(fd, "\n  Backtrace ("); WriteDecimal(fd, frames); WriteLiteral(fd, " frames):\n");
  for (WORD i = 0; i < frames; ++i) {
    WriteHex(fd, reinterpret_cast<uintptr_t>(callstack[i]));
    WriteLiteral(fd, "\n");
  }

  FlushAndRaise(SIGABRT);
}

#endif  // OS_WIN

}  // namespace

void InstallCrashHandlers() {
#if defined(OS_WIN)
  AddVectoredExceptionHandler(0, WinVectoredHandler);
  signal(SIGABRT, WinAbortHandler);
#else
  // Allocate an alternate signal stack so the signal handler runs safely
  // even if the crash was caused by a stack overflow.
  static stack_t alt_ss;
  static char* alt_stack_buf = static_cast<char*>(malloc(SIGSTKSZ * 4));
  if (alt_stack_buf) {
    alt_ss.ss_sp = alt_stack_buf;
    alt_ss.ss_size = SIGSTKSZ * 4;
    alt_ss.ss_flags = 0;
    sigaltstack(&alt_ss, nullptr);
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = OnCrashSignal;

  int signals[] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
  for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
    int sig = signals[i];
    struct sigaction old;
    memset(&old, 0, sizeof(old));
    sigaction(sig, &sa, &old);
    // Save the old handler if it's not our own (to avoid recursive chain)
    if (old.sa_sigaction != OnCrashSignal) {
      g_old_sigactions[sig] = old;
      g_sigactions_saved[sig] = true;
    }
  }
#endif
}

}  // namespace debug
}  // namespace base

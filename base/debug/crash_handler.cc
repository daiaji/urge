#include "base/debug/crash_handler.h"

#if defined(OS_WIN)
#include <windows.h>
#include <io.h>
#include <process.h>
#else
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#include <fcntl.h>
#include <ucontext.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <cstdint>

#if !defined(OS_WIN)
#include <pthread.h>
#endif

namespace base {
namespace debug {

// ---------------------------------------------------------------------------
// Crash-safe ring buffer (pre-allocated in BSS, no heap)
// ---------------------------------------------------------------------------

namespace {

static char g_ring_buffer[kCrashRingSize][kCrashLineMax];
static int g_ring_lens[kCrashRingSize];
static int g_ring_total = 0;
static int g_crash_fd = -1;

// Saved old signal actions for chaining to previous handlers (e.g. Ruby's sigsegv)
static struct sigaction g_old_sigactions[NSIG];
static bool g_sigactions_saved[NSIG] = {};

// Pump pipe + thread for tee-ing crash output to both crash.log and stderr.
// During crash handling, stderr is dup2'd to the pipe; a background thread
// reads from the pipe and writes to both g_crash_fd and the original stderr.
// This ensures Ruby's chained handler output appears in both destinations.
#if !defined(OS_WIN)
static int g_crash_pipe[2] = {-1, -1};
static int g_orig_stderr = -1;
static pthread_t g_pump_thread;
static std::atomic<bool> g_pump_running{false};

static void* CrashPumpThread(void*) {
  char buf[16384];
  while (true) {
    ssize_t n = read(g_crash_pipe[0], buf, sizeof(buf));
    if (n <= 0) break;
    if (g_crash_fd >= 0)
      write(g_crash_fd, buf, static_cast<size_t>(n));
    if (g_orig_stderr >= 0)
      write(g_orig_stderr, buf, static_cast<size_t>(n));
  }
  close(g_crash_pipe[0]);
  g_crash_pipe[0] = -1;
  return nullptr;
}

static void StartCrashPumpPipe() {
  if (g_pump_running.exchange(true))
    return;
  if (pipe(g_crash_pipe) < 0) {
    g_pump_running = false;
    return;
  }
  fcntl(g_crash_pipe[0], F_SETPIPE_SZ, 1048576);
  g_orig_stderr = dup(STDERR_FILENO);
  if (pthread_create(&g_pump_thread, nullptr, CrashPumpThread, nullptr) != 0) {
    close(g_crash_pipe[0]);
    close(g_crash_pipe[1]);
    g_crash_pipe[0] = g_crash_pipe[1] = -1;
    g_orig_stderr = -1;
    g_pump_running = false;
  }
  pthread_detach(g_pump_thread);
}
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
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME_COARSE, &ts);
  WriteLiteral(fd, "\n--- ");
  WriteDecimal(fd, static_cast<int>(ts.tv_sec));
  WriteLiteral(fd, " ---\n");
  WriteLiteral(fd,
    "================================================================\n"
    "  URGE CRASH - Unhandled signal detected\n"
    "================================================================\n");
  WriteLiteral(fd, "  Signal:    "); WriteLiteral(fd, sig_name); WriteLiteral(fd, "\n");
  WriteLiteral(fd, "  Signal #:  "); WriteDecimal(fd, sig); WriteLiteral(fd, "\n");
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
  // Start pump pipe + thread for tee-ing crash output to both destinations
  StartCrashPumpPipe();
#endif
}

void AppendCrashLog(const char* msg, size_t len) {
  int idx = g_ring_total % kCrashRingSize;
  size_t copy_len = std::min(len, kCrashLineMax - 1);
  memcpy(g_ring_buffer[idx], msg, copy_len);
  g_ring_buffer[idx][copy_len] = '\0';
  g_ring_lens[idx] = static_cast<int>(copy_len);
  ++g_ring_total;
}

// ---------------------------------------------------------------------------
// Signal / exception handler
// ---------------------------------------------------------------------------

namespace {

void DumpRingBuffer(int fd) {
  WriteLiteral(fd,
    "\n"
    "  Recent log messages (ring buffer):\n"
    "----------------------------------------------------------------\n");
  int total = g_ring_total;
  int ring_size = static_cast<int>(kCrashRingSize);
  int available = std::min(total, ring_size);
  int start = total > ring_size ? total % ring_size : 0;
  for (int i = 0; i < available; ++i) {
    int idx = (start + i) % ring_size;
    if (g_ring_lens[idx] > 0) {
#if defined(OS_WIN)
      _write(fd, g_ring_buffer[idx], static_cast<unsigned>(g_ring_lens[idx]));
#else
      write(fd, g_ring_buffer[idx], static_cast<size_t>(g_ring_lens[idx]));
#endif
      WriteLiteral(fd, "\n");
    }
  }
  WriteLiteral(fd,
    "================================================================\n"
    "  Crash log written.\n\n");
}

void FlushAndRaise(int sig) {
  fflush(NULL);
  // Close pipe write end to signal pump thread to drain and exit
#if !defined(OS_WIN)
  if (g_crash_pipe[1] >= 0) {
    close(g_crash_pipe[1]);
    g_crash_pipe[1] = -1;
  }
#endif
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
#if defined(__x86_64__)
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
#elif defined(__aarch64__)
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
  void* callstack[128];
  int frames = backtrace(callstack, 128);
  WriteLiteral(fd, "\n  Backtrace (");
  WriteDecimal(fd, frames);
  WriteLiteral(fd, " frames):\n");
  backtrace_symbols_fd(callstack, frames, fd);
}

void OnCrashSignal(int sig, siginfo_t* info, void* ucontext) {
  // Redirect stderr to pump pipe so all crash output (C++ dump + Ruby
  // backtrace) is tee'd to both crash.log and the console (game.log).
  if (g_crash_pipe[1] >= 0)
    dup2(g_crash_pipe[1], STDERR_FILENO);

  WriteCrashDumpToFd(STDERR_FILENO, sig, info, ucontext);

  // (Ring buffer omitted -- see Engine.log for full log history)

  // Chain to saved old handler (e.g. Ruby's sigsegv).
  // stderr → pipe → pump thread → crash.log + console.
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

static LONG CALLBACK WinVectoredHandler(PEXCEPTION_POINTERS ep) {
  int fd = g_crash_fd >= 0 ? g_crash_fd : -1;
  if (fd < 0)
    fd = _fileno(stderr);

  EXCEPTION_RECORD* er = ep->ExceptionRecord;
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

  DumpRingBuffer(fd);
  FlushAndRaise(0);

  // Never reached (FlushAndRaise calls _exit)
  return EXCEPTION_CONTINUE_SEARCH;
}

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

  DumpRingBuffer(fd);
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

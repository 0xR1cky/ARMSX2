/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2023  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Pcsx2Defs.h"
#include "CrashHandler.h"
#include "FileSystem.h"
#include "StringUtil.h"
#include <cinttypes>
#include <cstdio>
#include <ctime>

#if defined(_WIN32)
#include "RedtapeWindows.h"

#include "StackWalker.h"
#include <DbgHelp.h>

class CrashHandlerStackWalker : public StackWalker
{
public:
	explicit CrashHandlerStackWalker(HANDLE out_file);
	~CrashHandlerStackWalker();

protected:
	void OnOutput(LPCSTR szText) override;

private:
	HANDLE m_out_file;
};

CrashHandlerStackWalker::CrashHandlerStackWalker(HANDLE out_file)
	: StackWalker(RetrieveVerbose, nullptr, GetCurrentProcessId(), GetCurrentProcess())
	, m_out_file(out_file)
{
}

CrashHandlerStackWalker::~CrashHandlerStackWalker() = default;

void CrashHandlerStackWalker::OnOutput(LPCSTR szText)
{
	if (m_out_file)
	{
		DWORD written;
		WriteFile(m_out_file, szText, static_cast<DWORD>(std::strlen(szText)), &written, nullptr);
	}

	OutputDebugStringA(szText);
}

static bool WriteMinidump(HMODULE hDbgHelp, HANDLE hFile, HANDLE hProcess, DWORD process_id, DWORD thread_id,
	PEXCEPTION_POINTERS exception, MINIDUMP_TYPE type)
{
	using PFNMINIDUMPWRITEDUMP =
		BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
			PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
			PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	PFNMINIDUMPWRITEDUMP minidump_write_dump = hDbgHelp ?
												   reinterpret_cast<PFNMINIDUMPWRITEDUMP>(GetProcAddress(hDbgHelp, "MiniDumpWriteDump")) :
                                                   nullptr;
	if (!minidump_write_dump)
		return false;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	PMINIDUMP_EXCEPTION_INFORMATION mei_ptr = nullptr;
	if (exception)
	{
		mei.ThreadId = thread_id;
		mei.ExceptionPointers = exception;
		mei.ClientPointers = FALSE;
		mei_ptr = &mei;
	}

	return minidump_write_dump(hProcess, process_id, hFile, type, mei_ptr, nullptr, nullptr);
}

static std::wstring s_write_directory;
static HMODULE s_dbghelp_module = nullptr;
static PVOID s_veh_handle = nullptr;
static bool s_in_crash_handler = false;

static void GenerateCrashFilename(wchar_t* buf, size_t len, const wchar_t* prefix, const wchar_t* extension)
{
	SYSTEMTIME st = {};
	GetLocalTime(&st);

	_snwprintf_s(buf, len, _TRUNCATE, L"%s%scrash-%04u-%02u-%02u-%02u-%02u-%02u-%03u.%s",
		prefix ? prefix : L"", prefix ? L"\\" : L"",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, extension);
}

static void WriteMinidumpAndCallstack(PEXCEPTION_POINTERS exi)
{
	s_in_crash_handler = true;

	wchar_t filename[1024] = {};
	GenerateCrashFilename(filename, std::size(filename), s_write_directory.empty() ? nullptr : s_write_directory.c_str(), L"txt");

	// might fail
	HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (exi && hFile != INVALID_HANDLE_VALUE)
	{
		char line[1024];
		DWORD written;
		std::snprintf(line, std::size(line), "Exception 0x%08X at 0x%p\n", static_cast<unsigned>(exi->ExceptionRecord->ExceptionCode),
			exi->ExceptionRecord->ExceptionAddress);
		WriteFile(hFile, line, static_cast<DWORD>(std::strlen(line)), &written, nullptr);
	}

	GenerateCrashFilename(filename, std::size(filename), s_write_directory.empty() ? nullptr : s_write_directory.c_str(), L"dmp");

	const MINIDUMP_TYPE minidump_type =
		static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithHandleData | MiniDumpWithProcessThreadData |
								   MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory);
	const HANDLE hMinidumpFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (hMinidumpFile == INVALID_HANDLE_VALUE ||
		!WriteMinidump(s_dbghelp_module, hMinidumpFile, GetCurrentProcess(), GetCurrentProcessId(),
			GetCurrentThreadId(), exi, minidump_type))
	{
		static const char error_message[] = "Failed to write minidump file.\n";
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD written;
			WriteFile(hFile, error_message, sizeof(error_message) - 1, &written, nullptr);
		}
	}
	if (hMinidumpFile != INVALID_HANDLE_VALUE)
		CloseHandle(hMinidumpFile);

	CrashHandlerStackWalker sw(hFile);
	sw.ShowCallstack(GetCurrentThread(), exi ? exi->ContextRecord : nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
}

static LONG NTAPI ExceptionHandler(PEXCEPTION_POINTERS exi)
{
	if (s_in_crash_handler)
		return EXCEPTION_CONTINUE_SEARCH;

	switch (exi->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_BREAKPOINT:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		case EXCEPTION_STACK_OVERFLOW:
		case EXCEPTION_GUARD_PAGE:
			break;

		default:
			return EXCEPTION_CONTINUE_SEARCH;
	}

	// if the debugger is attached, let it take care of it.
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	WriteMinidumpAndCallstack(exi);
	return EXCEPTION_CONTINUE_SEARCH;
}

bool CrashHandler::Install()
{
	// load dbghelp at install/startup, that way we're not LoadLibrary()'ing after a crash
	// .. because that probably wouldn't go down well.
	s_dbghelp_module = StackWalker::LoadDbgHelpLibrary();

	s_veh_handle = AddVectoredExceptionHandler(0, ExceptionHandler);
	return (s_veh_handle != nullptr);
}

void CrashHandler::SetWriteDirectory(const std::string_view& dump_directory)
{
	if (!s_veh_handle)
		return;

	s_write_directory = StringUtil::UTF8StringToWideString(dump_directory);
}

void CrashHandler::WriteDumpForCaller()
{
	WriteMinidumpAndCallstack(nullptr);
}

void CrashHandler::Uninstall()
{
	if (s_veh_handle)
	{
		RemoveVectoredExceptionHandler(s_veh_handle);
		s_veh_handle = nullptr;
	}

	if (s_dbghelp_module)
	{
		FreeLibrary(s_dbghelp_module);
		s_dbghelp_module = nullptr;
	}
}

#elif defined(HAS_LIBBACKTRACE)

#include "FileSystem.h"

#include <backtrace.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace CrashHandler
{
	struct BacktraceBuffer
	{
		char* buffer;
		size_t used;
		size_t size;
	};

	static const char* GetSignalName(int signal_no);
	static void AllocateBuffer(BacktraceBuffer* buf);
	static void FreeBuffer(BacktraceBuffer* buf);
	static void AppendToBuffer(BacktraceBuffer* buf, const char* format, ...);
	static int BacktraceFullCallback(void* data, uintptr_t pc, const char* filename, int lineno, const char* function);
	static void CallExistingSignalHandler(int signal, siginfo_t* siginfo, void* ctx);
	static void CrashSignalHandler(int signal, siginfo_t* siginfo, void* ctx);

	static std::recursive_mutex s_crash_mutex;
	static bool s_in_signal_handler = false;

	static backtrace_state* s_backtrace_state = nullptr;
	static struct sigaction s_old_sigbus_action;
	static struct sigaction s_old_sigsegv_action;
}

const char* CrashHandler::GetSignalName(int signal_no)
{
	switch (signal_no)
	{
		// Don't need to list all of them, there's only a couple we register.
		// clang-format off
		case SIGSEGV: return "SIGSEGV";
		case SIGBUS: return "SIGBUS";
		default: return "UNKNOWN";
		// clang-format on
	}
}

void CrashHandler::AllocateBuffer(BacktraceBuffer* buf)
{
	buf->used = 0;
	buf->size = __pagesize;
	buf->buffer = static_cast<char*>(mmap(nullptr, buf->size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
	if (buf->buffer == static_cast<char*>(MAP_FAILED))
	{
		buf->buffer = nullptr;
		buf->size = 0;
	}
}

void CrashHandler::FreeBuffer(BacktraceBuffer* buf)
{
	if (buf->buffer)
		munmap(buf->buffer, buf->size);
}

void CrashHandler::AppendToBuffer(BacktraceBuffer* buf, const char* format, ...)
{
	std::va_list ap;
	va_start(ap, format);

	// Hope this doesn't allocate memory... it *can*, but hopefully unlikely since
	// it won't be the first call, and we're providing the buffer.
	if (buf->size > 0 && buf->used < (buf->size - 1))
	{
		const int written = std::vsnprintf(buf->buffer + buf->used, buf->size - buf->used, format, ap);
		if (written > 0)
			buf->used += static_cast<size_t>(written);
	}

	va_end(ap);
}

int CrashHandler::BacktraceFullCallback(void* data, uintptr_t pc, const char* filename, int lineno, const char* function)
{
	BacktraceBuffer* buf = static_cast<BacktraceBuffer*>(data);
	AppendToBuffer(buf, "  %016p", pc);
	if (function)
		AppendToBuffer(buf, " %s", function);
	if (filename)
		AppendToBuffer(buf, " [%s:%d]", filename, lineno);
	
	AppendToBuffer(buf, "\n");
	return 0;
}

void CrashHandler::CallExistingSignalHandler(int signal, siginfo_t* siginfo, void* ctx)
{
	const struct sigaction& sa = (signal == SIGBUS) ? s_old_sigbus_action : s_old_sigsegv_action;
	if (sa.sa_flags & SA_SIGINFO)
	{
		sa.sa_sigaction(signal, siginfo, ctx);
	}
	else if (sa.sa_handler == SIG_DFL)
	{
		// Re-raising the signal would just queue it, and since we'd restore the handler back to us,
		// we'd end up right back here again. So just abort, because that's probably what it'd do anyway.
		abort();
	}
	else if (sa.sa_handler != SIG_IGN)
	{
		sa.sa_handler(signal);
	}
}

void CrashHandler::CrashSignalHandler(int signal, siginfo_t* siginfo, void* ctx)
{
	std::unique_lock lock(s_crash_mutex);
	
	// If we crash somewhere in libbacktrace, don't bother trying again.
	if (!s_in_signal_handler)
	{
		s_in_signal_handler = true;

#if defined(__APPLE__) && defined(__x86_64__)
		void* const exception_pc = reinterpret_cast<void*>(static_cast<ucontext_t*>(ctx)->uc_mcontext->__ss.__rip);
#elif defined(__FreeBSD__) && defined(__x86_64__)
		void* const exception_pc = reinterpret_cast<void*>(static_cast<ucontext_t*>(ctx)->uc_mcontext.mc_rip);
#elif defined(__x86_64__)
		void* const exception_pc = reinterpret_cast<void*>(static_cast<ucontext_t*>(ctx)->uc_mcontext.gregs[REG_RIP]);
#else
		void* const exception_pc = nullptr;
#endif

		BacktraceBuffer buf;
		AllocateBuffer(&buf);
		AppendToBuffer(&buf, "*************** Unhandled %s at %p ***************\n", GetSignalName(signal), exception_pc);

		const int rc = backtrace_full(s_backtrace_state, 0, BacktraceFullCallback, nullptr, &buf);
		if (rc != 0)
			AppendToBuffer(&buf, "  backtrace_full() failed: %d\n");

		AppendToBuffer(&buf, "*******************************************************************\n");

		if (buf.used > 0)
			write(STDERR_FILENO, buf.buffer, buf.used);

		FreeBuffer(&buf);

		s_in_signal_handler = false;
	}

	// Chances are we're not going to have anything else to call, but just in case.
	lock.unlock();
	CallExistingSignalHandler(signal, siginfo, ctx);
}

bool CrashHandler::Install()
{
	const std::string progpath = FileSystem::GetProgramPath();
	s_backtrace_state = backtrace_create_state(progpath.empty() ? nullptr : progpath.c_str(), 0, nullptr, nullptr);
	if (!s_backtrace_state)
		return false;
	
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO | SA_NODEFER;
	sa.sa_sigaction = CrashSignalHandler;
	if (sigaction(SIGBUS, &sa, &s_old_sigbus_action) != 0)
		return false;
	if (sigaction(SIGSEGV, &sa, &s_old_sigsegv_action) != 0)
		return false;

	return true;
}

void CrashHandler::SetWriteDirectory(const std::string_view& dump_directory)
{
}

void CrashHandler::WriteDumpForCaller()
{
}

void CrashHandler::Uninstall()
{
	// We can't really unchain the signal handlers... so, YOLO.
}


#else

bool CrashHandler::Install()
{
	return false;
}

void CrashHandler::SetWriteDirectory(const std::string_view& dump_directory)
{
}

void CrashHandler::WriteDumpForCaller()
{
}

void CrashHandler::Uninstall()
{
}

#endif
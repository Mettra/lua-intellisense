#ifdef _WIN32
#include "Minidump.h"
namespace Windows
{
#include <DbgHelp.h>
}

Windows::LONG WINAPI WriteDump(Windows::EXCEPTION_POINTERS* pException)
{
  Windows::HANDLE file = Windows::CreateFile("minidump.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  Windows::MINIDUMP_EXCEPTION_INFORMATION info;
  info.ClientPointers = false;
  info.ExceptionPointers = pException;
  info.ThreadId = Windows::GetCurrentThreadId();

  Windows::MiniDumpWriteDump(Windows::GetCurrentProcess(), Windows::GetCurrentProcessId(), file, Windows::MiniDumpWithFullMemory, &info, NULL, NULL);

  Windows::CloseHandle(file);

  return EXCEPTION_CONTINUE_SEARCH;
}

void WINAPI SetupMinidump()
{
  Windows::ULONG stackSize = 1 << 17;
  Windows::SetThreadStackGuarantee(&stackSize);
}
#endif

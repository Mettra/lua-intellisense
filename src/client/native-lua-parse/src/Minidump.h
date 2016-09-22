#ifdef _WIN32
namespace Windows
{
  #include <Windows.h>
}

Windows::LONG WINAPI WriteDump(Windows::EXCEPTION_POINTERS* pException);
void WINAPI SetupMinidump();

#endif 

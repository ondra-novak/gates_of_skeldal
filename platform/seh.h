
#ifdef _WIN32
  #include "windows/minidump.h"
  #define SEH_MONITOR_BEGIN __try
  #define SEH_MONITOR_END __except(GenerateMinidumpAndExit(GetExceptionInformation())) {}
#else
  #define SEH_TRY 
  #define SEH_CATCH 
#endif


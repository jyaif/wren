
#include "os.h"

#if __APPLE__
  #include "TargetConditionals.h"
#endif

int numArgs;
const char** args;

void osSetArguments(int argc, const char* argv[])
{
  numArgs = argc;
  args = argv;
}

void platformName(WrenVM* vm)
{
  wrenEnsureSlots(vm, 1);
  
  wrenSetSlotString(vm, 0,
  #ifdef _WIN32
                    "Windows"
  #elif __APPLE__
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
                    "iOS"
    #elif TARGET_OS_MAC
                    "OS X"
    #else
                    "Unknown"
    #endif
  #elif __linux__
                    "Linux"
  #elif __unix__
                    "Unix"
  #elif defined(_POSIX_VERSION)
                    "POSIX"
  #else
                    "Unknown"
  #endif
                    );
}

void platformIsPosix(WrenVM* vm)
{
  wrenEnsureSlots(vm, 1);
  
  wrenSetSlotBool(vm, 0,
  #ifdef _WIN32
                  false
  #elif __APPLE__
                  true
  #elif __linux__
                  true
  #elif __unix__
                  true
  #elif defined(_POSIX_VERSION)
                  true
  #else
                  false
  #endif
                  );
}

void processAllArguments(WrenVM* vm)
{
  wrenEnsureSlots(vm, 2);
  wrenSetSlotNewList(vm, 0);

  for (int i = 0; i < numArgs; i++)
  {
    wrenSetSlotString(vm, 1, args[i]);
    wrenInsertInList(vm, 0, -1, 1);
  }
}

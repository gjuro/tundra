#include "StableHeaders.h"

#include "LoggingFunctions.h"
#include "Framework.h"
#include "ConsoleAPI.h"

#include "Win.h"

#ifdef TUNDRA_PLATFORM_ANDROID
#include "android/log.h"
#define __android_tundra_info(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Tundra", __VA_ARGS__))
#define __android_tundra_warning(...) ((void)__android_log_print(ANDROID_LOG_WARN, "Tundra", __VA_ARGS__))
#define __android_tundra_error(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "Tundra", __VA_ARGS__))
#define __android_tundra_debug(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "Tundra", __VA_ARGS__))
#endif

void PrintLogMessage(u32 logChannel, const char *str)
{
    if (!IsLogChannelEnabled(logChannel))
        return;

#ifdef TUNDRA_PLATFORM_ANDROID
    if (logChannel == LogChannelInfo)
        __android_tundra_info(str);
    else if (logChannel == LogChannelWarning)
        __android_tundra_warning(str);
    else if (logChannel == LogChannelError)
        __android_tundra_error(str);
    else if (logChannel == LogChannelDebug)
        __android_tundra_debug(str);
    return;
#endif

    Framework *instance = Framework::Instance();
    ConsoleAPI *console = (instance ? instance->Console() : 0);

    // On Windows, highlight errors and warnings.
#ifdef WIN32
    if ((logChannel & LogChannelError) != 0) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
    else if ((logChannel & LogChannelWarning) != 0) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif
    // The console and stdout prints are equivalent.
    if (console)
        console->Print(str);
    else // The Console API is already dead for some reason, print directly to stdout to guarantee we don't lose any logging messags.
        printf("%s", str);

    // Restore the text color to normal.
#ifdef WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

bool IsLogChannelEnabled(u32 logChannel)
{
    Framework *instance = Framework::Instance();
    ConsoleAPI *console = (instance ? instance->Console() : 0);

    if (console)
        return console->IsLogChannelEnabled(logChannel);
    else
        return true; // We've already killed Framework and Console! Print out everything so that we can't accidentally lose any important messages.
}


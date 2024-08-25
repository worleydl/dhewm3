/*
    SDL_winrt_main_NonXAML.cpp, placed in the public domain by David Ludwig  3/13/14
*/

#include "SDL_main.h"
#include <wrl.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.Core.h>
#include <winrt/Windows.Graphics.Display.Core.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.System.h>

#include <gamingdeviceinformation.h>

#include <winrt/Windows.Graphics.Display.Core.h>o

#include <expandedresources.h>
#include <deque>
#include <mutex>
#include <thread>



using namespace winrt::Windows::Graphics::Display::Core;

/* At least one file in any SDL/WinRT app appears to require compilation
   with C++/CX, otherwise a Windows Metadata file won't get created, and
   an APPX0702 build error can appear shortly after linking.

   The following set of preprocessor code forces this file to be compiled
   as C++/CX, which appears to cause Visual C++ 2012's build tools to
   create this .winmd file, and will help allow builds of SDL/WinRT apps
   to proceed without error.

   If other files in an app's project enable C++/CX compilation, then it might
   be possible for SDL_winrt_main_NonXAML.cpp to be compiled without /ZW,
   for Visual C++'s build tools to create a winmd file, and for the app to
   build without APPX0702 errors.  In this case, if
   SDL_WINRT_METADATA_FILE_AVAILABLE is defined as a C/C++ macro, then
   the #error (to force C++/CX compilation) will be disabled.

   Please note that /ZW can be specified on a file-by-file basis.  To do this,
   right click on the file in Visual C++, click Properties, then change the
   setting through the dialog that comes up.
*/

/* Prevent MSVC++ from warning about threading models when defining our
   custom WinMain.  The threading model will instead be set via a direct
   call to Windows::Foundation::Initialize (rather than via an attributed
   function).

   To note, this warning (C4447) does not seem to come up unless this file
   is compiled with C++/CX enabled (via the /ZW compiler flag).
*/
#ifdef _MSC_VER
#pragma warning(disable : 4447)
#endif

/* Make sure the function to initialize the Windows Runtime gets linked in. */
#ifdef _MSC_VER
#pragma comment(lib, "runtimeobject.lib")
#endif

// Window state vars
static int width, height;

#include "../neo/renderer/wininfo.h"

static std::deque<std::function<void()>> aux_queue;
static std::mutex aux_mutex;
static bool aux_running = true;

// Thread for running tasks like saving cfg files so it doesn't delay render
void AuxThread()
{
    while (aux_running) {
       static std::mutex m_event_mutex;
       while (!aux_queue.empty())
       {
           aux_queue.front()();
           aux_queue.pop_front();
       }

       Sleep(50);
    }

}

int WinInfo::getHostWidth() {
    return width;
}

int WinInfo::getHostHeight() {
    return height;
}

void WinInfo::runOnAuxThread(std::function<void()> funkshun)
{
    std::unique_lock<std::mutex> lk(aux_mutex);
    aux_queue.push_back(funkshun);
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Inject HDR mode hack
    HdmiDisplayInformation hdi = HdmiDisplayInformation::GetForCurrentView();

    auto modes = hdi.GetSupportedDisplayModes();

    for (unsigned i = 0; i < modes.Size(); i++)
    {
        HdmiDisplayMode mode = modes.GetAt(i);

        if (mode.ColorSpace() == HdmiDisplayColorSpace::BT2020 && mode.RefreshRate() >= 59)
        {
            hdi.RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::Eotf2084);
             width = hdi.GetCurrentDisplayMode().ResolutionWidthInRawPixels();
             height = hdi.GetCurrentDisplayMode().ResolutionHeightInRawPixels();
            break;
        }
    }
    std::thread t = std::thread(AuxThread);
    int resp =  SDL_WinRTRunApp(SDL_main, NULL);
    aux_running = false;
    t.join();
    return resp;
}

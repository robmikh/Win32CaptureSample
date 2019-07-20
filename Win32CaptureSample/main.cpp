#include "pch.h"
#include "SimpleCapture.h"
#include "App.h"
#include "SampleWindow.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace winrt;
using namespace Windows::Graphics::Capture;
using namespace Windows::UI::Composition;

int __stdcall WinMain(HINSTANCE instance, HINSTANCE, PSTR cmdLine, int cmdShow)
{
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); // works but everything draws small

    init_apartment(apartment_type::single_threaded);

    // Check to see that capture is supported
    auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
    if (!isCaptureSupported)
    {
        MessageBoxW(nullptr,
            L"Screen capture is not supported on this device for this release of Windows!",
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create the DispatcherQueue that the compositor needs to run
    auto controller = CreateDispatcherQueueControllerForCurrentThread();

    // Initialize Composition
    auto compositor = Compositor();
    auto root = compositor.CreateContainerVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    root.Size({ -220.0f, 0.0f });
    root.Offset({ 220.0f, 0.0f, 0.0f });

    auto picker = GraphicsCapturePicker();

    auto app = std::make_shared<App>(root, picker);

    auto window = SampleWindow(instance, cmdShow, app);

    // Provide the window handle to the picker (explict HWND initialization)
    window.InitializeObjectWithWindowHandle(picker);

    // Hookup the visual tree to the window
    auto target = window.CreateWindowTarget(compositor);
    target.Root(root);

    // Message pump
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
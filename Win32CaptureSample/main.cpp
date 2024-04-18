#include "pch.h"
#include "App.h"
#include "SampleWindow.h"

//
// SPOUT
//
//      Win32CaptureSample - created by Robert Mikhayelyan
//      Forked from : https://github.com/robmikh/Win32CaptureSample
//      Modified for Spout output.
//      SpoutDX static library / Visual Studio 2022
//      2022-2024 Lynn Jarvis : https://github.com/leadedge/Win32CaptureSample
//
//   Add console for debugging.
//   Limit to BRGA8 format.
//   Default cursor capture off.
//   Add "Send client area" option.
//   Include window handle for TryStartCaptureFromWindowHandle and StartCaptureFromItem
//   Add m_hWnd to SimpleCapture
//   Add Spout sender to SimpleCapture
//   Add SendTexture to OnFrameArrived
//
// Search for "SPOUT" in :
// Main.cpp, App.h, SimpleCapture.h, SimpleCapture.cpp
// SampleWindow.h, SampleWindow.cpp
// Build with Visual Studio 2022
//
// SpoutDX - for this application, added function to send part of a texture.
//   bool SendTexture(ID3D11Texture2D* pTexture,
//         unsigned int xoffset, unsigned int yoffset,
//         unsigned int width, unsigned int height);
//
// Changes :
// 22.02.22 - Add OpenDirectX to SimpleCapture
// 20.05.22 - Centre on the desktop
//            Capture display slightly larger
//            Name change to SpoutWinCapture throughout
//            Update project to Visual Studio 2022
// 16.09.23 - SimpleCapture.cpp - OnFrameArrived
//            Test window and client region sizes before sending texture.
// 19.09.23 - Change name to "SpoutWinCapture" to align with the caption title
//            and to distinguish from the original executable.
//            Change default icon file name to SpoutWinCapture.ico
// 13.04.23 - Change from SpoutDX dll to static library
//            App.h - m_capture moved from private to public
//            SampleWindow.cpp
//              - OnSnapshotButtonClicked - replace image display with messagebox
//              - Change from 800x600 to 800x480 (16:9)
// 16.04.24   SampleWindow.cpp
//              - For a command line, set the current combobox selection to the capture window
//              - Add About box
// 17.04.24   SimpleCapture::OnFrameArrived
//              - Fix for client area send when a minimized window is selected and restored
//            Add resource files for icon and version
//            Add version number to about box
// 18.04.24   SampleWindow.cpp 
//              - change icon to spoutC.ico with larger size for about box.
//              - load icon from resources for standalone program
//
// =============================================================================
//
//  MIT License
//
//  Copyright(c) 2019 Robert Mikhayelyan
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files(the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions :
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//
// =============================================================================

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace winrt
{
    using namespace Windows::Storage::Pickers;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace desktop;
}

// SPOUT
// To avoid warning C28251
// int __stdcall WinMain(HINSTANCE instance, HINSTANCE, PSTR cmdLine, int cmdShow)
int __stdcall WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE hInstPrev, _In_ LPSTR lpCmdLine, _In_ int cmdShow)
{
    // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); // works but everything draws small
    // SPOUT - Project properties > Manifest Tool > Input and Output > DPI Awareness > Per Monitor High DPI Aware
    
    // Initialize COM
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    // Check to see that capture is supported
    auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
    if (!isCaptureSupported)
    {
        MessageBoxW(nullptr,
            L"Screen capture is not supported on this device for this release of Windows!",
            // SPOUT - change name from "Win32CaptureSample" to "SpoutWinCapture"
            L"SpoutWinCapture",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    SampleWindow::RegisterWindowClass();

    // Create the DispatcherQueue that the compositor needs to run
    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    // Initialize Composition
    auto compositor = winrt::Compositor();
    auto root = compositor.CreateContainerVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    // SPOUT
    // Make a little bit bigger
    // root.Size({ -220.0f, 0.0f });
    // root.Offset({ 220.0f, 0.0f, 0.0f });
    root.Size({ -180.0f, 0.0f });
    root.Offset({ 195.0f, 0.0f, 0.0f });

    // Create the pickers
    auto capturePicker = winrt::GraphicsCapturePicker();
    auto savePicker = winrt::FileSavePicker();

    // Create the app
    auto app = std::make_shared<App>(root, capturePicker, savePicker);

    /*
    // SPOUT - console for debugging
    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    */

    // SPOUT
    // Allow for a command line
    auto window = SampleWindow(instance, lpCmdLine, cmdShow, app);
    // auto window = SampleWindow(instance, cmdShow, app);

    // Provide the window handle to the pickers (explicit HWND initialization)
    window.InitializeObjectWithWindowHandle(capturePicker);
    window.InitializeObjectWithWindowHandle(savePicker);

    // Hookup the visual tree to the window
    auto target = window.CreateWindowTarget(compositor);
    target.Root(root);

    // Message pump
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
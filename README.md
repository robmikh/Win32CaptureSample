# Win32CaptureSample
A simple sample using the Windows.Graphics.Capture APIs in a Win32 application.

## Table of Contents
  * [Requirements](https://github.com/robmikh/Win32CaptureSample#requirements)
  * [Win32 vs UWP](https://github.com/robmikh/Win32CaptureSample#win32-vs-uwp)
    * [HWND or HMONITOR based capture](https://github.com/robmikh/Win32CaptureSample#hwnd-or-hmonitor-based-capture)
    * [Using the GraphicsCapturePicker](https://github.com/robmikh/Win32CaptureSample#using-the-graphicscapturepicker)
  * [Create vs CreateFreeThreaded](https://github.com/robmikh/Win32CaptureSample#create-vs-createfreethreaded)
  * [Points of interest](https://github.com/robmikh/Win32CaptureSample#points-of-interest)

## Requirements
This sample requires the [Windows 11 SDK (10.0.22000.194)](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/) and [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) to compile. Neither are required to run the sample once you have a binary. The minimum verison of Windows 10 required to run the sample is build 17134.

## Win32 vs UWP
This sample demonstrates using the Windows.Graphics.Capture APIs in a Win32 application. For the most part, the usage is the same, except for a few tweaks:

### HWND or HMONITOR based capture
Win32 applications have access to the [IGraphicsCaptureItemInterop](https://docs.microsoft.com/en-us/windows/win32/api/windows.graphics.capture.interop/nn-windows-graphics-capture-interop-igraphicscaptureiteminterop) interface. This interface can be found by QIing for it on the `GraphicsCaptureItem` factory:

```cpp
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>

namespace winrt
{
    using namespace Windows::Graphics::Capture;
}

// Obtaining the factory
auto interopFactory = winrt::get_activation_factory<
    winrt::GraphicsCaptureItem, 
    IGraphicsCaptureItemInterop>();

winrt::GraphicsCaptureItem item{ nullptr };

// Creating a GraphicsCaptureItem from a HWND
winrt::check_hresult(interopFactory->CreateForWindow(
    hwnd, 
    winrt::guid_of<winrt::GraphicsCaptureItem>(), 
    winrt::put_abi(item)));

// Creating a GraphicsCaptureItem from a HMONITOR
winrt::check_hresult(interopFactory->CreateForMonitor(
    hmon, 
    winrt::guid_of<winrt::GraphicsCaptureItem>(), 
    winrt::put_abi(item)));
```
This sample makes uses a collection of common header files which [contains helpers](https://github.com/robmikh/robmikh.common/blob/master/robmikh.common/include/robmikh.common/capture.desktop.interop.h) for `GraphicsCaptureItem` creation.

### Using the GraphicsCapturePicker
Win32 applications may also use the `GraphicsCapturePicker` to obtain a `GraphicsCaptureItem`, which asks the user to do the selection. Like other pickers, the `GraphicsCapturePicker` won't be able to infer your window in a Win32 application. You'll need to QI for the [`IInitializeWithWindow`](https://msdn.microsoft.com/en-us/library/windows/desktop/hh706981(v=vs.85).aspx) interface and provide your window's HWND.

```cpp
#include <winrt/Windows.Graphics.Capture.h>
#include <shobjidl_core.h>

namespace winrt
{
    using namespace Windows::Graphics::Capture;
}

auto picker = winrt::GraphicsCapturePicker();
auto initializeWithWindow = picker.as<IInitializeWithWindow>();
winrt::check_hresult(initializeWithWindow->Initialize(hwnd));
// The picker is now ready for use!
```

## Create vs CreateFreeThreaded
When a `Direct3D11CaptureFramePool` is created using `Create`, you're required to have a `DispatcherQueue` for the current thread and for that thread to be pumping messages. When created with `Create`, the `FrameArrived` event will fire on the same thread that created the frame pool.

When created with `CreateFreeThreaded`, there is no `DispatcherQueue` requirement. However, the `FrameArrived` event will fire on the frame pool's internal thread. This means that the callback you provide must be agile (this is usually handled by the language projection).

## Points of interest
The code is organized into a couple of classes:

  * [`App.h/cpp`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp) handles the basic logic of the sample, as well as setting up the visual tree to present the capture preview.
  * [`SampleWindow.h/cpp`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SampleWindow.cpp) handles the main window and the controls.
  * [`SimpleCapture.h/cpp`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SimpleCapture.cpp) handles the basics of using the Windows.Graphics.Capture API given a `GraphicsCaptureItem`. It starts the capture and copies each frame to a swap chain that is shown on the main window.
  * [`CaptureSnapshot.h/cpp`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/CaptureSnapshot.cpp) shows how to take a snapshot with the Windows.Graphics.Capture API. The current version uses coroutines, but you could synchronously wait as well using the same events. Just remember to create your frame pool with `CreateFreeThreaded` so you don't deadlock!

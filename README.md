# Win32CaptureSample
A simple sample using the Windows.Graphics.Capture APIs in a Win32 application.

## Points of interest
Here are some places you should look at in the code to learn the following:

* Capture a window given its window handle. [`App::StartCapture(HWND)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Capture a monitor given its monitor handle. [`App::StartCapture(HMONITOR)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Show the system provided picker and capture the selected window/monitor. [`App::StartCaptureWithPickerAsync()`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Setting up the Windows.Graphics.Capture API. [`SimpleCapture::SimpleCapture()`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SimpleCapture.cpp)
* Processing frames received from the frame pool. [`SimpleCapture::OnFrameArrived(Direct3D11CaptureFramePool, IInspectable)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SimpleCapture.cpp)

## Win32 vs UWP
For the most part, using the API is the same between Win32 and UWP. However, there are some small differences.

1. The `GraphicsCapturePicker` won't be able to infer your window in a Win32 application, so you'll have to QI for [`IInitializeWithWindow`](https://msdn.microsoft.com/en-us/library/windows/desktop/hh706981(v=vs.85).aspx) and provide your window's HWND.
2. `Direct3D11CaptureFramePool` requires a `DispatcherQueue` much like the Composition APIs. You'll need to create a dispatcher for your thread. Alternatively you can use `Direct3D11CaptureFramePool::CreateFreeThreaded` to create the frame pool. Doing so will remove the `DispatcherQueue` requirement, but the `FrameArrived` event will be called from an arbitrary thread.

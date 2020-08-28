# Win32CaptureSample
A simple sample using the Windows.Graphics.Capture APIs in a Win32 application.

## Points of interest
Here are some places you should look at in the code to learn the following:

* Capture a window given its window handle. [`App::StartCapture(HWND)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Capture a monitor given its monitor handle. [`App::StartCapture(HMONITOR)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Show the system provided picker and capture the selected window/monitor. [`App::StartCaptureWithPickerAsync()`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp)
* Setting up the Windows.Graphics.Capture API. [`SimpleCapture::SimpleCapture()`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SimpleCapture.cpp)
* Processing frames received from the frame pool. [`SimpleCapture::OnFrameArrived(Direct3D11CaptureFramePool, IInspectable)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/SimpleCapture.cpp)
* Taking a snapshot [`CaptureSnapshot::TakeAsync(IDirect3DDevice, GraphicsCaptureItem)`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/CaptureSnapshot.cpp) and [`App::TakeSnapshotAsync()`](https://github.com/robmikh/Win32CaptureSample/blob/master/Win32CaptureSample/App.cpp) for capturing and encoding respectively.

## Win32 vs UWP
For the most part, using the API is the same between Win32 and UWP. However, there are some small differences.

1. The `GraphicsCapturePicker` won't be able to infer your window in a Win32 application, so you'll have to QI for [`IInitializeWithWindow`](https://msdn.microsoft.com/en-us/library/windows/desktop/hh706981(v=vs.85).aspx) and provide your window's HWND.
2. `Direct3D11CaptureFramePool` requires a `DispatcherQueue` much like the Composition APIs. You'll need to create a dispatcher for your thread. Alternatively you can use `Direct3D11CaptureFramePool::CreateFreeThreaded` to create the frame pool. Doing so will remove the `DispatcherQueue` requirement, but the `FrameArrived` event will be called from an arbitrary thread.

## Create vs CreateFreeThreaded
You might have noticed that there are two ways to create the `Direct3D11CaptureFramePool` in the code (`SimpleCapture::SimpleCapture` and `CaptureSnapshot::TakeAsync`). As the name suggests, the method you use to create the frame pool dictates its threading behavior.

Creating the frame pool using `Direct3D11CaptureFramePool::Create` ensures that the frame pool's `FrameArrived` event will always call you back on the thread the frame pool was created on. In order to do this, the frame pool requires that a `DispatcherQueue` be associated with the thread, much like the `Windows::UI::Composition::Compositor` object.

Creating the frame pool using `Direct3D11CaptureFramePool::CreateFreeThreaded`, on the other hand, does not require the presence of a `DispatcherQueue`. However, in return, the frame pool's `FrameArrived` event will call you back on an arbitrary thread. Additionally, your callback must be agile (this should only effect those using the raw WinRT ABI).

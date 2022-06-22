#include "pch.h"
#include "CaptureSnapshot.h"

namespace winrt
{
    using namespace Windows;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::uwp;
}

std::future<winrt::com_ptr<ID3D11Texture2D>>
CaptureSnapshot::TakeAsync(winrt::IDirect3DDevice const& device, winrt::GraphicsCaptureItem const& item, winrt::DirectXPixelFormat const& pixelFormat)
{
    // Grab the apartment context so we can return to it.
    winrt::apartment_context context;

    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(device);
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    d3dDevice->GetImmediateContext(d3dContext.put());

    // Creating our frame pool with CreateFreeThreaded means that we 
    // will be called back from the frame pool's internal worker thread
    // instead of the thread we are currently on. It also disables the
    // DispatcherQueue requirement.
    auto framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
        device,
        pixelFormat,
        1,
        item.Size());
    auto session = framePool.CreateCaptureSession(item);

    wil::shared_event captureEvent(wil::EventOptions::ManualReset);
    winrt::Direct3D11CaptureFrame frame{ nullptr };
    framePool.FrameArrived([&frame, captureEvent](auto& framePool, auto&)
    {
        frame = framePool.TryGetNextFrame();

        // Complete the operation
        captureEvent.SetEvent();
    });

    session.StartCapture();
    co_await winrt::resume_on_signal(captureEvent.get());
    co_await context;

    // End the capture
    session.Close();
    framePool.Close();

    auto texture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    auto result = util::CopyD3DTexture(d3dDevice, texture, true);

    co_return result;
}

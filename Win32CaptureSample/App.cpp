#include "pch.h"
#include "App.h"
#include "SimpleCapture.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::Graphics::Capture;

void App::Run(
    ContainerVisual root,
    GraphicsCapturePicker picker)
{
    auto queue = DispatcherQueue::GetForCurrentThread();

    m_picker = picker;
    m_compositor = root.Compositor();
    m_root = m_compositor.CreateSpriteVisual();
    m_content = m_compositor.CreateSpriteVisual();
    m_brush = m_compositor.CreateSurfaceBrush();

    m_root.RelativeSizeAdjustment({ 1, 1 });
    m_root.Brush(m_compositor.CreateColorBrush(Colors::White()));
    root.Children().InsertAtTop(m_root);

    if (GraphicsCaptureSession::IsSupported())
    {
        m_content.AnchorPoint({ 0.5f, 0.5f });
        m_content.RelativeOffsetAdjustment({ 0.5f, 0.5f, 0 });
        m_content.RelativeSizeAdjustment({ 1, 1 });
        m_content.Size({ -80, -80 });
        m_content.Brush(m_brush);
        m_brush.HorizontalAlignmentRatio(0.5f);
        m_brush.VerticalAlignmentRatio(0.5f);
        m_brush.Stretch(CompositionStretch::Uniform);
        auto shadow = m_compositor.CreateDropShadow();
        shadow.Mask(m_brush);
        m_content.Shadow(shadow);
        m_root.Children().InsertAtTop(m_content);

        auto d3dDevice = CreateD3DDevice();
        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        m_device = CreateDirect3DDevice(dxgiDevice.get());

        // We can't just call the picker here, because no one is pumping messages yet.
        // By asking the dispatcher for our UI thread to run this, we ensure that the
        // message pump is pumping messages by the time this runs.
        auto success = queue.TryEnqueue([=]() -> void
        {
            auto ignoredAction = StartCaptureAsync();
        });
        WINRT_ASSERT(success);
    }
    else
    {
        auto success = queue.TryEnqueue([=]() -> void
        {
            auto dialog = Windows::UI::Popups::MessageDialog(L"Screen capture is not supported on this device for this release of Windows!");

            auto ignoredOperation = dialog.ShowAsync();
        });
    }
}

IAsyncAction App::StartCaptureAsync()
{
    auto item = co_await m_picker.PickSingleItemAsync();

    if (item != nullptr)
    {
        m_capture = std::make_unique<SimpleCapture>(m_device, item);

        auto surface = m_capture->CreateSurface(m_compositor);
        m_brush.Surface(surface);

        m_capture->StartCapture();
    }
}
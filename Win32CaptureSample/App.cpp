#include "pch.h"
#include "EnumerationWindow.h"
#include "App.h"
#include "SimpleCapture.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::Graphics::Capture;

App::App(
    ContainerVisual root,
    GraphicsCapturePicker picker)
{
    m_picker = picker;

    m_compositor = root.Compositor();
    m_root = m_compositor.CreateContainerVisual();
    m_content = m_compositor.CreateSpriteVisual();
    m_brush = m_compositor.CreateSurfaceBrush();

    m_root.RelativeSizeAdjustment({ 1, 1 });
    root.Children().InsertAtTop(m_root);

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
}

void App::StartCaptureFromWindowHandle(HWND hwnd)
{
    auto item = CreateCaptureItemForWindow(hwnd);

    StartCaptureFromItem(item);
}

void App::StartCaptureFromMonitorHandle(HMONITOR hmon)
{
    auto item = CreateCaptureItemForMonitor(hmon);

    StartCaptureFromItem(item);
}

IAsyncAction App::StartCaptureWithPickerAsync()
{
    auto item = co_await m_picker.PickSingleItemAsync();

    if (item != nullptr)
    {
        StartCaptureFromItem(item);
    }
}

void App::StartCaptureFromItem(
    GraphicsCaptureItem item)
{
    m_capture = std::make_unique<SimpleCapture>(m_device, item);

    auto surface = m_capture->CreateSurface(m_compositor);
    m_brush.Surface(surface);

    m_capture->StartCapture();
}
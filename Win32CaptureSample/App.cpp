#include "pch.h"
#include "App.h"
#include "CaptureSnapshot.h"

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Popups;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;

App::App(ContainerVisual root, GraphicsCapturePicker capturePicker, FileSavePicker savePicker)
{
    m_capturePicker = capturePicker;
    m_savePicker = savePicker;
    m_mainThread = DispatcherQueue::GetForCurrentThread();
    WINRT_VERIFY(m_mainThread != nullptr);

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

    m_encoder = std::make_unique<SimpleImageEncoder>(m_device);
}

GraphicsCaptureItem App::StartCaptureFromWindowHandle(HWND hwnd)
{
    auto item = CreateCaptureItemForWindow(hwnd);
    StartCaptureFromItem(item);
    return item;
}

GraphicsCaptureItem App::StartCaptureFromMonitorHandle(HMONITOR hmon)
{
    auto item = CreateCaptureItemForMonitor(hmon);
    StartCaptureFromItem(item);
    return item;
}

IAsyncOperation<GraphicsCaptureItem> App::StartCaptureWithPickerAsync()
{
    auto item = co_await m_capturePicker.PickSingleItemAsync();
    if (item)
    {
        // We might resume on a different thread, so let's ask the main thread's
        // dispatcher to do this work for us. This is important because SimpleCapture
        // uses Direct3D11CaptureFramePool::Create, which requires the existence of
        // a DispatcherQueue. See CaptureSnapshot for an example that uses 
        // Direct3D11CaptureFramePool::CreateFreeThreaded, which doesn't now have this
        // requirement. See the README if you're unsure of which version of 'Create' to use.
        m_mainThread.TryEnqueue([=](auto&& ...)
        {
            StartCaptureFromItem(item);
        });
    }

    co_return item;
}

IAsyncOperation<StorageFile> App::TakeSnapshotAsync()
{
    // First, get a GraphicsCaptureItem. Here we're using the picker, but
    // this would also work for any other type of GraphicsCaptureItem.
    auto item = co_await m_capturePicker.PickSingleItemAsync();
    if (item == nullptr)
    {
        // The user decided not to capture anything.
        co_return nullptr;
    }

    // Ask the user where they want to save the snapshot.
    m_savePicker.SuggestedStartLocation(PickerLocationId::PicturesLibrary);
    m_savePicker.SuggestedFileName(L"snapshot");
    m_savePicker.DefaultFileExtension(L".png");
    m_savePicker.FileTypeChoices().Clear();
    m_savePicker.FileTypeChoices().Insert(L"PNG image", single_threaded_vector<hstring>({ L".png" }));
    m_savePicker.FileTypeChoices().Insert(L"JPG image", single_threaded_vector<hstring>({ L".jpg" }));
    m_savePicker.FileTypeChoices().Insert(L"JXR image", single_threaded_vector<hstring>({ L".jxr" }));
    auto file = co_await m_savePicker.PickSaveFileAsync();
    if (file == nullptr)
    {
        co_return nullptr;
    }

    // Decide on the pixel format depending on the image type
    auto fileExtension = file.FileType();
    SimpleImageEncoder::SupportedFormats fileFormat;
    DirectXPixelFormat pixelFormat;
    if (fileExtension == L".png")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Png;
        pixelFormat = DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jpg" || fileExtension == L".jpeg")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Jpg;
        pixelFormat = DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jxr")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Jxr;
        pixelFormat = DirectXPixelFormat::R16G16B16A16Float;
    }
    else
    {
        // Unsupported
        auto dialog = MessageDialog(L"Unsupported file format!");

        co_await dialog.ShowAsync();
        co_return nullptr;
    }

    // Get the file stream
    auto stream = co_await file.OpenAsync(FileAccessMode::ReadWrite);

    // Take the snapshot
    auto frame = co_await CaptureSnapshot::TakeAsync(m_device, item, pixelFormat);
    
    // Encode the image
    m_encoder->EncodeImage(frame, stream, fileFormat);

    co_return file;
}

void App::SnapshotCurrentCapture()
{
    if (m_capture)
    {
        m_capture->SaveNextFrame();
    }
}

void App::StartCaptureFromItem(GraphicsCaptureItem item)
{
    m_capture = std::make_unique<SimpleCapture>(m_device, item, m_pixelFormat);

    auto surface = m_capture->CreateSurface(m_compositor);
    m_brush.Surface(surface);

    m_capture->StartCapture();
}

void App::StopCapture()
{
    if (m_capture)
    {
        m_capture->Close();
        m_capture = nullptr;
        m_brush.Surface(nullptr);
    }
}

void App::PixelFormat(DirectXPixelFormat pixelFormat)
{
    m_pixelFormat = pixelFormat;
    if (m_capture)
    {
        auto item = m_capture->CaptureItem();
        StopCapture();
        StartCaptureFromItem(item);
    }
}
#include "pch.h"
#include "EnumerationWindow.h"
#include "App.h"
#include "SimpleCapture.h"
#include "CaptureSnapshot.h"

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
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

    m_d2dFactory = CreateD2DFactory();
    m_d2dDevice = CreateD2DDevice(m_d2dFactory, d3dDevice);
    check_hresult(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.put()));
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
    auto file = co_await m_savePicker.PickSaveFileAsync();
    if (file == nullptr)
    {
        co_return nullptr;
    }

    // Get the file stream
    auto randomAccessStream = co_await file.OpenAsync(FileAccessMode::ReadWrite);
    auto stream = CreateStreamFromRandomAccessStream(randomAccessStream);

    // Take the snapshot
    auto frame = co_await CaptureSnapshot::TakeAsync(m_device, item);
    auto frameTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame);
    D3D11_TEXTURE2D_DESC textureDesc = {};
    frameTexture->GetDesc(&textureDesc);
    auto dxgiFrameTexture = frameTexture.as<IDXGISurface>();

    // Get a D2D bitmap for our snapshot
    // TODO: Since this sample doesn't use D2D any other way, it may be better to map 
    //       the pixels manually and hand them to WIC. However, using d2d is easier for now.
    com_ptr<ID2D1Bitmap1> d2dBitmap;
    check_hresult(m_d2dContext->CreateBitmapFromDxgiSurface(dxgiFrameTexture.get(), nullptr, d2dBitmap.put()));

    // Encode the snapshot
    // TODO: dpi?
    auto dpi = 96.0f;
    WICImageParameters params = {};
    params.PixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    params.PixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    params.DpiX = dpi;
    params.DpiY = dpi;
    params.PixelWidth = textureDesc.Width;
    params.PixelHeight = textureDesc.Height;

    auto wicFactory = CreateWICFactory();
    com_ptr<IWICBitmapEncoder> encoder;
    check_hresult(wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put()));
    check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));

    com_ptr<IWICBitmapFrameEncode> wicFrame;
    com_ptr<IPropertyBag2> frameProperties;
    check_hresult(encoder->CreateNewFrame(wicFrame.put(), frameProperties.put()));
    check_hresult(wicFrame->Initialize(frameProperties.get()));

    com_ptr<IWICImageEncoder> imageEncoder;
    check_hresult(wicFactory->CreateImageEncoder(m_d2dDevice.get(), imageEncoder.put()));
    check_hresult(imageEncoder->WriteFrame(d2dBitmap.get(), wicFrame.get(), &params));
    check_hresult(wicFrame->Commit());
    check_hresult(encoder->Commit());

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
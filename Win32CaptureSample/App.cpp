#include "pch.h"
#include "App.h"
#include "CaptureSnapshot.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Metadata;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::Imaging;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Pickers;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Popups;
}

namespace util
{
    using namespace robmikh::common::desktop;
    using namespace robmikh::common::uwp;
}

App::App(winrt::ContainerVisual root)
{
    m_mainThread = winrt::DispatcherQueue::GetForCurrentThread();
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
    m_brush.Stretch(winrt::CompositionStretch::Uniform);
    auto shadow = m_compositor.CreateDropShadow();
    shadow.Mask(m_brush);
    m_content.Shadow(shadow);
    m_root.Children().InsertAtTop(m_content);

    auto d3dDevice = util::CreateD3D11Device();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    m_device = CreateDirect3DDevice(dxgiDevice.get());

    // Don't bother with a D2D device if we can't use dirty regions
    if (winrt::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"DirtyRegionMode"))
    {
        m_dirtyRegionVisualizer = std::make_shared<DirtyRegionVisualizer>(d3dDevice);
    }
}

winrt::GraphicsCaptureItem App::TryStartCaptureFromWindowHandle(HWND hwnd)
{
    winrt::GraphicsCaptureItem item{ nullptr };
    try
    {
        item = util::CreateCaptureItemForWindow(hwnd);
        StartCaptureFromItem(item);
    }
    catch (winrt::hresult_error const& error)
    {
        MessageBoxW(m_mainWindow,
            error.message().c_str(),
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
    }
    return item;
}

winrt::GraphicsCaptureItem App::TryStartCaptureFromMonitorHandle(HMONITOR hmon)
{
    winrt::GraphicsCaptureItem item{ nullptr };
    try
    {
        item = util::CreateCaptureItemForMonitor(hmon);
        StartCaptureFromItem(item);
    }
    catch (winrt::hresult_error const& error)
    {
        MessageBoxW(m_mainWindow,
            error.message().c_str(),
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
    }
    return item;
}

winrt::IAsyncOperation<winrt::GraphicsCaptureItem> App::StartCaptureWithPickerAsync()
{
    auto capturePicker = winrt::GraphicsCapturePicker();
    InitializeObjectWithWindowHandle(capturePicker);
    auto item = co_await capturePicker.PickSingleItemAsync();
    if (item)
    {
        // We might resume on a different thread, so let's resume execution on the
        // main thread. This is important because SimpleCapture uses 
        // Direct3D11CaptureFramePool::Create, which requires the existence of
        // a DispatcherQueue. See CaptureSnapshot for an example that uses 
        // Direct3D11CaptureFramePool::CreateFreeThreaded, which doesn't now have this
        // requirement. See the README if you're unsure of which version of 'Create' to use.
        co_await wil::resume_foreground(m_mainThread);
        StartCaptureFromItem(item);
    }

    co_return item;
}

winrt::IAsyncOperation<winrt::StorageFile> App::TakeSnapshotAsync()
{
    // Use what we're currently capturing
    if (m_capture == nullptr)
    {
        co_return nullptr;
    }
    auto item = m_capture->CaptureItem();

    // Ask the user where they want to save the snapshot.
    auto savePicker = winrt::FileSavePicker();
    InitializeObjectWithWindowHandle(savePicker);
    savePicker.SuggestedStartLocation(winrt::PickerLocationId::PicturesLibrary);
    savePicker.SuggestedFileName(L"snapshot");
    savePicker.DefaultFileExtension(L".png");
    savePicker.FileTypeChoices().Clear();
    savePicker.FileTypeChoices().Insert(L"PNG image", winrt::single_threaded_vector<winrt::hstring>({ L".png" }));
    savePicker.FileTypeChoices().Insert(L"JPG image", winrt::single_threaded_vector<winrt::hstring>({ L".jpg" }));
    savePicker.FileTypeChoices().Insert(L"JXR image", winrt::single_threaded_vector<winrt::hstring>({ L".jxr" }));
    auto file = co_await savePicker.PickSaveFileAsync();
    if (file == nullptr)
    {
        co_return nullptr;
    }

    // Decide on the pixel format depending on the image type
    auto fileExtension = file.FileType();
    winrt::guid fileFormatGuid = {};
    winrt::guid bitmapPixelFormat = {};
    winrt::DirectXPixelFormat pixelFormat;
    if (fileExtension == L".png")
    {
        fileFormatGuid = GUID_ContainerFormatPng;
        bitmapPixelFormat = GUID_WICPixelFormat32bppBGRA;
        pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jpg" || fileExtension == L".jpeg")
    {
        fileFormatGuid = GUID_ContainerFormatJpeg;
        bitmapPixelFormat = GUID_WICPixelFormat32bppBGRA;
        pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jxr")
    {
        fileFormatGuid = GUID_ContainerFormatWmp;
        bitmapPixelFormat = GUID_WICPixelFormat64bppRGBAHalf;
        pixelFormat = winrt::DirectXPixelFormat::R16G16B16A16Float;
    }
    else
    {
        // Unsupported
        co_await wil::resume_foreground(m_mainThread);
        MessageBoxW(nullptr,
            L"Unsupported file format!",
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
        co_return nullptr;
    }

    // Ensure WIC
    if (!m_wicFactory)
    {
        m_wicFactory = util::CreateWICFactory();
    }

    // Take the snapshot
    auto texture = co_await CaptureSnapshot::TakeAsync(m_device, item, pixelFormat);

    {
        // Get the file stream
        auto randomAccessStream = co_await file.OpenAsync(winrt::FileAccessMode::ReadWrite);
        auto streamUnknown = randomAccessStream.as<IUnknown>();
        winrt::com_ptr<IStream> stream;
        winrt::check_hresult(CreateStreamOverRandomAccessStream(streamUnknown.get(), winrt::guid_of<IStream>(), stream.put_void()));

        // Initialize the encoder
        winrt::com_ptr<IWICBitmapEncoder> encoder;
        winrt::check_hresult(m_wicFactory->CreateEncoder(fileFormatGuid, nullptr, encoder.put()));
        winrt::check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));
        winrt::com_ptr<IWICBitmapFrameEncode> frame;
        winrt::com_ptr<IPropertyBag2> props;
        winrt::check_hresult(encoder->CreateNewFrame(frame.put(), props.put()));

        // Encode the image
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);
        auto bytes = util::CopyBytesFromTexture(texture);
        auto bytesPerPixel = util::GetBytesPerPixel(desc.Format);
        auto stride = static_cast<uint32_t>(bytesPerPixel) * desc.Width;
        auto bufferSize = stride * desc.Height;

        winrt::check_hresult(frame->Initialize(props.get()));
        winrt::check_hresult(frame->SetSize(desc.Width, desc.Height));
        winrt::guid targetFormat = bitmapPixelFormat;
        winrt::check_hresult(frame->SetPixelFormat(reinterpret_cast<WICPixelFormatGUID*>(&targetFormat)));
        if (targetFormat != bitmapPixelFormat)
        {
            // We must convert the image, but we should only really be converting to a single format.
            if (targetFormat != winrt::guid(GUID_WICPixelFormat24bppBGR))
            {
                throw winrt::hresult_error(E_FAIL, L"Unsupported pixel format!");
            }
            uint32_t convertedBytesPerPixel = 3;
            uint32_t convertedStride = convertedBytesPerPixel * desc.Width;
            uint32_t convertedBufferSize = convertedStride * desc.Height;

            winrt::com_ptr<IWICFormatConverter> converter;
            winrt::check_hresult(m_wicFactory->CreateFormatConverter(converter.put()));
            winrt::com_ptr<IWICBitmap> bitmap;
            winrt::check_hresult(m_wicFactory->CreateBitmapFromMemory(desc.Width, desc.Height, bitmapPixelFormat, stride, bufferSize, bytes.data(), bitmap.put()));

            winrt::check_hresult(converter->Initialize(bitmap.get(), targetFormat, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut));

            bytes = std::vector<byte>(convertedBufferSize, 0);
            winrt::check_hresult(converter->CopyPixels(nullptr, convertedStride, convertedBufferSize, bytes.data()));
            bytesPerPixel = convertedBytesPerPixel;
            stride = convertedStride;
            bufferSize = convertedBufferSize;
        }
        // TODO: Metadata

        winrt::check_hresult(frame->WritePixels(desc.Height, stride, bufferSize, bytes.data()));
        winrt::check_hresult(frame->Commit());
        winrt::check_hresult(encoder->Commit());
    }

    co_return file;
}

void App::StartCaptureFromItem(winrt::GraphicsCaptureItem item)
{
    m_capture = std::make_unique<SimpleCapture>(m_device, m_dirtyRegionVisualizer, item, m_pixelFormat);

    auto surface = m_capture->CreateSurface(m_compositor);
    m_brush.Surface(surface);

    m_capture->StartCapture();
}

void App::InitializeObjectWithWindowHandle(winrt::Windows::Foundation::IUnknown const& object)
{
    if (m_mainWindow == nullptr)
    {
        throw winrt::hresult_error(E_FAIL, L"App hasn't been properly initialized!");
    }

    // Provide the window handle to the pickers (explicit HWND initialization)
    auto initializer = object.as<util::IInitializeWithWindow>();
    winrt::check_hresult(initializer->Initialize(m_mainWindow));
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

void App::InitializeWithWindow(HWND window)
{
    m_mainWindow = window;
}

bool App::IsCursorEnabled()
{
    if (m_capture != nullptr)
    {
        return m_capture->IsCursorEnabled();
    }
    return false;
}

void App::IsCursorEnabled(bool value)
{
    if (m_capture != nullptr)
    {
        m_capture->IsCursorEnabled(value);
    }
}

void App::PixelFormat(winrt::DirectXPixelFormat pixelFormat)
{
    m_pixelFormat = pixelFormat;
    if (m_capture)
    {
        m_capture->SetPixelFormat(pixelFormat);
    }
}


bool App::IsBorderRequired()
{
    if (m_capture != nullptr)
    {
        return m_capture->IsBorderRequired();
    }
    return false;
}

winrt::fire_and_forget App::IsBorderRequired(bool value)
{
    if (m_capture != nullptr)
    {
        // Even if the user or system policy denies access, it's
        // still safe to set the IsBorderRequired property. In the
        // event that the policy changes, the property will be honored.
        auto ignored = co_await winrt::GraphicsCaptureAccess::RequestAccessAsync(winrt::GraphicsCaptureAccessKind::Borderless);
        m_capture->IsBorderRequired(value);
    }
}

bool App::IncludeSecondaryWindows()
{
    if (m_capture != nullptr)
    {
        return m_capture->IncludeSecondaryWindows();
    }
    return false;
}

void App::IncludeSecondaryWindows(bool value)
{
    if (m_capture != nullptr)
    {
        m_capture->IncludeSecondaryWindows(value);
    }
}

bool App::VisualizeDirtyRegions()
{
    if (m_capture != nullptr)
    {
        return m_capture->VisualizeDirtyRegions();
    }
    return false;
}

void App::VisualizeDirtyRegions(bool value)
{
    if (m_capture != nullptr)
    {
        m_capture->VisualizeDirtyRegions(value);
    }
}

winrt::GraphicsCaptureDirtyRegionMode App::DirtyRegionMode()
{
    if (m_capture != nullptr)
    {
        return m_capture->DirtyRegionMode();
    }
    return winrt::GraphicsCaptureDirtyRegionMode::ReportOnly;
}

void App::DirtyRegionMode(winrt::GraphicsCaptureDirtyRegionMode value)
{
    if (m_capture != nullptr)
    {
        m_capture->DirtyRegionMode(value);
    }
}

winrt::TimeSpan App::MinUpdateInterval()
{
    if (m_capture != nullptr)
    {
        return m_capture->MinUpdateInterval();
    }
    return winrt::TimeSpan{ 0 };
}

void App::MinUpdateInterval(winrt::TimeSpan value)
{
    if (m_capture != nullptr)
    {
        m_capture->MinUpdateInterval(value);
    }
}
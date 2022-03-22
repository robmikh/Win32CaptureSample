#include "pch.h"
#include "App.h"
#include "CaptureSnapshot.h"

namespace winrt
{
    using namespace Windows::Storage;
    using namespace Windows::Storage::Pickers;
    using namespace Windows::System;
    using namespace Windows::Foundation;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Popups;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
}

namespace util
{
    using namespace desktop;
    using namespace uwp;
}

winrt::com_ptr<IDXGIAdapter1> GetHardwareAdapter(winrt::com_ptr<IDXGIFactory1> const& factory)
{
    auto factory6 = factory.as<IDXGIFactory6>();
    winrt::com_ptr<IDXGIAdapter1> adapter;
    for (
        uint32_t adapterIndex = 0;
        SUCCEEDED(factory6->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            winrt::guid_of<IDXGIAdapter1>(),
            adapter.put_void()));
        ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see whether the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }
    return adapter;
}

App::App(winrt::ContainerVisual root, winrt::GraphicsCapturePicker capturePicker, winrt::FileSavePicker savePicker)
{
    m_capturePicker = capturePicker;
    m_savePicker = savePicker;
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

    // Create our D3D12 device
    winrt::com_ptr<IDXGIFactory4> dxgiFactory;
    winrt::check_hresult(CreateDXGIFactory1(winrt::guid_of<IDXGIFactory4>(), dxgiFactory.put_void()));
    auto adapter = GetHardwareAdapter(dxgiFactory);
    if (adapter.get() == nullptr)
    {
        winrt::check_hresult(dxgiFactory->EnumWarpAdapter(winrt::guid_of<IDXGIAdapter1>(), adapter.put_void()));
    }
    winrt::com_ptr<ID3D12Device> d3d12Device;
    winrt::check_hresult(D3D12CreateDevice(
        adapter.get(), 
        D3D_FEATURE_LEVEL_11_0, 
        winrt::guid_of<ID3D12Device>(), 
        d3d12Device.put_void()));

    // Create our command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    winrt::com_ptr<ID3D12CommandQueue> d3d12Queue;
    winrt::check_hresult(d3d12Device->CreateCommandQueue(&queueDesc, winrt::guid_of<ID3D12CommandQueue>(), d3d12Queue.put_void()));

    // Wrap our D3D12 device with a D3D11 device
    winrt::com_ptr<ID3D11Device> d3d11Device;
    winrt::com_ptr<ID3D11DeviceContext> d3d11Context;
    auto d3d12QueuePointer = d3d12Queue.get();
    winrt::check_hresult(D3D11On12CreateDevice(
        d3d12Device.get(),
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        reinterpret_cast<IUnknown**>(&d3d12QueuePointer),
        1,
        0,
        d3d11Device.put(),
        d3d11Context.put(),
        nullptr));

    // Get the 11on12 interface
    auto d3d11on12Device = d3d11Device.as<ID3D11On12Device>();

    // Initialize as usual
    auto dxgiDevice = d3d11Device.as<IDXGIDevice>();
    m_device = CreateDirect3DDevice(dxgiDevice.get());

    m_dxgiFactory = dxgiFactory;
    m_d3d12Device = d3d12Device;
    m_d3d12Queue = d3d12Queue;
    m_d3d11on12Device = d3d11on12Device;

    m_encoder = std::make_unique<SimpleImageEncoder>(m_device);
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
        MessageBoxW(nullptr,
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
        MessageBoxW(nullptr,
            error.message().c_str(),
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);
    }
    return item;
}

winrt::IAsyncOperation<winrt::GraphicsCaptureItem> App::StartCaptureWithPickerAsync()
{
    auto item = co_await m_capturePicker.PickSingleItemAsync();
    if (item)
    {
        // We might resume on a different thread, so let's resume execution on the
        // main thread. This is important because SimpleCapture uses 
        // Direct3D11CaptureFramePool::Create, which requires the existence of
        // a DispatcherQueue. See CaptureSnapshot for an example that uses 
        // Direct3D11CaptureFramePool::CreateFreeThreaded, which doesn't now have this
        // requirement. See the README if you're unsure of which version of 'Create' to use.
        co_await m_mainThread;
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
    m_savePicker.SuggestedStartLocation(winrt::PickerLocationId::PicturesLibrary);
    m_savePicker.SuggestedFileName(L"snapshot");
    m_savePicker.DefaultFileExtension(L".png");
    m_savePicker.FileTypeChoices().Clear();
    m_savePicker.FileTypeChoices().Insert(L"PNG image", winrt::single_threaded_vector<winrt::hstring>({ L".png" }));
    m_savePicker.FileTypeChoices().Insert(L"JPG image", winrt::single_threaded_vector<winrt::hstring>({ L".jpg" }));
    m_savePicker.FileTypeChoices().Insert(L"JXR image", winrt::single_threaded_vector<winrt::hstring>({ L".jxr" }));
    auto file = co_await m_savePicker.PickSaveFileAsync();
    if (file == nullptr)
    {
        co_return nullptr;
    }

    // Decide on the pixel format depending on the image type
    auto fileExtension = file.FileType();
    SimpleImageEncoder::SupportedFormats fileFormat;
    winrt::DirectXPixelFormat pixelFormat;
    if (fileExtension == L".png")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Png;
        pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jpg" || fileExtension == L".jpeg")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Jpg;
        pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    }
    else if (fileExtension == L".jxr")
    {
        fileFormat = SimpleImageEncoder::SupportedFormats::Jxr;
        pixelFormat = winrt::DirectXPixelFormat::R16G16B16A16Float;
    }
    else
    {
        // Unsupported
        auto dialog = winrt::MessageDialog(L"Unsupported file format!");

        co_await dialog.ShowAsync();
        co_return nullptr;
    }

    // Get the file stream
    auto stream = co_await file.OpenAsync(winrt::FileAccessMode::ReadWrite);

    // Take the snapshot
    auto frame = co_await CaptureSnapshot::TakeAsync(m_device, item, pixelFormat);
    
    // Encode the image
    m_encoder->EncodeImage(frame, stream, fileFormat);

    co_return file;
}

void App::StartCaptureFromItem(winrt::GraphicsCaptureItem item)
{
    m_capture = std::make_unique<SimpleCapture>(m_device, m_dxgiFactory, m_d3d12Queue, m_d3d11on12Device, item, m_pixelFormat);

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
#pragma once

class CaptureSnapshot 
{
public:
    static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface>
        TakeAsync(
            winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
            winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item,
			winrt::Windows::Graphics::DirectX::DirectXPixelFormat const& format = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized);

private:
    CaptureSnapshot() = delete;
};
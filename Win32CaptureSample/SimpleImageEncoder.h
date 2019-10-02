#pragma once

class SimpleImageEncoder
{
public:
    SimpleImageEncoder(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device);
    ~SimpleImageEncoder() {}

    enum class SupportedFormats
    {
        Png,
        Jpg,
        Jxr
    };

    void EncodeImage(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface const& surface,
        winrt::Windows::Storage::Streams::IRandomAccessStream const& stream, 
        SupportedFormats const& format = SupportedFormats::Png);

private:
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<ID2D1Factory1> m_d2dFactory;
    winrt::com_ptr<ID2D1Device> m_d2dDevice;
    winrt::com_ptr<ID2D1DeviceContext> m_d2dContext;
	winrt::com_ptr<IWICImagingFactory2> m_wicFactory;
};
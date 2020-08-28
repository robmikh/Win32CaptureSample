#include "pch.h"
#include "SimpleImageEncoder.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Streams;
}

namespace util
{
    using namespace uwp;
}

struct WICSettings
{
    GUID ContainerFormat;
    WICPixelFormatGUID PixelFormat;
};

WICSettings GetWICSettingsForFormat(SimpleImageEncoder::SupportedFormats format)
{
    switch (format)
    {
    case SimpleImageEncoder::SupportedFormats::Jpg:
        return { GUID_ContainerFormatJpeg, GUID_WICPixelFormat32bppBGRA };
    case SimpleImageEncoder::SupportedFormats::Png:
        return { GUID_ContainerFormatPng, GUID_WICPixelFormat32bppBGRA };
    case SimpleImageEncoder::SupportedFormats::Jxr:
        return { GUID_ContainerFormatWmp, GUID_WICPixelFormat64bppRGBAHalf };
    }
    throw winrt::hresult_invalid_argument();
}

SimpleImageEncoder::SimpleImageEncoder(winrt::IDirect3DDevice const& device)
{
    m_device = device;
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    m_d2dFactory = util::CreateD2DFactory();
    m_d2dDevice = util::CreateD2DDevice(m_d2dFactory, d3dDevice);
    winrt::check_hresult(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.put()));
	m_wicFactory = util::CreateWICFactory();
}

void SimpleImageEncoder::EncodeImage(winrt::IDirect3DSurface const& surface, winrt::IRandomAccessStream const& stream, SupportedFormats const& format)
{
    auto abiStream = util::CreateStreamFromRandomAccessStream(stream);

    // Get the texture and setup the D2D bitmap
    auto frameTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);
    D3D11_TEXTURE2D_DESC textureDesc = {};
    frameTexture->GetDesc(&textureDesc);
    auto dxgiFrameTexture = frameTexture.as<IDXGISurface>();
    // TODO: Since this sample doesn't use D2D any other way, it may be better to map 
    //       the pixels manually and hand them to WIC. However, using d2d is easier for now.
    winrt::com_ptr<ID2D1Bitmap1> d2dBitmap;
    winrt::check_hresult(m_d2dContext->CreateBitmapFromDxgiSurface(dxgiFrameTexture.get(), nullptr, d2dBitmap.put()));

    // Get the WIC settings for the given format
    auto wicSettings = GetWICSettingsForFormat(format);

    // Encode the image
    // TODO: dpi?
    auto dpi = 96.0f;
    WICImageParameters params = {};
    params.PixelFormat.format = textureDesc.Format;
    params.PixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    params.DpiX = dpi;
    params.DpiY = dpi;
    params.PixelWidth = textureDesc.Width;
    params.PixelHeight = textureDesc.Height;
    
    winrt::com_ptr<IWICBitmapEncoder> encoder;
    winrt::check_hresult(m_wicFactory->CreateEncoder(wicSettings.ContainerFormat, nullptr, encoder.put()));
    winrt::check_hresult(encoder->Initialize(abiStream.get(), WICBitmapEncoderNoCache));

    winrt::com_ptr<IWICBitmapFrameEncode> wicFrame;
    winrt::com_ptr<IPropertyBag2> frameProperties;
    winrt::check_hresult(encoder->CreateNewFrame(wicFrame.put(), frameProperties.put()));
    winrt::check_hresult(wicFrame->Initialize(frameProperties.get()));
    auto wicPixelFormat = wicSettings.PixelFormat;
    winrt::check_hresult(wicFrame->SetPixelFormat(&wicPixelFormat));

    winrt::com_ptr<IWICImageEncoder> imageEncoder;
    winrt::check_hresult(m_wicFactory->CreateImageEncoder(m_d2dDevice.get(), imageEncoder.put()));
    winrt::check_hresult(imageEncoder->WriteFrame(d2dBitmap.get(), wicFrame.get(), &params));
    winrt::check_hresult(wicFrame->Commit());
    winrt::check_hresult(encoder->Commit());
}
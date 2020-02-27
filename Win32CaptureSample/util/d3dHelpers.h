#pragma once
#include "composition.interop.h"

namespace util::uwp
{
    struct SurfaceContext
    {
    public:
        SurfaceContext(std::nullptr_t) {}
        SurfaceContext(
            winrt::Windows::UI::Composition::CompositionDrawingSurface surface)
        {
            m_surface = surface;
            m_d2dContext = SurfaceBeginDraw(m_surface);
        }
        ~SurfaceContext()
        {
            SurfaceEndDraw(m_surface);
            m_d2dContext = nullptr;
            m_surface = nullptr;
        }

        winrt::com_ptr<ID2D1DeviceContext> GetDeviceContext() { return m_d2dContext; }

    private:
        winrt::com_ptr<ID2D1DeviceContext> m_d2dContext;
        winrt::Windows::UI::Composition::CompositionDrawingSurface m_surface{ nullptr };
    };

    struct D3D11DeviceLock
    {
    public:
        D3D11DeviceLock(std::nullopt_t) {}
        D3D11DeviceLock(ID3D11Multithread* pMultithread)
        {
            m_multithread.copy_from(pMultithread);
            m_multithread->Enter();
        }
        ~D3D11DeviceLock()
        {
            m_multithread->Leave();
            m_multithread = nullptr;
        }
    private:
        winrt::com_ptr<ID3D11Multithread> m_multithread;
    };

    inline auto CreateWICFactory()
    {
        return winrt::create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory2);
    }

    inline auto CreateD2DDevice(winrt::com_ptr<ID2D1Factory1> const& factory, winrt::com_ptr<ID3D11Device> const& device)
    {
        winrt::com_ptr<ID2D1Device> result;
        winrt::check_hresult(factory->CreateDevice(device.as<IDXGIDevice>().get(), result.put()));
        return result;
    }

    inline auto CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
    {
        WINRT_ASSERT(!device);

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        //#ifdef _DEBUG
        //	flags |= D3D11_CREATE_DEVICE_DEBUG;
        //#endif

        return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.put(),
            nullptr, nullptr);
    }

    inline auto CreateD3DDevice()
    {
        winrt::com_ptr<ID3D11Device> device;
        HRESULT hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, device);
        if (DXGI_ERROR_UNSUPPORTED == hr)
        {
            hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, device);
        }

        winrt::check_hresult(hr);
        return device;
    }

    inline auto CreateD2DFactory()
    {
        D2D1_FACTORY_OPTIONS options{};

        //#ifdef _DEBUG
        //	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
        //#endif

        winrt::com_ptr<ID2D1Factory1> factory;
        winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, factory.put()));
        return factory;
    }

    inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device, const DXGI_SWAP_CHAIN_DESC1* desc)
    {
        auto dxgiDevice = device.as<IDXGIDevice2>();
        winrt::com_ptr<IDXGIAdapter> adapter;
        winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
        winrt::com_ptr<IDXGIFactory2> factory;
        winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

        winrt::com_ptr<IDXGISwapChain1> swapchain;
        winrt::check_hresult(factory->CreateSwapChainForComposition(device.get(), desc, nullptr, swapchain.put()));
        return swapchain;
    }

    inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device,
        uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferCount = bufferCount;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

        return CreateDXGISwapChain(device, &desc);
    }

    inline auto CopyD3DTexture(winrt::com_ptr<ID3D11Device> const& device, winrt::com_ptr<ID3D11Texture2D> const& texture, bool asStagingTexture)
    {
        winrt::com_ptr<ID3D11DeviceContext> context;
        device->GetImmediateContext(context.put());

        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);
        // Clear flags that we don't need
        desc.Usage = asStagingTexture ? D3D11_USAGE_STAGING : D3D11_USAGE_DEFAULT;
        desc.BindFlags = asStagingTexture ? 0 : D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = asStagingTexture ? D3D11_CPU_ACCESS_READ : 0;
        desc.MiscFlags = 0;

        // Create and fill the texture copy
        winrt::com_ptr<ID3D11Texture2D> textureCopy;
        winrt::check_hresult(device->CreateTexture2D(&desc, nullptr, textureCopy.put()));
        context->CopyResource(textureCopy.get(), texture.get());

        return textureCopy;
    }
}
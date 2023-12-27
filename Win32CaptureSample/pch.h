#pragma once

// Windows SDK support
#include <Unknwn.h>
#include <inspectable.h>

// Needs to come before C++/WinRT headers
#include <wil/cppwinrt.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>

// STL
#include <atomic>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <optional>
#include <future>
#include <mutex>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// DWM
#include <dwmapi.h>

// WIL
#include <wil/resource.h>
#include <wil/cppwinrt_helpers.h>

// Helpers
#include <robmikh.common/composition.interop.h>
#include <robmikh.common/composition.desktop.interop.h>
#include <robmikh.common/d3dHelpers.h>
#include <robmikh.common/d3dHelpers.desktop.h>
#include <robmikh.common/direct3d11.interop.h>
#include <robmikh.common/capture.desktop.interop.h>
// robmikh.common needs to be updated to support newer versions of C++/WinRT
//#include <robmikh.common/dispatcherqueue.desktop.interop.h>
#include "dispatcherqueue.desktop.interop.h"
#include <robmikh.common/stream.interop.h>
#include <robmikh.common/hwnd.interop.h>
#include <robmikh.common/ControlsHelper.h>

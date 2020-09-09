#pragma once

#include <Unknwn.h>
#include <inspectable.h>

#include <wil/cppwinrt.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

// STL
#include <atomic>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <optional>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// DWM
#include <dwmapi.h>

// WIL
#include <wil/resource.h>

// Helpers
#include "util/composition.interop.h"
#include "util/composition.desktop.interop.h"
#include "util/d3dHelpers.h"
#include "util/d3dHelpers.desktop.h"
#include "util/direct3d11.interop.h"
#include "util/capture.desktop.interop.h"
#include "util/dispatcherqueue.desktop.interop.h"
#include "util/stream.interop.h"
#include "util/hwnd.interop.h"
#include "completionSource.h"
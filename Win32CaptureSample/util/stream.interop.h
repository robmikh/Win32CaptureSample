#pragma once
#include <shcore.h>
#include <windows.storage.streams.h>

inline auto CreateStreamFromRandomAccessStream(
    winrt::Windows::Storage::Streams::IRandomAccessStream const& stream)
{
    winrt::com_ptr<IStream> result;
    auto unknown = stream.as<::IUnknown>();
    winrt::check_hresult(CreateStreamOverRandomAccessStream( unknown.get(), winrt::guid_of<IStream>(), result.put_void()));
    return result;
}
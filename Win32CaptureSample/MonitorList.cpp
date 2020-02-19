#include "pch.h"
#include "MonitorList.h"

std::vector<MonitorInfo> EnumerateAllMonitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT, LPARAM lparam)
    {
        auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(lparam);
        monitors.push_back(MonitorInfo(hmon));

        return TRUE;
    }, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

MonitorList::MonitorList()
{
    m_monitors = EnumerateAllMonitors();
}

void MonitorList::Update()
{
    auto monitors = EnumerateAllMonitors();
    std::map<HMONITOR, MonitorInfo> newMonitors;
    for (auto& monitor : monitors)
    {
        newMonitors.insert({ monitor.MonitorHandle, monitor });
    }

    std::vector<int> monitorIndexesToRemove;
    auto index = 0;
    for (auto& monitor : m_monitors)
    {
        auto search = newMonitors.find(monitor.MonitorHandle);
        if (search == newMonitors.end())
        {
            monitorIndexesToRemove.push_back(index);
        }
        else
        {
            newMonitors.erase(search);
        }
        index++;
    }

    // Remove old monitors
    for (auto& removalIndex : monitorIndexesToRemove)
    {
        m_monitors.erase(m_monitors.begin() + removalIndex);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_DELETESTRING, removalIndex, 0)));
        }
    }

    // Add new monitors
    for (auto& pair : newMonitors)
    {
        auto monitor = pair.second;
        m_monitors.push_back(monitor);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName.c_str())));
        }
    }
}

void MonitorList::ForceUpdateComboBox(HWND comboBoxHandle)
{
    winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_RESETCONTENT, 0, 0)));
    for (auto& monitor : m_monitors)
    {
        winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName.c_str())));
    }
}
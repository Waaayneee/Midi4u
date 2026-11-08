#include "Platform.h"

#if JUCE_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <mmdeviceapi.h>
    #include <propidl.h>
#endif

namespace platform
{

#if JUCE_WINDOWS

juce::String cableOutputPrefix() { return "CABLE Input"; }
juce::String cableDiscordInputName() { return "CABLE Output"; }
juce::URL cableDownloadPage() { return juce::URL("https://vb-audio.com/Cable/"); }

juce::File vst3Folder()
{
    // C:\Program Files\Common Files\VST3
    return juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory).getChildFile("Common Files").getChildFile("VST3");
}

juce::File appDataFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Midi4u");
}

namespace
{
const juce::String runKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\Midi4u";

template <typename T>
struct ComRelease
{
    T* p = nullptr;
    ~ComRelease()
    {
        if (p != nullptr)
            p->Release();
    }
};

juce::String endpointId(IMMDevice* device)
{
    juce::String result;
    LPWSTR id = nullptr;

    if (SUCCEEDED(device->GetId(&id)) && id != nullptr)
    {
        result = juce::String(id);
        CoTaskMemFree(id);
    }

    return result;
}
} // namespace

bool supportsStartWithSystem() { return true; }

void setStartWithSystem(bool enabled)
{
    if (enabled)
    {
        const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
        juce::WindowsRegistry::setValue(runKey, "\"" + exe + "\" --tray");
    }
    else
    {
        juce::WindowsRegistry::deleteValue(runKey);
    }
}

std::vector<std::string> systemDeviceIds(bool input, const juce::StringArray& juceNames)
{
    // Rebuilds the list exactly as JUCE's WASAPI scan does (all active endpoints, default
    // first, friendly names, numbers appended to duplicates) and maps each name to its ID.
    std::vector<std::string> result(size_t(juceNames.size()));

    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool mustUninit = SUCCEEDED(init);

    {
        ComRelease<IMMDeviceEnumerator> enumerator;

        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                    reinterpret_cast<void**>(&enumerator.p))))
        {
            if (mustUninit)
                CoUninitialize();
            return result;
        }

        const EDataFlow wantedFlow = input ? eCapture : eRender;
        juce::String defaultId;

        {
            ComRelease<IMMDevice> device;
            if (SUCCEEDED(enumerator.p->GetDefaultAudioEndpoint(wantedFlow, eMultimedia, &device.p)))
                defaultId = endpointId(device.p);
        }

        juce::StringArray names, ids;
        ComRelease<IMMDeviceCollection> collection;
        UINT count = 0;

        if (SUCCEEDED(enumerator.p->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &collection.p))
            && SUCCEEDED(collection.p->GetCount(&count)))
        {
            const PROPERTYKEY friendlyNameKey = {{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

            for (UINT i = 0; i < count; ++i)
            {
                ComRelease<IMMDevice> device;
                if (FAILED(collection.p->Item(i, &device.p)))
                    continue;

                ComRelease<IMMEndpoint> endpoint;
                EDataFlow flow = eAll;
                if (FAILED(device.p->QueryInterface(__uuidof(IMMEndpoint), reinterpret_cast<void**>(&endpoint.p)))
                    || FAILED(endpoint.p->GetDataFlow(&flow)) || flow != wantedFlow)
                    continue;

                ComRelease<IPropertyStore> properties;
                if (FAILED(device.p->OpenPropertyStore(STGM_READ, &properties.p)))
                    continue;

                juce::String name;
                PROPVARIANT value;
                PropVariantInit(&value);
                if (SUCCEEDED(properties.p->GetValue(friendlyNameKey, &value)) && value.vt == VT_LPWSTR)
                    name = juce::String(value.pwszVal);
                PropVariantClear(&value);

                const auto id = endpointId(device.p);
                const int index = id == defaultId ? 0 : -1;
                names.insert(index, name);
                ids.insert(index, id);
            }
        }

        names.appendNumbersToDuplicates(false, false);

        for (int i = 0; i < juceNames.size(); ++i)
        {
            const int match = names.indexOf(juceNames[i]);
            if (match >= 0)
                result[size_t(i)] = ids[match].toStdString();
        }
    }

    if (mustUninit)
        CoUninitialize();

    return result;
}

#else // macOS (development and v3)

juce::String cableOutputPrefix() { return "BlackHole 2ch"; }
juce::String cableDiscordInputName() { return "BlackHole 2ch"; }
juce::URL cableDownloadPage() { return juce::URL("https://existential.audio/blackhole/"); }

juce::File vst3Folder()
{
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Library/Audio/Plug-Ins/VST3");
}

juce::File appDataFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support/Midi4u");
}

bool supportsStartWithSystem() { return false; }
void setStartWithSystem(bool) {}

std::vector<std::string> systemDeviceIds(bool, const juce::StringArray& juceNames)
{
    return std::vector<std::string>(size_t(juceNames.size()));
}

#endif

} // namespace platform

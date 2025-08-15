#include "al2_proxy.h"
#include "log.h"

AMD::AntiLag2DX12::PFNAmdExtD3DCreateInterface AL2Proxy::o_AmdExtD3DCreateInterface = nullptr;
bool AL2Proxy::disableAl2Kill = false;

HRESULT AL2Proxy::hkAmdExtD3DCreateInterface(IUnknown* pOuter, REFIID riid, void** ppvObject) {
    if (o_AmdExtD3DCreateInterface == nullptr)
        return E_NOINTERFACE;

    if (riid == IID_IAmdExtAntiLagApi && !disableAl2Kill) {
        spdlog::info("Killing native AL2");
        return E_NOINTERFACE;
    }

    return o_AmdExtD3DCreateInterface(pOuter, riid, ppvObject);
}

void AL2Proxy::hookAntiLag() {
    HMODULE hModule = GetModuleHandleA("amdxc64.dll");

    if (hModule && o_AmdExtD3DCreateInterface == nullptr) {
        o_AmdExtD3DCreateInterface = reinterpret_cast<AMD::AntiLag2DX12::PFNAmdExtD3DCreateInterface>( (VOID*)GetProcAddress(hModule, "AmdExtD3DCreateInterface") );

        if (o_AmdExtD3DCreateInterface) {
            spdlog::info("AmdExtD3DCreateInterface hooked");

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            DetourAttach(&(PVOID&) o_AmdExtD3DCreateInterface, hkAmdExtD3DCreateInterface);

            DetourTransactionCommit();
        }
    }
}
#pragma once

#include <d3d12.h>
#include "../external/ffx_antilag2_dx12.h"
#include <detours.h>

class AL2Proxy {
    public:
        static AMD::AntiLag2DX12::PFNAmdExtD3DCreateInterface o_AmdExtD3DCreateInterface;
        static bool disableAl2Kill;

        static HRESULT hkAmdExtD3DCreateInterface(IUnknown* pOuter, REFIID riid, void** ppvObject);
        static void hookAntiLag();
};
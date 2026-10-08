#include <Windows.h>
#include <d3d11.h>
#include <MinHook.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <atomic>

#include "aimbot/aimbot.hpp"
#include "visual/visual.hpp"
#include "menu/menu.hpp"
#include "tools/console.hpp"

static ID3D11Device* g_pd3dDevice = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static ID3D11RenderTargetView* g_mainRTV = nullptr;
static HWND g_hwnd = nullptr;
static void* origin_present = nullptr;
static WNDPROC origin_wndProc = nullptr;
static bool g_menu_open = true;
static bool g_insert_prev = false;
static std::atomic<bool> g_inited{false};

using Present = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT __stdcall WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return 1;
    return CallWindowProc(origin_wndProc, hwnd, uMsg, wParam, lParam);
}

long __stdcall my_present(IDXGISwapChain* _this, UINT a, UINT b) {
    if (!g_inited.load()) {
        _this->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice);
        g_pd3dDevice->GetImmediateContext(&g_pd3dContext);

        DXGI_SWAP_CHAIN_DESC sd;
        _this->GetDesc(&sd);
        g_hwnd = sd.OutputWindow;

        ID3D11Texture2D* buf = nullptr;
        _this->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
        g_pd3dDevice->CreateRenderTargetView(buf, nullptr, &g_mainRTV);
        buf->Release();

        origin_wndProc = (WNDPROC)SetWindowLongPtr(
            g_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        static const ImWchar ranges[] = {
            0x0020, 0x00FF,
            0x0100, 0x024F,
            0x2000, 0x206F,
            0x3000, 0x30FF,
            0x4E00, 0x9FA5,
            0xFF00, 0xFFEF,
            0,
        };
        io.Fonts->AddFontFromFileTTF(
            R"(C:\Windows\Fonts\msyh.ttc)", 18.0f, &cfg, ranges);

        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

        g_inited.store(true);
    }

    // F11 menu 开关
    bool insert_now = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
    if (insert_now && !g_insert_prev) {
        g_menu_open = !g_menu_open;
    }
    g_insert_prev = insert_now;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const std::uintptr_t aim_target = aimbot::run_once();

    if (aimbot::g_enabled) {
        visual::draw_aim_circle(aimbot::g_circle_radius);
        visual::draw_target_marker(aim_target);
    }

    visual::draw_esp();

    if (g_menu_open) {
        menu::draw();
    }

    ImGui::End();
    ImGui::Render();

    if (g_mainRTV) {
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRTV, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return ((Present)origin_present)(_this, a, b);
}

DWORD WINAPI create(LPVOID) {
    constexpr unsigned level_count = 2;
    constexpr D3D_FEATURE_LEVEL levels[level_count] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    HWND tmp = CreateWindowExA(
        0, "STATIC", "", WS_POPUP,
        0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount        = 1;
    sd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow       = tmp;
    sd.SampleDesc.Count   = 1;
    sd.Windowed           = TRUE;
    sd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, level_count, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice,
        nullptr, nullptr);

    if (FAILED(hr) || !g_pSwapChain) {
        if (tmp) DestroyWindow(tmp);
        if (g_pd3dDevice)  { g_pd3dDevice->Release();  g_pd3dDevice  = nullptr; }
        if (g_pSwapChain)  { g_pSwapChain->Release();  g_pSwapChain  = nullptr; }
        return 0;
    }

    void** vtable = *reinterpret_cast<void***>(g_pSwapChain);
    void*  present = vtable[8];

    MH_RemoveHook(present);

    MH_STATUS st = MH_CreateHook(present, LPVOID(my_present), &origin_present);
    if (st != MH_OK) {
        if (tmp) DestroyWindow(tmp);
        g_pd3dDevice->Release();  g_pd3dDevice  = nullptr;
        g_pSwapChain->Release();  g_pSwapChain  = nullptr;
        return 0;
    }

    st = MH_EnableHook(present);
    if (st != MH_OK) {
        MH_RemoveHook(present);
        if (tmp) DestroyWindow(tmp);
        g_pd3dDevice->Release();  g_pd3dDevice  = nullptr;
        g_pSwapChain->Release();  g_pSwapChain  = nullptr;
        return 0;
    }

    if (tmp) DestroyWindow(tmp);

    g_pd3dDevice->Release();  g_pd3dDevice  = nullptr;
    g_pSwapChain->Release();  g_pSwapChain  = nullptr;

    return 0;
}

BOOL __stdcall DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        // tools::console::open();

        MH_STATUS st = MH_Initialize();
        if (st != MH_OK) return FALSE;

        HANDLE h = CreateThread(nullptr, 0, create, nullptr, 0, nullptr);
        if (!h) return FALSE;
    }
    return TRUE;
}
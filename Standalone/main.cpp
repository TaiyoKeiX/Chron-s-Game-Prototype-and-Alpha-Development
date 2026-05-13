#include "GameState.h"
#include "ScreenMainMenu.h"
#include "ScreenLevelSelect.h"
#include "ScreenCharacterUpgrade.h"
#include "ScreenBattle.h"
#include "ScreenBattleResult.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    static ID3D11Device* GDevice = nullptr;
    static ID3D11DeviceContext* GDeviceContext = nullptr;
    static IDXGISwapChain* GSwapChain = nullptr;
    static ID3D11RenderTargetView* GMainRenderTargetView = nullptr;

    void CreateRenderTarget()
    {
        ID3D11Texture2D* BackBuffer = nullptr;
        GSwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
        GDevice->CreateRenderTargetView(BackBuffer, nullptr, &GMainRenderTargetView);
        BackBuffer->Release();
    }

    void CleanupRenderTarget()
    {
        if (GMainRenderTargetView)
        {
            GMainRenderTargetView->Release();
            GMainRenderTargetView = nullptr;
        }
    }

    bool CreateDeviceD3D(HWND Hwnd)
    {
        DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
        SwapChainDesc.BufferCount = 2;
        SwapChainDesc.BufferDesc.Width = 0;
        SwapChainDesc.BufferDesc.Height = 0;
        SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDesc.OutputWindow = Hwnd;
        SwapChainDesc.SampleDesc.Count = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
        SwapChainDesc.Windowed = TRUE;
        SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT CreateDeviceFlags = 0;
#if defined(_DEBUG)
        CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL FeatureLevel;
        const D3D_FEATURE_LEVEL FeatureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
        const HRESULT Result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            CreateDeviceFlags,
            FeatureLevelArray,
            1,
            D3D11_SDK_VERSION,
            &SwapChainDesc,
            &GSwapChain,
            &GDevice,
            &FeatureLevel,
            &GDeviceContext);

        if (Result != S_OK)
        {
            return false;
        }

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D()
    {
        CleanupRenderTarget();
        if (GSwapChain) { GSwapChain->Release(); GSwapChain = nullptr; }
        if (GDeviceContext) { GDeviceContext->Release(); GDeviceContext = nullptr; }
        if (GDevice) { GDevice->Release(); GDevice = nullptr; }
    }

    LRESULT WINAPI MainWindowProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam))
        {
            return true;
        }

        switch (Msg)
        {
        case WM_SIZE:
            if (GDevice != nullptr && WParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();
                GSwapChain->ResizeBuffers(0, static_cast<UINT>(LOWORD(LParam)), static_cast<UINT>(HIWORD(LParam)), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((WParam & 0xfff0) == SC_KEYMENU)
            {
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProcA(Hwnd, Msg, WParam, LParam);
    }
}

int main()
{
    WNDCLASSEXA WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_CLASSDC;
    WindowClass.lpfnWndProc = MainWindowProc;
    WindowClass.hInstance = GetModuleHandleA(nullptr);
    WindowClass.lpszClassName = "BioStandaloneWindow";
    RegisterClassExA(&WindowClass);

    HWND Window = CreateWindowA(
        WindowClass.lpszClassName,
        "Chron's Game - Immune System Tactics",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1400, 800,
        nullptr, nullptr, WindowClass.hInstance, nullptr);

    if (!CreateDeviceD3D(Window))
    {
        CleanupDeviceD3D();
        UnregisterClassA(WindowClass.lpszClassName, WindowClass.hInstance);
        return 1;
    }

    ShowWindow(Window, SW_SHOWDEFAULT);
    UpdateWindow(Window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    // Tweak default style for a cleaner look
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 4.0f;
    Style.FrameRounding = 3.0f;
    Style.GrabRounding = 3.0f;
    Style.WindowBorderSize = 0.0f;

    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX11_Init(GDevice, GDeviceContext);

    FGameState GameState;

    bool Running = true;
    while (Running)
    {
        MSG Message;
        while (PeekMessageA(&Message, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
            if (Message.message == WM_QUIT)
            {
                Running = false;
            }
        }
        if (!Running)
        {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        switch (GameState.CurrentScreen)
        {
        case EGameScreen::MainMenu:
            RenderMainMenu(GameState);
            break;
        case EGameScreen::LevelSelect:
            RenderLevelSelect(GameState);
            break;
        case EGameScreen::CharacterUpgrade:
            RenderCharacterUpgrade(GameState);
            break;
        case EGameScreen::Battle:
            RenderBattle(GameState);
            break;
        case EGameScreen::BattleResult:
            RenderBattleResult(GameState);
            break;
        }

        ImGui::Render();
        const float ClearColor[4] = { 0.06f, 0.06f, 0.08f, 1.0f };
        GDeviceContext->OMSetRenderTargets(1, &GMainRenderTargetView, nullptr);
        GDeviceContext->ClearRenderTargetView(GMainRenderTargetView, ClearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        GSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(Window);
    UnregisterClassA(WindowClass.lpszClassName, WindowClass.hInstance);

    return 0;
}

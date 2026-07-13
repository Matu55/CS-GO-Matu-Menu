#include "gui.h"

#include "../external/imgui-legacy/imgui.h"
#include "../external/imgui-legacy/imgui_impl_dx11.h"
#include "../external/imgui-legacy/imgui_impl_win32.h"

#include "globals.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window, 
    UINT message, 
    WPARAM wideParameter,
    LPARAM longParameter
);

LRESULT __stdcall WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
        return true;

    switch (message) {
        case WM_DPICHANGED: {
            // Handle DPI change
            const UINT dpi = HIWORD(wideParameter);
            gui::dpiScale = dpi / 96.0f;

            // Resize window according to suggested rect
            const RECT* suggestedRect = (RECT*)longParameter;
            SetWindowPos(window, nullptr, suggestedRect->left, suggestedRect->top,
                         suggestedRect->right - suggestedRect->left, suggestedRect->bottom - suggestedRect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);

            // Recreate render target with new size
            if (gui::device) {
                gui::DestroyRenderTarget();
                gui::swapChain->ResizeBuffers(0, suggestedRect->right - suggestedRect->left,
                                              suggestedRect->bottom - suggestedRect->top, DXGI_FORMAT_UNKNOWN, 0);
                gui::CreateRenderTarget();
            }
        }
            return 0;

        case WM_SIZE: {
            if (gui::device && wideParameter != SIZE_MINIMIZED) {
                gui::DestroyRenderTarget();
                gui::swapChain->ResizeBuffers(0, (UINT)LOWORD(longParameter), (UINT)HIWORD(longParameter),
                                              DXGI_FORMAT_UNKNOWN, 0);
                gui::CreateRenderTarget();
            }
        }
            return 0;

        case WM_SYSCOMMAND: {
            if ((wideParameter & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
                return 0;
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
        }
            return 0;

        case WM_LBUTTONDOWN: {
            gui::position = MAKEPOINTS(longParameter);  // set click points
        }
            return 0;

        case WM_MOUSEMOVE: {
            if (wideParameter == MK_LBUTTON) {
                const auto points = MAKEPOINTS(longParameter);
                auto rect = ::RECT{};

                GetWindowRect(gui::window, &rect);

                rect.left += points.x - gui::position.x;
                rect.top += points.y - gui::position.y;

                if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 &&
                    gui::position.y <= 19)
                    SetWindowPos(gui::window, HWND_TOPMOST, rect.left, rect.top, 0, 0,
                                 SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
            return 0;
    }

    return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(std::wstring_view title) {
    // Enable DPI awareness if configured
    if (isDPIAware) {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = WindowProcess;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hIcon = 0;
    windowClass.hCursor = 0;
    windowClass.hbrBackground = 0;
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = L"class001";
    windowClass.hIconSm = 0;

    RegisterClassEx(&windowClass);

    // Get DPI for the primary monitor and calculate scaling
    if (isDPIAware) {
        const UINT dpi = GetDpiForWindow(GetDesktopWindow());
        dpiScale = dpi / 96.0f + 1.f;
    }

    // Calculate DPI-scaled window size
    const int scaledWidth = static_cast<int>(WIDTH * dpiScale);
    const int scaledHeight = static_cast<int>(HEIGHT * dpiScale);

    window = CreateWindowEx(0, windowClass.lpszClassName, title.data(), WS_POPUP, 100, 100, scaledWidth, scaledHeight,
                            0, 0, windowClass.hInstance, 0);

    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept {
    DestroyWindow(window);
    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
                                      D3D11_SDK_VERSION, &sd, &swapChain, &device, &featureLevel,
                                      &deviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void gui::DestroyDevice() noexcept {
    DestroyRenderTarget();
    if (swapChain) {
        swapChain->Release();
        swapChain = nullptr;
    }
    if (deviceContext) {
        deviceContext->Release();
        deviceContext = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
}

void gui::CreateRenderTarget() noexcept {
    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device->CreateRenderTargetView(backBuffer, NULL, &mainRenderTargetView);
    backBuffer->Release();
}

void gui::DestroyRenderTarget() noexcept {
    if (mainRenderTargetView) {
        mainRenderTargetView->Release();
        mainRenderTargetView = nullptr;
    }
}

void gui::CreateImGui() noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ::ImGui::GetIO();

    io.IniFilename = NULL;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, deviceContext);

    // Enable DPI awareness - ImGui handles scaling internally
    if (isDPIAware) {
        ImGui_ImplWin32_EnableDpiAwareness();
    }
}

void gui::DestroyImGui() noexcept {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void gui::BeginRender() noexcept {
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);

        if (message.message == WM_QUIT) {
            isRunning = false;
            return;
        }
    }

    // Only start a new frame if we're still running
    if (!isRunning) {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void gui::EndRender() noexcept {
    // Don't render if we're shutting down
    if (!isRunning) {
        return;
    }

    ImGui::EndFrame();

    constexpr float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    deviceContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    deviceContext->ClearRenderTargetView(mainRenderTargetView, clear_color);
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present with VSync (1) or without VSync (0)
    swapChain->Present(vSyncEnabled ? 1 : 0, 0);
}

void gui::Render() noexcept 
{
    // Don't render if we're shutting down
    if (!isRunning)
        return;

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({WIDTH * dpiScale, HEIGHT * dpiScale});
    ImGui::Begin(
        "Matu's Tottaly Not Cheat Menu", 
        &isRunning,
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove
    );

    if (globals::clientAddress != 0 && globals::engineAddress != 0)
    {
        ImGui::Text("Client Address: %p", (void*)globals::clientAddress);
        ImGui::Text("Engine Address: %p", (void*)globals::engineAddress);

        ImGui::Text("\nPANIC MODE");
        if (ImGui::Button("PANIC!!!"))
        {
            globals::glow = false;
            globals::chams = false;
            globals::radar = false;
            globals::aimbot = false;
            globals::norecoil = false;
            globals::triggerbot = false;
            globals::bhop = false;

            // Reset glow colors
            globals::friendlyGlowColor[0] = 0.f;
            globals::friendlyGlowColor[1] = 1.f;
            globals::friendlyGlowColor[2] = 0.f;
            globals::friendlyGlowColor[3] = 1.f;

            globals::enemyGlowColor[0] = 1.f;
            globals::enemyGlowColor[1] = 0.f;
            globals::enemyGlowColor[2] = 0.f;
            globals::enemyGlowColor[3] = 1.f;

            // Reset chams colors
            globals::friendlyChamsColor[0] = 0.f;
            globals::friendlyChamsColor[1] = 1.f;
            globals::friendlyChamsColor[2] = 0.f;

            globals::enemyChamsColor[0] = 1.f;
            globals::enemyChamsColor[1] = 0.f;
            globals::enemyChamsColor[2] = 0.f;

            globals::chamsBright = 0.0f;


            //Close menu
        }

        //Visual
        ImGui::Text("\nVisual Cheats");
        //Glow
        ImGui::Text("Glow Settings");
        ImGui::Checkbox("Glow Cheat Enabled", &globals::glow);
        ImGui::ColorEdit4("Friendly Glow Color", globals::friendlyGlowColor);
        ImGui::ColorEdit4("Enemy Glow Color", globals::enemyGlowColor);

        //CHAMS
        ImGui::Text("\nChams");
        ImGui::Checkbox("Chams Cheat Enabled", &globals::chams);
        ImGui::ColorEdit3("Friendly Chams Color", globals::friendlyChamsColor);
        ImGui::ColorEdit3("Enemy Chams Color", globals::enemyChamsColor);
        ImGui::SliderFloat("Brightness", &globals::chamsBright, 0.0f, 25.0f);

        //Radar
        ImGui::Text("\nRadar");
        ImGui::Checkbox("Radar Cheat Enabled", &globals::radar);

        //Aim Chacks
        ImGui::Text("\nAim Cheats");
        //aimbot
        ImGui::Text("By holding Mouse Button 4 you can move camera (aimbot will not shoot then)");
        ImGui::Checkbox("Aimbot Enabled", &globals::aimbot);

        //No recoil
        ImGui::Checkbox("No Recoil Enabled", &globals::norecoil);

        //Trigger Bot
        ImGui::Text("\nTrigger Bot");
        ImGui::Checkbox("Trigger Bot Enabled", &globals::triggerbot);
        ImGui::Checkbox("Trigger on button (Q)", &globals::triggerbotBindEnabled);

        //Movement
        ImGui::Text("\nMovement Cheats");
        //BHop
        ImGui::Checkbox("Bunny Hop Enabled", &globals::bhop);
    }
    else
    {
        ImGui::Text("\nPlease lunch CS:GO");
        ImGui::Text("And then relunch this file");
    }

    ImGui::End();
}

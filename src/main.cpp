#include "gui.h"
#include "hacks.h"
#include "globals.h"

#include <thread>

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow) {
    //Hacks
    Memory mem{L"csgo.exe" };

    globals::clientAddress = mem.GetModuleAddress(L"client.dll");
    globals::engineAddress = mem.GetModuleAddress(L"engine.dll");

    std::thread(hacks::VisualThread, mem).detach();
    std::thread(hacks::AimThread, mem).detach();
    std::thread(hacks::MoveThread, mem).detach();

    // Configure settings before creating window
    gui::isDPIAware = true;
    gui::vSyncEnabled = true;

    // create gui
    gui::CreateHWindow(L"Matu's Tottaly Not Cheat Menu");
    gui::CreateDevice();
    gui::CreateImGui();

    while (gui::isRunning) {
        gui::BeginRender();
        gui::Render();
        gui::EndRender();

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // destroy gui
    gui::DestroyImGui();
    gui::DestroyDevice();
    gui::DestroyHWindow();

    return EXIT_SUCCESS;
}

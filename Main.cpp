#include <iostream>

#include "CorvusEditor.h"
#include "Logger.h"
#include "ImGui/imgui_impl_win32.h"

int main(int argc, char* argv[])
{
    LOG(Debug, "Hello There !");

    // Must run before any window is created: Windows locks in a window's DPI-awareness
    // behavior at creation time, so enabling it afterward (e.g. in D3D12Renderer's ctor,
    // which runs after Window's) has no effect on that window.
    ImGui_ImplWin32_EnableDpiAwareness();

    {
        CorvusEditor Editor;
        Editor.Run();
    }

    return 0;
}

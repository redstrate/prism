#include "imgui_backend.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "engine.hpp"
#include "platform.hpp"
#include "assertions.hpp"

using prism::imgui_backend;

const std::map<ImGuiKey, InputButton> imToPl = {
    {ImGuiKey_Tab, InputButton::Tab},
    {ImGuiKey_LeftArrow, InputButton::LeftArrow},
    {ImGuiKey_RightArrow, InputButton::RightArrow},
    {ImGuiKey_UpArrow, InputButton::Escape},
    {ImGuiKey_DownArrow, InputButton::Escape},
    {ImGuiKey_PageUp, InputButton::Escape},
    {ImGuiKey_PageDown, InputButton::Escape},
    {ImGuiKey_Home, InputButton::Escape},
    {ImGuiKey_End, InputButton::Escape},
    {ImGuiKey_Insert, InputButton::Escape},
    {ImGuiKey_Delete, InputButton::Escape},
    {ImGuiKey_Backspace, InputButton::Backspace},
    {ImGuiKey_Space, InputButton::Space},
    {ImGuiKey_Enter, InputButton::Enter},
    {ImGuiKey_Escape, InputButton::Escape},
    {ImGuiKey_A, InputButton::A},
    {ImGuiKey_C, InputButton::C},
    {ImGuiKey_V, InputButton::V},
    {ImGuiKey_X, InputButton::X},
    {ImGuiKey_Y, InputButton::Y},
    {ImGuiKey_Z, InputButton::Z}
};

imgui_backend::imgui_backend() {
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "";
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    io.BackendPlatformName = "Prism";
    
    if(platform::supports_feature(PlatformFeature::Windowing))
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    for (auto& [im, pl] : imToPl)
        io.KeyMap[im] = platform::get_keycode(pl);
        
    const auto theme = platform::get_theme();
    switch(theme) {
        case PlatformTheme::Light:
            ImGui::StyleColorsLight();
            break;
        case PlatformTheme::Dark:
        {
            ImGui::StyleColorsDark();
            
            auto& style = ImGui::GetStyle();
            style.FrameRounding = 3;
            style.FrameBorderSize = 0.0f;
            style.WindowBorderSize = 0.0f;
            style.WindowPadding = ImVec2(10, 10);
            style.FramePadding = ImVec2(4, 4);
            style.ItemInnerSpacing = ImVec2(5, 0);
            style.ItemSpacing = ImVec2(10, 5);
            style.ScrollbarSize = 10;
            style.GrabMinSize = 6;
            style.WindowRounding = 3.0f;
            style.ChildRounding = 3.0f;
            style.PopupRounding = 3.0f;
            style.ScrollbarRounding = 12.0f;
            style.GrabRounding = 3.0f;
            style.TabRounding = 3.0f;
            style.WindowTitleAlign = ImVec2(0.0, 0.5);
            
            ImVec4* colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_FrameBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.68f, 0.68f, 0.68f, 0.40f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.47f, 0.52f, 0.67f);
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.23f, 0.43f, 0.73f, 1.00f);
            colors[ImGuiCol_CheckMark]              = ImVec4(0.74f, 0.80f, 0.87f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.46f, 0.54f, 0.64f, 1.00f);
            colors[ImGuiCol_Button]                 = ImVec4(0.38f, 0.51f, 0.65f, 0.40f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.57f, 0.61f, 0.67f, 1.00f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.22f, 0.27f, 0.33f, 0.86f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.33f, 0.51f, 0.75f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
        }
            break;
    }
    
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = new int(0);

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    
    ImGuiPlatformMonitor monitor = {};
    
    const auto rect = platform::get_monitor_resolution();
    monitor.MainPos = rect.offset;
    monitor.MainSize = rect.extent;
    
    const auto wrect = platform::get_monitor_work_area();
    monitor.WorkPos = wrect.offset;
    monitor.WorkSize = wrect.extent;
    
    monitor.DpiScale = platform::get_monitor_dpi();

    platform_io.Monitors.push_back(monitor);
    
    platform_io.Platform_CreateWindow = [](ImGuiViewport* viewport) {
        viewport->PlatformHandle = new int(platform::open_window("", {viewport->Pos, viewport->Size}, WindowFlags::Borderless));
    };
    
    platform_io.Platform_DestroyWindow = [](ImGuiViewport* viewport) {
        platform::close_window(*(int*)viewport->PlatformHandle);
    };
    
    platform_io.Platform_ShowWindow = [](ImGuiViewport*) {};
    
    platform_io.Platform_SetWindowPos = [](ImGuiViewport* viewport, const ImVec2 pos) {
        platform::set_window_position(*(int*)viewport->PlatformHandle, pos);
    };
    
    platform_io.Platform_GetWindowPos = [](ImGuiViewport* viewport) {
        auto [x, y] = platform::get_window_position(*(int*)viewport->PlatformHandle);
        return ImVec2(x, y);
    };
    
    platform_io.Platform_SetWindowSize = [](ImGuiViewport* viewport, const ImVec2 size) {
        platform::set_window_size(*(int*)viewport->PlatformHandle, size);
    };
    
    platform_io.Platform_GetWindowSize = [](ImGuiViewport* viewport) {
        auto [x, y] = platform::get_window_size(*(int*)viewport->PlatformHandle);
        return ImVec2(x, y);
    };
    
    platform_io.Platform_SetWindowFocus = [](ImGuiViewport* viewport) {
        platform::set_window_focused(*(int*)viewport->PlatformHandle);
    };
    
    platform_io.Platform_GetWindowFocus = [](ImGuiViewport* viewport) {
        return platform::is_window_focused(*(int*)viewport->PlatformHandle);
    };
    
    platform_io.Platform_GetWindowMinimized = [](ImGuiViewport*) {
        return false;
    };
    
    platform_io.Platform_SetWindowTitle = [](ImGuiViewport* viewport,  const char* title) {
        platform::set_window_title(*(int*)viewport->PlatformHandle, title);
    };
}

void imgui_backend::begin_frame(const float delta_time) {
    ImGuiIO& io = ImGui::GetIO();
    
    const auto [width, height] = platform::get_window_size(0);
    const auto [dw, dh] = platform::get_window_drawable_size(0);

    io.DisplaySize = ImVec2(width, height);
    io.DisplayFramebufferScale = ImVec2((float)dw / width, (float)dh / height);
    io.DeltaTime = delta_time;
    io.KeyCtrl = platform::get_key_down(InputButton::Ctrl);
    io.KeyShift = platform::get_key_down(InputButton::Shift);
    io.KeyAlt = platform::get_key_down(InputButton::Alt);
    io.KeySuper = platform::get_key_down(InputButton::Super);
    io.MouseHoveredViewport = 0;
    
    const auto [sx, sy] = platform::get_wheel_delta();
    io.MouseWheel = sy;
    io.MouseWheelH = sx;

    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        const auto [x, y] = platform::get_screen_cursor_position();
        
        io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
    } else {
        const auto [x, y] = platform::get_cursor_position();
        
        io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
    }

    io.MouseDown[0] = platform::get_mouse_button_down(0);
    io.MouseDown[1] = platform::get_mouse_button_down(1);
    
    ImGui::NewFrame();
}

void imgui_backend::render(int index) {
    Expects(index >= 0);
    
    if(index == 0) {
        ImGui::EndFrame();
        ImGui::Render();
    }
}

void imgui_backend::process_key_down(unsigned int key_code) {
    Expects(key_code >= 0);
    
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(platform::translate_keycode(key_code));
    
    io.KeysDown[key_code] = true;
}

void imgui_backend::process_key_up(unsigned int key_code) {
    Expects(key_code >= 0);

    ImGuiIO& io = ImGui::GetIO();
    
    io.KeysDown[key_code] = false;
}

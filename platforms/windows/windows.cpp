#include "platform.hpp"

#include <tchar.h>
#include <windows.h>
#include <winuser.h>
#include <thread>
#include <map>
#include <winrt/Windows.UI.ViewManagement.h>

#pragma comment(lib, "windowsapp")

std::map<InputButton, int> inputToKeyCode = { {
	{InputButton::C, 67},
	{InputButton::V, 86},
	{InputButton::X, 58},
	{InputButton::Y, 59},
	{InputButton::Z, 60},
	{InputButton::Backspace, 8},
	{InputButton::Enter, 4},
	{InputButton::Space, 20},
	{InputButton::Ctrl, 17},
	{InputButton::Shift, 10},
	{InputButton::A, 65},
	{InputButton::W, 87},
	{InputButton::S, 83},
	{InputButton::D, 68},
	{InputButton::Escape, 27},
	{InputButton::Tab, 9}
}};

const char* platform::get_name() {
	return "Windows";
}

bool platform::supports_feature(const PlatformFeature feature) {
    return false;
}

std::vector<const char*> platform::get_native_surface_extension() {
    return {"VK_KHR_surface", "VK_KHR_win32_surface"};
}

bool platform::get_key_down(const InputButton key) {
	if (inputToKeyCode.count(key)) {
		return (GetKeyState(inputToKeyCode[key]) & 0x8000) != 0;
	}
}

int platform::get_keycode(const InputButton key) {
	return inputToKeyCode[key];
}

void platform::open_dialog(const bool existing, const std::function<void(std::string)> returnFunction, const bool openDirectory) {
	const auto openDialog = [returnFunction] {
		const int BUFSIZE = 1024;
		char buffer[BUFSIZE] = { 0 };
		OPENFILENAME ofns = { 0 };
		ofns.lStructSize = sizeof(ofns);
		ofns.lpstrFile = buffer;
		ofns.nMaxFile = BUFSIZE;
		ofns.lpstrTitle = "Open File";

		if (GetOpenFileName(&ofns))
			returnFunction(buffer);
	};

	std::thread* ot = new std::thread(openDialog);
}

void platform::save_dialog(const std::function<void(std::string)> returnFunction) {
	const auto saveDialog = [returnFunction] {
		const int BUFSIZE = 1024;
		char buffer[BUFSIZE] = { 0 };
		OPENFILENAME ofns = { 0 };
		ofns.lStructSize = sizeof(ofns);
		ofns.lpstrFile = buffer;
		ofns.nMaxFile = BUFSIZE;
		ofns.lpstrTitle = "Save File";

		if (GetSaveFileName(&ofns))
			returnFunction(buffer);
	};

	std::thread* ot = new std::thread(saveDialog);
}

char* platform::translate_keycode(const unsigned int keycode) {
	char* uc = new char[5];

	BYTE kb[256];
	GetKeyboardState(kb);
	ToUnicode(keycode, 0, kb, (LPWSTR)uc, 4, 0);

	return uc;
}

void platform::mute_output() {

}

void platform::unmute_output() {

}

PlatformTheme platform::get_theme() {
	using namespace winrt::Windows::UI::ViewManagement;

	// TODO: figure out if this works pre-anniversary update/other windows other than 10
	UISettings settings;
	auto background = settings.GetColorValue(UIColorType::Background);
	auto foreground = settings.GetColorValue(UIColorType::Foreground);

	if (background == winrt::Windows::UI::Colors::White())
		return PlatformTheme::Light;
	else
		return PlatformTheme::Dark;
}
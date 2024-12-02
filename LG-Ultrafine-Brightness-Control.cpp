/*
 * Copyright (C) 2024 Darezik Damkevala
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "framework.h"
#include "LG-Ultrafine-Brightness-Control.h"
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <dbt.h>
#include "hidapi.h"
#include <cstdint>
#include <initguid.h>

#define MAX_LOADSTRING 100

// GUID for HID devices
DEFINE_GUID(GUID_DEVINTERFACE_HID,
	0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);

// Constants
constexpr uint16_t VENDOR_ID = 0x43e;
constexpr uint16_t PRODUCT_ID = 0x9a40;
constexpr uint16_t MAX_BRIGHTNESS = 0xd2f0;
constexpr uint16_t MIN_BRIGHTNESS = 0x0190;

// Global Variables:
HINSTANCE           hInst;
WCHAR               windowTitle[MAX_LOADSTRING];
WCHAR               windowClassName[MAX_LOADSTRING];
hid_device*			deviceHandle = nullptr;
HDEVNOTIFY          deviceNotifyHandle = nullptr;
HBRUSH				whiteBrush = nullptr;

// Forward declarations of functions included in this code module:
ATOM                registerWindowClass(HINSTANCE hInstance);
BOOL                initializeInstance(HINSTANCE, int);
LRESULT CALLBACK    windowProcedure(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    aboutDialogProcedure(HWND, UINT, WPARAM, LPARAM);
void                setBrightness(uint16_t value);
uint16_t            getBrightness();
void                updateDeviceConnection(HWND statusLabel, HWND slider);
void                registerDeviceNotifications(HWND hWnd);
void                unregisterDeviceNotifications();



// Set the brightness of the connected device
void setBrightness(uint16_t value) {
	if (!deviceHandle) {
		OutputDebugStringW(L"Warning: Attempted to set brightness with no monitor connected!\n");
		return;
	}

	uint8_t data[7] = {
		0x00,
		static_cast<uint8_t>(value & 0x00FF),
		static_cast<uint8_t>((value >> 8) & 0x00FF),
		0x00, 0x00, 0x00, 0x00
	};

	if (hid_send_feature_report(deviceHandle, data, sizeof(data)) < 0) {
		OutputDebugStringW(L"Error: Unable to set brightness!\n");
	}
}

// Get the current brightness of the connected device
uint16_t getBrightness() {
	if (!deviceHandle) {
		return MIN_BRIGHTNESS;
	}

	uint8_t data[7] = { 0 };
	if (hid_get_feature_report(deviceHandle, data, sizeof(data)) < 0) {
		OutputDebugStringW(L"Error: Unable to get brightness!\n");
		return MIN_BRIGHTNESS;
	}

	return data[1] + (data[2] << 8);
}

// Update the device connection and UI elements
void updateDeviceConnection(HWND statusLabel, HWND slider) {
	if (deviceHandle) {
		hid_close(deviceHandle);
		deviceHandle = nullptr;
	}

	hid_device_info* devices = hid_enumerate(0x0, 0x0);
	hid_device_info* currentDevice = devices;
	const char* monitorPath = nullptr;

	while (currentDevice) {
		if (currentDevice->vendor_id == VENDOR_ID &&
			wcsstr(currentDevice->product_string, L"BRIGHTNESS")) {
			monitorPath = currentDevice->path;
			break;
		}
		currentDevice = currentDevice->next;
	}

	if (monitorPath) {
		deviceHandle = hid_open_path(monitorPath);
		if (deviceHandle) {
			OutputDebugStringW(L"Info: Monitor connected.\n");
			SetWindowTextW(statusLabel, L"Monitor connected.");

			EnableWindow(slider, TRUE);
			uint16_t currentBrightness = getBrightness();
			int sliderPos = static_cast<int>((currentBrightness - MIN_BRIGHTNESS) * 100.0 /
				(MAX_BRIGHTNESS - MIN_BRIGHTNESS));
			sliderPos = max(0, min(sliderPos, 100));
			SendMessageW(slider, TBM_SETPOS, TRUE, sliderPos);

			wchar_t buffer[50];
			swprintf_s(buffer, L"Current Brightness: %d%%", sliderPos);
			SetWindowTextW(statusLabel, buffer);
		}
		else {
			OutputDebugStringW(L"Error: Unable to open HID device!\n");
			SetWindowTextW(statusLabel, L"Unable to connect to monitor.");
			EnableWindow(slider, FALSE);
		}
	}
	else {
		OutputDebugStringW(L"Warning: Monitor not found!\n");
		SetWindowTextW(statusLabel, L"Monitor disconnected.");
		EnableWindow(slider, FALSE);
	}

	hid_free_enumeration(devices);
}

// Register for device notifications
void registerDeviceNotifications(HWND hWnd) {
	DEV_BROADCAST_DEVICEINTERFACE notificationFilter = {};
	notificationFilter.dbcc_size = sizeof(notificationFilter);
	notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	notificationFilter.dbcc_classguid = GUID_DEVINTERFACE_HID;

	deviceNotifyHandle = RegisterDeviceNotification(hWnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (deviceNotifyHandle) {
		OutputDebugStringW(L"Info: Registered for device notifications.\n");
	}
	else {
		OutputDebugStringW(L"Error: Failed to register for device notifications!\n");
	}
}

// Unregister device notifications
void unregisterDeviceNotifications() {
	if (deviceNotifyHandle) {
		UnregisterDeviceNotification(deviceNotifyHandle);
		deviceNotifyHandle = nullptr;
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, windowTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_LGULTRAFINEBRIGHTNESSCONTROL, windowClassName, MAX_LOADSTRING);
	registerWindowClass(hInstance);

	// Perform application initialization:
	if (!initializeInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LGULTRAFINEBRIGHTNESSCONTROL));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: registerWindowClass()
//
//  PURPOSE: Registers the window class.
//
ATOM registerWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProcedure;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LGULTRAFINEBRIGHTNESSCONTROL));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LGULTRAFINEBRIGHTNESSCONTROL);
	wcex.lpszClassName = windowClassName;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: initializeInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL initializeInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(windowClassName, windowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 400, 150, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: windowProcedure(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK windowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND slider;
	static HWND statusLabel;
	switch (message)
	{
	case WM_COMMAND:
	{
		int menuCommandId = LOWORD(wParam);
		// Parse the menu selections:
		switch (menuCommandId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, aboutDialogProcedure);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_CREATE: {
		whiteBrush = CreateSolidBrush(RGB(255, 255, 255));

		HFONT font = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		statusLabel = CreateWindowW(L"STATIC", L"Monitoring monitor connection...",
			WS_VISIBLE | WS_CHILD, 10, 10, 380, 20, hWnd, nullptr, nullptr, nullptr);

		slider = CreateWindowW(TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_HORZ,
			10, 40, 360, 40, hWnd, (HMENU)101, nullptr, nullptr);
		SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
		EnableWindow(slider, FALSE);

		SendMessageW(statusLabel, WM_SETFONT, (WPARAM)font, TRUE);
		SendMessageW(slider, WM_SETFONT, (WPARAM)font, TRUE);

		updateDeviceConnection(statusLabel, slider);
		registerDeviceNotifications(hWnd);

		// Set the focus to the slider on startup so the window can be controlled via the keyboard on start
		SetFocus(slider);

		break;
	}
	case WM_ACTIVATE: {
		if (wParam != WA_INACTIVE) { // The window is becoming active
			if (slider) {
				SetFocus(slider); // Refocus the slider
			}
		}
		break;
	}

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN: {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0, 0, 0));
		return (LRESULT)whiteBrush;
	}

	case WM_DEVICECHANGE: {
		if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			if (pHdr && pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				updateDeviceConnection(statusLabel, slider);
			}
		}
		break;
	}

	case WM_HSCROLL: {
		if ((HWND)lParam == slider) {
			int pos = (int)SendMessageW(slider, TBM_GETPOS, 0, 0);
			uint16_t brightnessValue = static_cast<uint16_t>((pos / 100.0) *
				(MAX_BRIGHTNESS - MIN_BRIGHTNESS) + MIN_BRIGHTNESS);
			setBrightness(brightnessValue);

			wchar_t buffer[50];
			swprintf_s(buffer, L"Current Brightness: %d%%", pos);
			SetWindowTextW(statusLabel, buffer);
		}
		break;
	}

	case WM_DESTROY: {
		unregisterDeviceNotifications();
		if (whiteBrush) {
			DeleteObject(whiteBrush);
			whiteBrush = nullptr;
		}
		if (deviceHandle) {
			hid_close(deviceHandle);
			deviceHandle = nullptr;
		}
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK aboutDialogProcedure(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

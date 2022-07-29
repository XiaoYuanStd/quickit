#include <windows.h>
#include <windowsx.h>
#include <string>
#include <fstream>
using namespace std;
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

struct qkItem
{
	HICON hIcon;
	string fileName;
	bool isHover = false;
	bool shouldRemove = false;
	int extInfo = 0;
};

qkItem qkItems[100];

const int W = 500;
const int H = 300;
const int ICON_SIZE = 50;

char qkPath[MAX_PATH] = { 0 };

bool uiHidden = false;

HICON getFileIcon(LPCSTR fileName) {
	SHFILEINFO info;
	SHGetFileInfo(fileName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_LARGEICON);
	return info.hIcon;
}

bool mouseNotOnWindow(HWND hwnd) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	POINT p;
	GetCursorPos(&p);
	if (rect.left > p.x || rect.top > p.y) {
		return true;
	}
	if (rect.right < p.x || rect.bottom < p.y) {
		return true;
	}
	return false;
}

void writeItemList() {
	ofstream out;
	out.open((string(qkPath) + ".itemlist.txt").c_str(), ios::trunc);
	for (int i = 0; i < sizeof(qkItems) / sizeof(qkItem); i++) {
		if (!qkItems[i].fileName.empty() && qkItems[i].extInfo == 0 && qkItems[i].shouldRemove == false)
			out << qkItems[i].fileName << endl;
	}
	out.close();
}

void loop(LPVOID p) {
	HWND hwnd = (HWND)p;
	for (;;) {
		if (KEY_DOWN(VK_LBUTTON) && mouseNotOnWindow(hwnd)) {
			uiHidden = true;
		}

		if (KEY_DOWN(0x51) && KEY_DOWN(0x4B)) {
			uiHidden = false;
		}

		if (uiHidden) {
			ShowWindow(hwnd, SW_HIDE);
		}
		else {
			ShowWindow(hwnd, SW_SHOW);
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		Sleep(50);
	}
}

void render(HWND hwnd) {
	HDC hdc = GetDC(hwnd);
	HDC hcdc = CreateCompatibleDC(hdc);
	RECT rect;
	GetWindowRect(hwnd, &rect);
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	HDC hdcTemp = CreateCompatibleDC(hdc);
	HBITMAP hbmTemp = CreateCompatibleBitmap(hdc, w, h);
	SelectObject(hdcTemp, hbmTemp);
	HBITMAP hbm = (HBITMAP)LoadImage(NULL, (string(qkPath) + ".bg.bmp").c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	SelectObject(hcdc, hbm);
	BitBlt(hdcTemp, 0, 0, w, h, hcdc, 0, 0, SRCCOPY);
	for (int i = 0; i <= H; i += ICON_SIZE) {
		for (int j = 0; j <= W; j += ICON_SIZE) {
			int index = (i / ICON_SIZE) * (W / ICON_SIZE) + (j / ICON_SIZE);
			if (qkItems[index].hIcon != NULL) {
				if (qkItems[index].isHover) {
					HBRUSH hb = CreateSolidBrush(RGB(22, 36, 44));
					if (KEY_DOWN(VK_LBUTTON)) {
						hb = CreateSolidBrush(RGB(13, 21, 26));
					}
					SelectBrush(hdcTemp, hb);
					RoundRect(hdcTemp, j, i, j + 50, i + 50, 10, 10);
					
				}
				DrawIconEx(hdcTemp, j + 9, i + 9, qkItems[index].hIcon, 0, 0, 0, 0, DI_NORMAL);
			}
		}
	}
	BitBlt(hdc, 0, 0, w, h, hdcTemp, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hdc);
	DeleteDC(hcdc);
	DeleteDC(hdcTemp);
	DeleteObject(hbmTemp);
	DeleteObject(hbm);
}

void handleClick(HWND hwnd, int x, int y) {
	int index = (y / ICON_SIZE) * (W / ICON_SIZE) + (x / ICON_SIZE);
	if (qkItems[index].extInfo == 0 && !qkItems[index].fileName.empty()) {
		ShellExecute(NULL, "open", qkItems[index].fileName.c_str(), NULL, NULL, SW_SHOWDEFAULT);
	}
	if (qkItems[index].extInfo == 1) {
		ExitProcess(0);
	}
	if (qkItems[index].extInfo == 2) {
		OPENFILENAME opfn;
		char fileName[MAX_PATH];
		ZeroMemory(&opfn, sizeof(OPENFILENAME));
		opfn.lStructSize = sizeof(OPENFILENAME);
		opfn.lpstrFilter = "Any File\0*.*\0";
		opfn.nFilterIndex = 1;
		opfn.lpstrFile = fileName;
		opfn.lpstrFile[0] = '\0';
		opfn.nMaxFile = sizeof(fileName);
		opfn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		if (GetOpenFileName(&opfn) && fileName != NULL && index < W * H / ICON_SIZE / ICON_SIZE - 1)
		{
			qkItem item;
			item.fileName = fileName;
			item.hIcon = getFileIcon(item.fileName.c_str());
			qkItems[index + 1] = item;
			writeItemList();
			ShellExecute(NULL, "open", qkPath, NULL, NULL, SW_SHOWDEFAULT);
			ExitProcess(0);
		}
	}
}

void handleRightClick(HWND hwnd, int x, int y) {
	int index = (y / ICON_SIZE) * (W / ICON_SIZE) + (x / ICON_SIZE);
	if (qkItems[index].extInfo == 0 && !qkItems[index].fileName.empty()) {
		qkItems[index].shouldRemove = true;
		writeItemList();
		ShellExecute(NULL, "open", qkPath, NULL, NULL, SW_SHOWDEFAULT);
		ExitProcess(0);
	}
}

void handleHover(HWND hwnd, int x, int y) {
	int index = (y / ICON_SIZE) * (W / ICON_SIZE) + (x / ICON_SIZE);
	for (int i = 0; i < 100; i++) {
		qkItems[i].isHover = false;
	}
	if (index > 99 || index < 0) {
		return;
	}
	qkItems[index].isHover = true;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int w, h;
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
)
{
	GetModuleFileName(hInstance, qkPath, MAX_PATH);
	ifstream in((string(qkPath) + ".itemlist.txt").c_str());
	char buf[MAX_PATH] = { 0 };
	int index = W / ICON_SIZE;
	while (in.getline(buf, MAX_PATH, '\n')) {
		qkItem item;
		item.fileName = buf;
		item.hIcon = getFileIcon(item.fileName.c_str());
		qkItems[index] = item;
		index++;
	}
	in.close();
	qkItems[W / ICON_SIZE - 1].extInfo = 1;
	HICON hIcon = (HICON)LoadImage(NULL, (string(qkPath) + ".close.ico").c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	qkItems[W / ICON_SIZE - 1].hIcon = hIcon;
	qkItems[index].extInfo = 2;
	hIcon = (HICON)LoadImage(NULL, (string(qkPath) + ".add.ico").c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	qkItems[index].hIcon = hIcon;

	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = WHITE_BRUSH;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "qkmain";

	RegisterClass(&wndclass);
	hwnd = CreateWindow("qkmain",
		"quickit",
		WS_POPUP,
		(w - W) / 2,
		(h - H) / 2,
		W,
		H,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hwnd, nShowCmd);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)loop, hwnd, 0, NULL);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (!uiHidden) {
			render(hwnd);
		}
		Sleep(50);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y = 0;
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
	case WM_LBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		handleClick(hwnd, x, y);
		return 0;
	case WM_RBUTTONUP:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		handleRightClick(hwnd, x, y);
		return 0;
	case WM_MOUSEHOVER:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		handleHover(hwnd, x, y);
	case WM_MOUSEMOVE:
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		handleHover(hwnd, x, y);
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
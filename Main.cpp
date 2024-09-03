/*
 *          Copyright 2020, Vitali Baumtrok.
 * Distributed under the Boost Software License, Version 1.0.
 *     (See accompanying file LICENSE or copy at
 *        http://www.boost.org/LICENSE_1_0.txt)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#include <assert.h>
#include <iostream>

#define ERR_NONE 0
#define ERR_REGCLS 1
#define ERR_CRWIN_AWR 2
#define ERR_CRWIN 3
#define ERR_DC 4
#define ERR_CPF 5
#define ERR_SPF 6
#define ERR_RC 7

struct {
	int code;
	LPCWSTR message;
} err = { ERR_NONE, nullptr };

struct {
	LPCWSTR className;
	LPCSTR classNameChar;
	LPCWSTR title;
	HWND hndl;
	HDC deviceContext;
	HGLRC renderContext;
	int prevX, prevY, width, height, prevWidth, prevHeight, resX, resY;
	bool fullscreen;
} window = { L"OpenGL", "OpenGL", L"OpenGL Example", nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 640, 480, false };

#define STROKE_TYPE_NONE 0
#define STROKE_TYPE_LBUTTON 1
#define STROKE_TYPE_POINTER 2

static int strokeType = STROKE_TYPE_NONE;
static POINTS stampPoint;
static double width;
static double height;
static float stampThickness = 10.0f;

static void
setupState()
{
	stampPoint.x = window.width*0.5;
	stampPoint.y = window.height*0.5;
}

static void
drawLines()
{ 
	float pts[8];
	pts[0] = stampPoint.x - stampThickness;
	pts[1] = stampPoint.y - stampThickness;

	pts[2] = stampPoint.x + stampThickness;
	pts[3] = stampPoint.y - stampThickness;
	
	pts[4] = stampPoint.x + stampThickness;
	pts[5] = stampPoint.y + stampThickness;

	pts[6] = stampPoint.x - stampThickness;
	pts[7] = stampPoint.y + stampThickness;

	glLineWidth(3.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);

	if ( strokeType == STROKE_TYPE_LBUTTON )
	{
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (strokeType == STROKE_TYPE_POINTER)
	{
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	}

	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void
draw()
{
	glViewport(0, 0, window.width, window.height );
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	// setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,              // left
		    window.width,   // right 
		    window.height,  // bottom
		    0,              //top
		    -1,             //near
		     1);            // far

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	drawLines();
}

static void
setFullscreen(bool fullscreen)
{
	DWORD style = GetWindowLong(window.hndl, GWL_STYLE);
	if (fullscreen)
	{
		RECT rect;
		MONITORINFO mi = { sizeof(mi) };
		GetWindowRect(window.hndl, &rect);
		window.prevX = rect.left;
		window.prevY = rect.top;
		window.prevWidth = rect.right - rect.left;
		window.prevHeight = rect.bottom - rect.top;

		GetMonitorInfo(MonitorFromWindow(window.hndl, MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowLong(window.hndl, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(window.hndl, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
	else
	{
		MONITORINFO mi = { sizeof(mi) };
		UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
		GetMonitorInfo(MonitorFromWindow(window.hndl, MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowLong(window.hndl, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPos(window.hndl, HWND_NOTOPMOST, window.prevX, window.prevY, window.prevWidth, window.prevHeight, flags);
	}
}

static RECT
getStampRect()
{
	RECT rect;
	rect.left = stampPoint.x - stampThickness;
	rect.right = stampPoint.x + stampThickness;
	rect.top = stampPoint.y - stampThickness;
	rect.bottom = stampPoint.y + stampThickness;
	return rect;
}

static void
damageRect(HWND hWnd, RECT rect )
{
	::InvalidateRect(hWnd, &rect, false);
	::UpdateWindow(hWnd);
}

static void
updateStampPoint(HWND hWnd, POINTS point)
{
	stampPoint.x = point.x;
	stampPoint.y = point.y;
	RECT rect = getStampRect();
	damageRect(hWnd, rect);
}

static bool
proccessPointer(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL result = false;

	// only handle pen pointer
	UINT pointerID = GET_POINTERID_WPARAM(wParam);
	POINTER_INPUT_TYPE pointerType = PT_POINTER;
	result = GetPointerType(pointerID, &pointerType);
	if (!result)
		return false;

	if (PT_PEN != pointerType) 
		return false;

	// get pen info
	POINTER_PEN_INFO penInfo;
	result = GetPointerPenInfo(pointerID, &penInfo); assert(result);
	if (!result) 
		return false;

	float x, y;
	x = (float)penInfo.pointerInfo.ptPixelLocation.x;
	y = (float)penInfo.pointerInfo.ptPixelLocation.y;

	POINTS point;
	point.x = x;
	point.y = y;
	updateStampPoint(hWnd, point);
}

static LRESULT CALLBACK
wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
		case WM_PAINT:
		{

			OutputDebugString(L"HELLO WORLD\n");

			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);

			// get the damage region
			RECT rcPaint;
			if (!::IsRectEmpty(&ps.rcPaint)) {
				::CopyRect(&rcPaint, &ps.rcPaint);
			}
			else {
				::GetClientRect(hWnd, &rcPaint);
			}

			draw();
			::SwapBuffers(window.deviceContext);
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_SIZE:
		{
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);
			window.width = w;
			window.height = h;
			break;
		}
		case WM_KEYDOWN:
		{
			/* ESC */
			if (wParam == 27)
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			/* f */
			else if (wParam == 70)
				setFullscreen(window.fullscreen = !window.fullscreen);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			strokeType = 1;

			POINTS points = MAKEPOINTS(lParam);
			updateStampPoint(hWnd, points);
			break;
		}
		case WM_LBUTTONUP:
		{
			POINTS points = MAKEPOINTS(lParam);
			updateStampPoint(hWnd, points);
			strokeType = STROKE_TYPE_NONE;
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (strokeType == 1)
			{
				POINTS points = MAKEPOINTS(lParam);
				updateStampPoint(hWnd, points);
			}
			break;
		}
		case WM_POINTERDOWN:
		{
			strokeType = 2;
			proccessPointer(hWnd, message, wParam, lParam);
			break;
		}
		case WM_POINTERUP:
		{
			proccessPointer(hWnd, message, wParam, lParam);
			strokeType = STROKE_TYPE_NONE;
			break;
		}
		case WM_POINTERUPDATE:
		{
			if (strokeType == 2)
			{
				proccessPointer(hWnd, message, wParam, lParam);

			}
			break;
		}
		case WM_CLOSE:
		{
			wglMakeCurrent(window.deviceContext, NULL);
			wglDeleteContext(window.renderContext);
			ReleaseDC(hWnd, window.deviceContext);
			DestroyWindow(hWnd);
			/* stop event queue thread */
			PostQuitMessage(0);
			break;
		}
		default:
		{
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return result;
}

static void
registerClass(HINSTANCE instance)
{
	WNDCLASSEXW wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = (WNDPROC)wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = window.className;
	wcex.hIconSm = NULL;

	if (!RegisterClassExW(&wcex))
	{
		err.code = ERR_REGCLS;
		err.message = L"RegisterClassExW failed: Can not register window class.";
	}
}

static void
createWindow(HINSTANCE instance)
{
	if (err.code == ERR_NONE)
	{
		DWORD  style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		RECT rect = { 0, 0, window.resX, window.resY };

		if (AdjustWindowRect(&rect, style, false))
		{
			/* compute window size including border */
			window.width = rect.right - rect.left;
			window.height = rect.bottom - rect.top;

			window.hndl = CreateWindowW(window.className, window.title, style, 0, 0, window.width, window.height, nullptr, nullptr, instance, nullptr);
			if (!window.hndl)
			{
				err.code = ERR_CRWIN;
				err.message = L"CreateWindowW failed: Can not create window.";
			}
		}
		else
		{
			err.code = ERR_CRWIN_AWR;
			err.message = L"AdjustWindowRect failed: Can not create window.";
		}
	}
}

static void
centerWindow()
{
	if (err.code == ERR_NONE)
	{
		RECT rect;
		MONITORINFO mi = { sizeof(mi) };

		GetMonitorInfo(MonitorFromWindow(window.hndl, MONITOR_DEFAULTTONEAREST), &mi);
		int x = (mi.rcMonitor.right - mi.rcMonitor.left - window.width) / 2;
		int y = (mi.rcMonitor.bottom - mi.rcMonitor.top - window.height) / 2;

		SetWindowPos(window.hndl, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}

static void
createContext()
{
	if (err.code == ERR_NONE)
	{
		window.deviceContext = GetDC(window.hndl);
		if (window.deviceContext)
		{
			int pixelFormat;
			PIXELFORMATDESCRIPTOR pixelFormatDesc;

			/* initialize bits to 0 */
			memset(&pixelFormatDesc, 0, sizeof(PIXELFORMATDESCRIPTOR));
			pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pixelFormatDesc.nVersion = 1;
			pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
			pixelFormatDesc.cColorBits = 32;
			pixelFormatDesc.cAlphaBits = 8;
			pixelFormatDesc.cDepthBits = 24;

			pixelFormat = ChoosePixelFormat(window.deviceContext, &pixelFormatDesc);
			if (pixelFormat)
			{
				if (SetPixelFormat(window.deviceContext, pixelFormat, &pixelFormatDesc))
				{
					window.renderContext = wglCreateContext(window.deviceContext);
					if (!window.renderContext)
					{
						err.code = ERR_RC;
						err.message = L"wglCreateContext failed: Can not create render context.";
					}
				}
				else
				{
					err.code = ERR_SPF;
					err.message = L"SetPixelFormat failed: Can not create render context.";
				}
			}
			else
			{
				err.code = ERR_CPF;
				err.message = L"ChoosePixelFormat failed: Can not create render context.";
			}
		}
		else
		{
			err.code = ERR_DC;
			err.message = L"GetDC failed: Can not create device context.";
		}
	}
}

int APIENTRY
wWinMain(_In_     HINSTANCE hInstance,
         _In_opt_ HINSTANCE hPrevInstance,
         _In_     LPWSTR    lpCmdLine,
         _In_     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	registerClass(hInstance);
	createWindow(hInstance);
	createContext();
	setupState();

	if (err.code == ERR_NONE)
	{
		wglMakeCurrent(window.deviceContext, window.renderContext);
		ShowWindow(window.hndl, nCmdShow);
		centerWindow();
		UpdateWindow(window.hndl);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		wchar_t *title = new wchar_t[10];
		swprintf_s(title, 10, L"Error %d", err.code);
		MessageBox(NULL, err.message, title, MB_OK);
	}

	return err.code;
}
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#include <assert.h>
#include <iostream>

struct {
	LPCWSTR className;
	LPCSTR classNameChar;
	LPCWSTR title;
	HWND hndl;
	HDC deviceContext;
	HGLRC renderContext;

	// variables used switch back and forth between fullscreen
	int prevX, prevY, width, height, prevWidth, prevHeight, resX, resY;
	bool fullscreen;
} window = { L"OpenGLEvents", "OpenGLEvents", L"OpenGL Events Example", nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 640, 480, false };


enum StrokType
{
	NONE,
	LBUTTON,
	POINTER
};

static StrokType _strokeType = StrokType::NONE;
static POINTS _currentStampPoint;
static double _width;
static double _height;
static float STAMP_THICKNESS = 10.0f;
static HMENU _hMenu = NULL;

//
// Control over the UpdateWindow logic
//
#define ID_STAMP_DAMAGE 9001
static bool _updateWindowPerEvent = true;
static const char* DAMAGE_AFTER_EVENT = "&Update After Event";
static const char* DAMAGE_AFTER_EVENT_HISTORY = "&Update After Event History ";

static const char*
getUpdateMenuText()
{
	return _updateWindowPerEvent ? DAMAGE_AFTER_EVENT : DAMAGE_AFTER_EVENT_HISTORY;
}

//
// Control over how we process pointer history events
//
#define ID_HISTORY_ALGORITHM 9002
enum HistoryEventAlgorithm
{
	REDUCE,
	SKIP,
	PROCESS_ALL
};

static HistoryEventAlgorithm _historyEventAlgorithm = HistoryEventAlgorithm::REDUCE;
static const char* REDUCE_HISTORY_EVENTS = "&Reduce History Events";
static const char* SKIP_HISTORY_EVENTS = "&Skip History Events";
static const char* PROCESS_ALL_HISTORY_EVENTS = "&Process All History Events"; 

static const char*
getHistoryAlgorithmMenuText()
{
	switch (_historyEventAlgorithm)
	{
	case HistoryEventAlgorithm::REDUCE:
		return REDUCE_HISTORY_EVENTS;
		break;
	case HistoryEventAlgorithm::SKIP:
		return SKIP_HISTORY_EVENTS;
		break;
	case HistoryEventAlgorithm::PROCESS_ALL:
		return PROCESS_ALL_HISTORY_EVENTS;
		break;
	};
}

static void cycleHistoryAlgorithm()
{
	switch (_historyEventAlgorithm)
	{
	case HistoryEventAlgorithm::REDUCE:
		_historyEventAlgorithm = HistoryEventAlgorithm::SKIP;
		break;
	case HistoryEventAlgorithm::SKIP:
		_historyEventAlgorithm = HistoryEventAlgorithm::PROCESS_ALL;
		break;
	case HistoryEventAlgorithm::PROCESS_ALL:
		_historyEventAlgorithm = HistoryEventAlgorithm::REDUCE;
		break;
	}
}

static void
initializeState()
{
	_currentStampPoint.x = window.width*0.5;
	_currentStampPoint.y = window.height*0.5;
}

static void
getBoxColor(float* color)
{
	if (_strokeType == StrokType::LBUTTON)
	{
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 1.0f;
	}
	else if (_strokeType == StrokType::POINTER)
	{
		color[0] = 0.0f;
		color[1] = 1.0f;
		color[2] = 0.0f;
		color[3] = 1.0f;
	}
	else
	{
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 1.0f;
	}
}

static void
drawBox( float x, float y, float size, float* color )
{ 
	float pts[8];

	// bottom left
	pts[0] = x - size;
	pts[1] = y - size;

	// top left
	pts[2] = x - size;
	pts[3] = y + size;

	// top right
	pts[4] = x + size;
	pts[5] = y + size;

	// bottom right
	pts[6] = x + size;
	pts[7] = y - size;

	GLubyte indices[] = { 0, 1, 2,
						  0, 2, 3 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, pts);
	glColor4fv(color);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
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

	float color[4];
	getBoxColor(color);
	drawBox(_currentStampPoint.x, _currentStampPoint.y, STAMP_THICKNESS, color);
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
	rect.left = _currentStampPoint.x - STAMP_THICKNESS;
	rect.right = _currentStampPoint.x + STAMP_THICKNESS;
	rect.top = _currentStampPoint.y - STAMP_THICKNESS;
	rect.bottom = _currentStampPoint.y + STAMP_THICKNESS;
	return rect;
}

static void
updateStampPoint(POINTS point)
{
	_currentStampPoint.x = point.x;
	_currentStampPoint.y = point.y;
}

static void
invalidateStampRect(HWND hWnd)
{
	RECT rect = getStampRect();
	::InvalidateRect(hWnd, &rect, false);
}

static void
invalidateWindow(HWND hWnd)
{
	::InvalidateRect(hWnd, NULL, false);
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
	result = GetPointerPenInfo(pointerID, &penInfo);
	if (!result) 
		return false;

	UINT entitiesCount = 0;
	UINT pointersCount = 0;
	GetPointerFramePenInfoHistory(pointerID, &entitiesCount, &pointersCount, NULL);
	if (entitiesCount > 0 && pointersCount > 0)
	{
		static const UINT PenInfoHistoryBufferSize = 128;
		static POINTER_PEN_INFO PenInfoHistoryBuffer[PenInfoHistoryBufferSize];

		POINTER_PEN_INFO* penInfoHistory = NULL;
		if (entitiesCount * pointersCount > PenInfoHistoryBufferSize)
		{
			penInfoHistory = new POINTER_PEN_INFO[entitiesCount * pointersCount];
		}
		else
		{
			penInfoHistory = PenInfoHistoryBuffer;
		}

		GetPointerFramePenInfoHistory(pointerID, &entitiesCount, &pointersCount, penInfoHistory);

		// Reduce the events processed by skipping events based on the number of events
		int frameSkip = 1;
		if (_historyEventAlgorithm == HistoryEventAlgorithm::REDUCE)
		{
			frameSkip = (entitiesCount > 64) ? 16 :
				(entitiesCount > 32) ? 8 :
				(entitiesCount > 16) ? 4 :
				(entitiesCount > 8) ? 2 :
				1;
		}

		// We are potentially doing 3 things:
		// - Reducing the number events by skipping some
		// - Processing all of the events
		// - Just processing the first event (skip)
		for (int entity = entitiesCount - 1; entity >= 0; entity -= frameSkip)
		{
			POINTER_PEN_INFO* framePenInfo = &(penInfoHistory[entity]);
			for (UINT pointer = 0; pointer < pointersCount; ++pointer)
			{
				const POINTER_PEN_INFO& pi = framePenInfo[pointer];

				if (pi.pointerInfo.pointerId == pointerID)
				{
					float x, y;
					x = (float)pi.pointerInfo.ptPixelLocation.x;
					y = (float)pi.pointerInfo.ptPixelLocation.y;

					// convert from screen to window
					POINT screenPoint;
					screenPoint.x = x;
					screenPoint.y = y;
					::ScreenToClient(hWnd, &screenPoint);

					POINTS point;
					point.x = screenPoint.x;
					point.y = screenPoint.y;

					updateStampPoint(point);
					invalidateStampRect(hWnd);
					if (_updateWindowPerEvent)
					{
						::UpdateWindow(hWnd);
					}

					if (_historyEventAlgorithm == HistoryEventAlgorithm::SKIP)
						break;
				}
			}
		}

		if (entitiesCount * pointersCount > PenInfoHistoryBufferSize)
		{
			delete[]penInfoHistory;
		}

		SkipPointerFrameMessages(pointerID);
	}

	if (!_updateWindowPerEvent)
	{
		::UpdateWindow(hWnd);
	}

	return true;
}

static LRESULT CALLBACK
wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{
		case WM_CREATE:
		{
			_hMenu = ::CreateMenu();
			HMENU hSubMenu = ::CreatePopupMenu();
			::AppendMenuA(hSubMenu, MF_STRING, ID_STAMP_DAMAGE, getUpdateMenuText());
			::AppendMenuA(hSubMenu, MF_STRING, ID_HISTORY_ALGORITHM, getHistoryAlgorithmMenuText());
			::AppendMenuA(_hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Current State");
			::SetMenu(hWnd, _hMenu);
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
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
			_strokeType = StrokType::LBUTTON;

			POINTS points = MAKEPOINTS(lParam);
			updateStampPoint(points);
			invalidateStampRect(hWnd);
			::UpdateWindow(hWnd);
			break;
		}
		case WM_LBUTTONUP:
		{
			POINTS points = MAKEPOINTS(lParam);
			updateStampPoint(points);
			invalidateStampRect(hWnd);
			::UpdateWindow(hWnd);
			_strokeType = StrokType::NONE;
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (_strokeType == StrokType::LBUTTON)
			{
				POINTS points = MAKEPOINTS(lParam);
				updateStampPoint(points);
				invalidateStampRect(hWnd);
				::UpdateWindow(hWnd);
			}
			break;
		}
		case WM_POINTERDOWN:
		{
			_strokeType = StrokType::POINTER;
			proccessPointer(hWnd, message, wParam, lParam);
			break;
		}
		case WM_POINTERUP:
		{
			proccessPointer(hWnd, message, wParam, lParam);
			_strokeType = StrokType::NONE;
			break;
		}
		case WM_POINTERUPDATE:
		{
			if (_strokeType == StrokType::POINTER)
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
			PostQuitMessage(0);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case ID_STAMP_DAMAGE:
				{
					_updateWindowPerEvent = !_updateWindowPerEvent;
					ModifyMenuA(_hMenu, ID_STAMP_DAMAGE, MF_BYCOMMAND, ID_STAMP_DAMAGE, getUpdateMenuText());
					break;
				}
				case ID_HISTORY_ALGORITHM:
				{
					cycleHistoryAlgorithm();
					ModifyMenuA(_hMenu, ID_HISTORY_ALGORITHM, MF_BYCOMMAND, ID_HISTORY_ALGORITHM, getHistoryAlgorithmMenuText());
					break;
				}
			}
		}
		default:
		{
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return result;
}

static bool
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
		OutputDebugStringW(L"RegisterClassExW failed");
		return false;
	}

	return true;
}

static bool
createWindow(HINSTANCE instance)
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
			OutputDebugStringW(L"CreateWindowW failed: Can not create window.");
			return false;
		}
	}
	else
	{
		OutputDebugStringW(L"AdjustWindowRect failed: Can not create window.");
		return false;
	}

	return true;
}

static void
centerWindow()
{
	RECT rect;
	MONITORINFO mi = { sizeof(mi) };

	GetMonitorInfo(MonitorFromWindow(window.hndl, MONITOR_DEFAULTTONEAREST), &mi);
	int x = (mi.rcMonitor.right - mi.rcMonitor.left - window.width) / 2;
	int y = (mi.rcMonitor.bottom - mi.rcMonitor.top - window.height) / 2;

	SetWindowPos(window.hndl, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
}

static bool
createContext()
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
					OutputDebugStringW(L"wglCreateContext failed: Can not create render context.");
					return false;
				}
			}
			else
			{
				OutputDebugStringW(L"SetPixelFormat failed: Can not create render context.");
				return false;
			}
		}
		else
		{
			OutputDebugStringW(L"ChoosePixelFormat failed: Can not create render context.");
			return false;
		}
	}
	else
	{
		OutputDebugStringW(L"GetDC failed: Can not create device context.");
		return false;
	}

	return true;
}

int APIENTRY
wWinMain(_In_     HINSTANCE hInstance,
         _In_opt_ HINSTANCE hPrevInstance,
         _In_     LPWSTR    lpCmdLine,
         _In_     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!registerClass(hInstance))
		return 1;

	if (!createWindow(hInstance))
		return 1;

	if (!createContext())
		return 1;

	initializeState();

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

	return 0;
}
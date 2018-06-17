#include <windows.h>

// TODO: global for now
static bool running;

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static HBITMAP BitmapHandle;
static HDC BitmapDeviceContext;

void Win32ResizeDIBSection(int width, int height)
{
	// TODO: Bulletproof this
	// May be don't free first, free after, then free first if that fails

	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	if (!BitmapDeviceContext)
	{
		// TODO: Should we re-create under special circumstances
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = width;
	BitmapInfo.bmiHeader.biHeight = height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

void Win32UpdateWindow(HDC hdc, int x, int y, int width, int height)
{
	StretchDIBits(hdc, x, y, width, height, x, y, width, height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch(uMsg)
	{
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(hwnd, &ClientRect);
			int height = ClientRect.bottom - ClientRect.top;
			int width = ClientRect.right - ClientRect.left;
			Win32ResizeDIBSection(width, height);
			OutputDebugStringA("WM_SIZE\n");
			break;
		}
		case WM_CLOSE:
		{
			// TODO: Handle this with a message to the user
			running = false;
			OutputDebugStringA("WM_CLOSE\n");
			break;
		}
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
			break;
		}
		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			running = false;
			OutputDebugStringA("WM_DESTROY\n");
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC DeviceContext = BeginPaint(hwnd, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			Win32UpdateWindow(DeviceContext, x, y, width, height);
			EndPaint(hwnd, &paint);
			break;
		}
		default:
		{
			//OutputDebugStringA("default\n");
			result = DefWindowProc(hwnd, uMsg, wParam, lParam);
			break;
		}
	}

	return(result);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS WindowClass = {};

	//WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	//WNDCLASS.hIcon = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

		if(WindowHandle)
		{
			running = true;
			while(running)
			{
				MSG message;
				bool MessageResult = GetMessageA(&message, 0, 0, 0);
				if (MessageResult > 0)
				{
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			//TODO: Logging
		}
	}
	else
	{
		//TODO: Logging
	}

	return(0);
}
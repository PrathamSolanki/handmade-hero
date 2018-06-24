#include <windows.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// TODO: global for now
static bool running;

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixel = 4;

void RenderWeirdGradiant(int BlueOffset, int GreenOffset)
{
	int width = BitmapWidth;
	int height = BitmapHeight;

	int pitch = width*BytesPerPixel;
	uint8 *row = (uint8 *)BitmapMemory;

	for(int y = 0; y < BitmapHeight; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for(int x = 0; x < BitmapWidth; ++x)
		{
			uint8 blue = (x + BlueOffset);
			uint8 green = (y + GreenOffset);
			*pixel++ = (green << 8) | blue;
		}
		row += pitch;
	}
}

void Win32ResizeDIBSection(int width, int height)
{
	// TODO: Bulletproof this
	// May be don't free first, free after, then free first if that fails

	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}
	
	BitmapWidth = width;
	BitmapHeight = height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (width*height)*BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void Win32UpdateWindow(HDC hdc, RECT *ClientRect, int x, int y, int width, int height)
{
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;
	StretchDIBits(hdc, 0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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

			RECT ClientRect;
			GetClientRect(hwnd, &ClientRect);
			
			Win32UpdateWindow(DeviceContext, &ClientRect, x, y, width, height);
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
			int BlueOffset = 0;
			int GreenOffset = 0;
			running = true;
			while(running)
			{
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if(message.message == WM_QUIT)
					{
						running = false;
					}
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				RenderWeirdGradiant(BlueOffset, GreenOffset);

				HDC hdc = GetDC(WindowHandle);
				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32UpdateWindow(hdc, &ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(WindowHandle, hdc);

				++BlueOffset;
				++GreenOffset;
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
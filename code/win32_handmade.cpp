#include <windows.h>
#include <stdint.h>
#include <Xinput.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	// Note: Pixels are always 32 bits wide, Memory order BB GG RR xx
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
};

struct win32_window_dimension
{
	int width;
	int height;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (0);
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (0);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

static void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

win32_window_dimension Win32GetWindowDimension(HWND hwnd)
{
	win32_window_dimension result;

	RECT ClientRect;
	GetClientRect(hwnd, &ClientRect);
	result.height = ClientRect.bottom - ClientRect.top;
	result.width = ClientRect.right - ClientRect.left;

	return (result);
}

// TODO: global for now
static bool GlobalRunning;
static win32_offscreen_buffer GlobalBackBuffer;

void RenderWeirdGradiant(win32_offscreen_buffer *buffer, int BlueOffset, int GreenOffset)
{
	uint8 *row = (uint8 *)buffer->memory;

	for (int y = 0; y < buffer->height; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < buffer->width; ++x)
		{
			uint8 blue = (x + BlueOffset);
			uint8 green = (y + GreenOffset);
			*pixel++ = (green << 8) | blue;
		}
		row += buffer->pitch;
	}
}

void Win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
	// TODO: Bulletproof this
	// May be don't free first, free after, then free first if that fails

	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	int BytesPerPixel = 4;

	/*
		Note: When the biHeight field is negative, this is a clue to
		Windows to treat this bitmap as top-down, not bottom-up,
		meaning that the first three bytes of the image are the
		color for the top left pixel in the image, not the bottom left!
	*/
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (buffer->width * buffer->height) * BytesPerPixel;
	buffer->memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	buffer->pitch = width * BytesPerPixel;
}

void Win32DisplayBuffer(HDC hdc, int WindowWidth, int WindowHeight, win32_offscreen_buffer *buffer)
{
	// TODO:  Aspect ratio correction
	StretchDIBits(hdc, 0, 0, WindowWidth, WindowHeight, 0, 0, buffer->width, buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (uMsg)
	{
	case WM_SIZE:
	{
		OutputDebugStringA("WM_SIZE\n");
		break;
	}
	case WM_CLOSE:
	{
		// TODO: Handle this with a message to the user
		GlobalRunning = false;
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
		GlobalRunning = false;
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

		win32_window_dimension dimension = Win32GetWindowDimension(hwnd);
		Win32DisplayBuffer(DeviceContext, dimension.width, dimension.height, &GlobalBackBuffer);
		EndPaint(hwnd, &paint);
		break;
	}

	case WM_SYSKEYDOWN:
	{
		break;
	}

	case WM_SYSKEYUP:
	{
		break;
	}

	case WM_KEYDOWN:
	{
		break;
	}

	case WM_KEYUP:
	{
		uint32 VKCode = wParam;
		bool WasDown = (lParam & (1 << 30)) != 0;
		bool IsDown = (lParam & (1 << 31)) == 0;

		if (WasDown != IsDown)
		{
			if (VKCode == 'W')
			{
			}
			else if (VKCode == 'S')
			{
			}
			else if (VKCode == 'A')
			{
			}
			else if (VKCode == 'D')
			{
			}
			else if (VKCode == 'Q')
			{
			}
			else if (VKCode == 'E')
			{
			}
			else if (VKCode == VK_UP)
			{
			}
			else if (VKCode == VK_DOWN)
			{
			}
			else if (VKCode == VK_LEFT)
			{
			}
			else if (VKCode == VK_RIGHT)
			{
			}
			else if (VKCode == VK_ESCAPE)
			{
				OutputDebugStringA("ESCAPE: ");
				if (IsDown)
				{
					OutputDebugStringA("IsDown ");
				}
				if (WasDown)
				{
					OutputDebugStringA("WasDown");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == VK_SPACE)
			{
			}
		}
		break;
	}

	default:
	{
		//OutputDebugStringA("default\n");
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		break;
	}
	}

	return (result);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	//WNDCLASS.hIcon = ;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

		if (WindowHandle)
		{
			int BlueOffset = 0;
			int GreenOffset = 0;
			GlobalRunning = true;
			while (GlobalRunning)
			{
				MSG message;
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}

				// TODO: Should we poll this more frequently?
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// Note: Controller is pluged in
						// TODO: See if ControllerState.dwPacketNumber increments too rapidly

						XINPUT_GAMEPAD *pad = &ControllerState.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);

						bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);

						bool LeftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

						bool AButton = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = pad->sThumbLX;
						int16 StickY = pad->sThumbLY;

						BlueOffset += StickX >> 8;
						GreenOffset += StickY >> 8;
					}
					else
					{
						// Note: Controller isn't available
					}
				}

				XINPUT_VIBRATION vibration;
				vibration.wLeftMotorSpeed = 4000;
				vibration.wRightMotorSpeed = 4000;
				XInputSetState(0, &vibration);

				RenderWeirdGradiant(&GlobalBackBuffer, BlueOffset, GreenOffset);

				HDC hdc = GetDC(WindowHandle);
				win32_window_dimension dimension = Win32GetWindowDimension(WindowHandle);
				Win32DisplayBuffer(hdc, dimension.width, dimension.height, &GlobalBackBuffer);
				ReleaseDC(WindowHandle, hdc);

				++BlueOffset;
			}
		}
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}

	return (0);
}
#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

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

struct win32_sound_output
{
	int SamplesPerSecond;
	int ToneHz;
	uint16 ToneVolume;
	uint32 RunningSampleIndex;
	int SquareWaveCounter;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};

static win32_sound_output SoundOutput;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

static void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (!XInputLibrary)
	{
		//TODO: Diagnostic
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState)
		{
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState)
		{
			XInputSetState = XInputSetStateStub;
		}
	}
	else
	{
		//TODO: Diagnostic
	}
}

static void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	//NOTE: Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	//The primary buffer will only be used internally by direct sound
	//we'll only be writing audio to the secondary buffer.

	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate_ = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND DSound;
		if (DirectSoundCreate_ && SUCCEEDED(DirectSoundCreate_(0, &DSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			//NOTE: Get a DirectSound object! - cooperative
			if (SUCCEEDED(DSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescriptor = {};
				BufferDescriptor.dwSize = sizeof(BufferDescriptor);
				BufferDescriptor.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//NOTE: Create a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DSound->CreateSoundBuffer(&BufferDescriptor, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error))
					{
						//NOTe: We have finally set the format
						OutputDebugStringA("We've set format on primary buffer\n");
					}
					else
					{
						//TODO: Diagnostic
					}
				}
				else
				{
					//TODO: Diagnostic
				}
			}
			else
			{
				//TODO: Diagnostic
			}

			//NOTE: Create a secondary buffer
			DSBUFFERDESC BufferDescriptor = {};
			BufferDescriptor.dwSize = sizeof(BufferDescriptor);
			BufferDescriptor.dwFlags = 0;
			BufferDescriptor.dwBufferBytes = BufferSize;
			BufferDescriptor.lpwfxFormat = &WaveFormat;
			HRESULT Error = DSound->CreateSoundBuffer(&BufferDescriptor, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				//NOTE: Start it playing!
				OutputDebugStringA("Created secondary buffer\n");
			}
			else
			{
			}
		}
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

static void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	//TODO: More Test!
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		//TODO: assert that region sizes are valid

		//TODO: Collapse these two loops
		int16 *SampleOut = (int16 *)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;

			SoundOutput->tSine += 2.0f * Pi32 * (1.0f / (real32)SoundOutput->WavePeriod);
			++SoundOutput->RunningSampleIndex;
		}

		SampleOut = (int16 *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;

			SoundOutput->tSine += 2.0f * Pi32 * (1.0f / (real32)SoundOutput->WavePeriod);
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

void Win32DisplayBuffer(HDC hdc, int WindowWidth, int WindowHeight, win32_offscreen_buffer *buffer)
{
	// TODO:  Aspect ratio correction
	StretchDIBits(hdc, 0, 0, WindowWidth, WindowHeight, 0, 0, buffer->width, buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

//NOTE: Sound Test
static short wheelDelta;

//NOTE: Graphics Test
static int BlueOffset = 0;
static int GreenOffset = 0;

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

	case WM_MOUSEWHEEL:
	{
		wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		SoundOutput.ToneHz += (int)(wheelDelta);
		SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
	}

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = wParam;
		bool WasDown = (lParam & (1 << 30)) != 0;
		bool IsDown = (lParam & (1 << 31)) == 0;

		if (VKCode == 'W')
		{
			OutputDebugStringA("W");
			GreenOffset += 10;
		}
		else if (VKCode == 'S')
		{
			OutputDebugStringA("S");
			GreenOffset -= 10;
		}
		else if (VKCode == 'A')
		{
			OutputDebugStringA("A");
			BlueOffset -= 10;
		}
		else if (VKCode == 'D')
		{
			OutputDebugStringA("D");
			BlueOffset += 10;
		}

		if (WasDown != IsDown)
		{
			if (VKCode == 'Q')
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
		bool AltKeyWasDown = (lParam & (1 << 29)) != 0;
		if ((VKCode == VK_F4) && AltKeyWasDown)
		{
			GlobalRunning = false;
		}
		break;
	}

	default:
	{
		//OutputDebugStringA("default\n");
		result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
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
			HDC hdc = GetDC(WindowHandle);

			//NOTE: Sound test
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 1000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.SquareWaveCounter = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.tSine = 0.0f;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

			Win32InitDSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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

						// TODO: We will do deadzone handling later using
						// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
						// XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
						BlueOffset += StickX / 4096;
						GreenOffset += StickY / 4096;
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

				//Note: Direct sound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;

					//TODO: Change this to using a lower latency offset from the playcursor
					// when we actually start having sound effects
					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
				}

				win32_window_dimension dimension = Win32GetWindowDimension(WindowHandle);
				Win32DisplayBuffer(hdc, dimension.width, dimension.height, &GlobalBackBuffer);

				//++BlueOffset;
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
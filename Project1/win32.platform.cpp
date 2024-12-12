#include "utils.cpp"
#include <windows.h>

// Global variable to control the running state of the game
global_variable bool running = true;

// Structure to handle rendering details
struct Render_State {
	int height, width;        // Dimensions of the window
	void* memory;             // Pointer to pixel memory

	BITMAPINFO bitmap_info;   // Bitmap information for rendering
};

global_variable Render_State render_state;

#include "platform.cpp"
#include "renderer.cpp"
#include "game.cpp"

// Window callback function to handle window events
LRESULT CALLBACK window_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;

	switch (uMsg) {
	case WM_CLOSE:
	case WM_DESTROY: {
		// Stop the game loop when the window is closed
		running = false;
	} break;

	case WM_SIZE: {
		// Handle window resizing
		RECT rect;
		GetClientRect(hwnd, &rect);
		render_state.width = rect.right - rect.left;
		render_state.height = rect.bottom - rect.top;

		int size = render_state.width * render_state.height * sizeof(unsigned int);

		// Free previous memory allocation if any
		if (render_state.memory) VirtualFree(render_state.memory, 0, MEM_RELEASE);
		render_state.memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		// Configure bitmap information for rendering
		render_state.bitmap_info.bmiHeader.biSize = sizeof(render_state.bitmap_info.bmiHeader);
		render_state.bitmap_info.bmiHeader.biWidth = render_state.width;
		render_state.bitmap_info.bmiHeader.biHeight = render_state.height;
		render_state.bitmap_info.bmiHeader.biPlanes = 1;
		render_state.bitmap_info.bmiHeader.biBitCount = 32;
		render_state.bitmap_info.bmiHeader.biCompression = BI_RGB;

	} break;

	default: {
		// Default window procedure for other messages
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
	return result;
}

// Main entry point of the application
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// Hide the cursor for the game
	ShowCursor(FALSE);

	// Create and register a window class
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Game Window Class";
	window_class.lpfnWndProc = window_callback;
	RegisterClass(&window_class);

	// Create the game window
	HWND window = CreateWindow(window_class.lpszClassName, "Pong - Tutorial", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, hInstance, 0);
	{
		// Enable fullscreen mode
		SetWindowLong(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowPos(window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}

	// Get device context for rendering
	HDC hdc = GetDC(window);

	Input input = {};  // Input structure to handle game inputs

	// Timing variables for game loop
	float delta_time = 0.016666f;  // Approx. 60 FPS
	LARGE_INTEGER frame_begin_time;
	QueryPerformanceCounter(&frame_begin_time);

	float performance_frequency;
	{
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		performance_frequency = (float)perf.QuadPart;
	}

	// Main game loop
	while (running) {
		// Handle input
		MSG message;

		// Reset button states
		for (int i = 0; i < BUTTON_COUNT; i++) {
			input.buttons[i].changed = false;
		}

		// Process Windows messages
		while (PeekMessage(&message, window, 0, 0, PM_REMOVE)) {

			switch (message.message) {
			case WM_KEYUP:
			case WM_KEYDOWN: {
				// Handle keyboard input
				u32 vk_code = (u32)message.wParam;
				bool is_down = ((message.lParam & (1 << 31)) == 0);

#define process_button(b, vk)\
case vk: {\
input.buttons[b].changed = is_down != input.buttons[b].is_down;\
input.buttons[b].is_down = is_down;\
} break;

				switch (vk_code) {
					process_button(BUTTON_UP, VK_UP);
					process_button(BUTTON_DOWN, VK_DOWN);
					process_button(BUTTON_W, 'W');
					process_button(BUTTON_S, 'S');
					process_button(BUTTON_LEFT, VK_LEFT);
					process_button(BUTTON_RIGHT, VK_RIGHT);
					process_button(BUTTON_ENTER, VK_RETURN);
				}
			} break;

			default: {
				// Pass other messages to default handler
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			}

		}

		// Simulate game logic
		simulate_game(&input, delta_time);

		// Render the game
		StretchDIBits(hdc, 0, 0, render_state.width, render_state.height, 0, 0, render_state.width, render_state.height, render_state.memory, &render_state.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

		// Measure frame time for consistent timing
		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;
	}
}

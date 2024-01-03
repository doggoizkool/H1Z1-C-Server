#if defined(YOTE_INTERNAL)
#include <stdio.h>
#else
static void platform_win_console_write(char *format, ...);
#define printf(s, ...) platform_win_console_write(s, __VA_ARGS__)
#endif // YOTE_INTERNAL

#include <stdbool.h>
#include "yote.h"
#define YOTE_PLATFORM_USE_SOCKETS 1
#define YOTE_PLATFORM_WINDOWS 1
#include "yote_platform.h"
#include "game_server.h"

#define MODULE_FILE "login_server_module.dll"
#define MODULE_FILE_TEMP "login_server_module_temp.dll"
#define MODULE_LOCK_FILE ".reload-lock"

typedef struct App_Code
{
	HMODULE module;
	FILETIME module_last_write_time;
	app_tick_t *tick_func;
	b32 is_valid;
} App_Code;

APP_TICK(app_tick_stub)
{
	UNUSED(app_memory);
}

FILETIME win32_get_last_write_time(char *filename)
{
	FILETIME result = {0};

	WIN32_FILE_ATTRIBUTE_DATA file_data;
	if (GetFileAttributesExA(filename, GetFileExInfoStandard, &file_data))
	{
		result = file_data.ftLastWriteTime;
	}

	return result;
}

App_Code win32_app_code_load()
{
	App_Code result = {0};

	result.module_last_write_time = win32_get_last_write_time(MODULE_FILE);
	CopyFileA(MODULE_FILE, MODULE_FILE_TEMP, false);

	result.module = LoadLibraryA(MODULE_FILE_TEMP);
	if (result.module)
	{
		result.tick_func = (app_tick_t *)GetProcAddress(result.module, "server_tick");
		result.is_valid = !!result.tick_func;
	}

	if (!result.is_valid)
	{
		result.tick_func = app_tick_stub;
	}

	return result;
}

void win32_app_code_unload(App_Code *app_code)
{
	if (app_code->module)
	{
		FreeLibrary(app_code->module);
		app_code->module = 0;
	}

	app_code->is_valid = false;
	app_code->tick_func = app_tick_stub;
}

#if defined(YOTE_INTERNAL)
int main(void)
#else
int mainCRTStartup(void)
#endif // YOTE_INTERNAL
{
	App_Memory app_memory =
		{
			.platform_api =
				{
					.folder_create = platform_win_folder_create,
					.buffer_write_to_file = platform_win_buffer_write_to_file,
					.buffer_load_from_file = platform_win_buffer_load_from_file,
					.socket_udp_create_and_bind = platform_win_socket_udp_create_and_bind,
					.receive_from = platform_win_receive_from,
					.send_to = platform_win_send_to,
					.wall_clock = platform_win_wall_clock,
					.elapsed_seconds = platform_win_elapsed_seconds,
				},
			.backing_memory.size = MB(100),
			.backing_memory.data = VirtualAlloc(NULL, app_memory.backing_memory.size, MEM_COMMIT, PAGE_READWRITE),
		};

	// #if defined(YOTE_INTERNAL)
	// core_memory_fill(app_memory.backing_memory.data, 0xcc, app_memory.backing_memory.size);
	// #endif // YOTE_INTERNAL

	LARGE_INTEGER local_performance_frequency;
	QueryPerformanceFrequency(&local_performance_frequency);
	global_performance_frequency = local_performance_frequency.QuadPart;
	b32 is_sleep_granular = timeBeginPeriod(1) == TIMERR_NOERROR;
	f32 tick_rate = 128.0f;
	f32 target_seconds_per_tick = 1.0f / tick_rate;

#if defined(TERMINAL_UI)
	HANDLE console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
													  0,
													  NULL,
													  CONSOLE_TEXTMODE_BUFFER,
													  NULL);
	SMALL_RECT window_rect = {0, 0, 1, 1};
	SetConsoleWindowInfo(console_handle, true, &window_rect);
	SetConsoleScreenBufferSize(console_handle, (COORD){SCREEN_WIDTH, SCREEN_HEIGHT});
	SetConsoleActiveScreenBuffer(console_handle);

	window_rect = (SMALL_RECT){0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
	SetConsoleWindowInfo(console_handle, true, &window_rect);
#endif // TERMINAL_UI

	App_Code app_code = win32_app_code_load();
	u64 previous_counter = platform_win_wall_clock();
	b32 is_running = true;
	while (is_running)
	{
		app_code.tick_func(&app_memory);

#if defined(TERMINAL_UI)
		DWORD bytes_written;
		WriteConsoleOutputCharacter(console_handle,
									(LPCSTR)app_memory.screen.data,
									(DWORD)app_memory.screen.size,
									(COORD){0},
									&bytes_written);
#endif // TERMINAL_UI

		//		local_persist i32 arena_debug_cooldown_ticks;
		//		if (GetKeyState('M') & 0x8000)
		//		{
		//			if (!arena_debug_cooldown_ticks)
		//			{
		//				arena_debug_cooldown_ticks = 30;
		//
		//				printf("\nArena Debug\n[*] %s:\t\tPEAK: %lld KiB\tTAIL: %lld KiB\tPADDING: %lld B\tCOUNT: %d\n",
		//				       app_memory.app_state->arena_total.name,
		//				       app_memory.app_state->arena_total.peak_used / 1024,
		//				       app_memory.app_state->arena_total.tail_offset / 1024,
		//				       app_memory.app_state->arena_total.padding / 1024,
		//				       app_memory.app_state->arena_total.active_count);
		//				printf("[*] %s:\t\tPEAK: %lld KiB\tTAIL: %lld KiB\tPADDING: %lld B\tCOUNT: %d\n",
		//				       app_memory.app_state->arena_per_tick.name,
		//				       app_memory.app_state->arena_per_tick.peak_used / 1024,
		//				       app_memory.app_state->arena_per_tick.tail_offset / 1024,
		//				       app_memory.app_state->arena_per_tick.padding / 1024,
		//				       app_memory.app_state->arena_per_tick.active_count);
		//			}
		//		}
		//		if (arena_debug_cooldown_ticks)
		//		{
		//			arena_debug_cooldown_ticks -= 1;
		//		}

		u64 work_counter = platform_win_wall_clock();
		app_memory.work_ms = 1000.0f * platform_win_elapsed_seconds(previous_counter, work_counter);

		f32 elapsed_tick_seconds = platform_win_elapsed_seconds(previous_counter, platform_win_wall_clock());
		if (elapsed_tick_seconds < target_seconds_per_tick)
		{
			if (is_sleep_granular)
			{
				i32 sleep_ms = (i32)(target_seconds_per_tick * 1000.0f) - (i32)(elapsed_tick_seconds * 1000.0f) - 1;
				// char sleep_info[128] = { 0 };
				// stbsp_snprintf(sleep_info, sizeof(sleep_info), "[*] Sleeping %dms\n", sleep_ms);
				// OutputDebugString(sleep_info);

				if (sleep_ms > 0)
				{
					Sleep(sleep_ms);
				}
			}

			while (elapsed_tick_seconds < target_seconds_per_tick)
			{
				// Sleep(0);
				elapsed_tick_seconds = platform_win_elapsed_seconds(previous_counter, platform_win_wall_clock());
			}
		}

		u64 end_counter = platform_win_wall_clock();
		app_memory.tick_ms = 1000.0f * platform_win_elapsed_seconds(previous_counter, end_counter);
		previous_counter = end_counter;

		app_memory.tick_count++;
	}

	return 0;
}
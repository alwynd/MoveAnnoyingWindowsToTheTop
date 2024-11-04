#include "movewindows.h"

RECT ScreenDimensions;

int WindowLeftPosition = 0;
int WindowTopPosition = 0;

LOG_LEVEL LogLevel = LOG_LEVEL::LOG_INFO;

bool containsCI(std::string str1, std::string str2) {
	std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

	return (str1.find(str2) != std::string::npos);
}

// Get current ms
const long long CurrentTimeMS()
{
	chrono::system_clock::time_point currentTime = chrono::system_clock::now();

	long long transformed = currentTime.time_since_epoch().count() / 1000000;
	long long millis = transformed % 1000;

	return millis;
}


// Format date
void FormatDate(char* buffer)
{
	buffer[0] = '\0';
	time_t timer;
	tm tm_info;

	timer = time(NULL);
	localtime_s(&tm_info, &timer);

	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S\0", &tm_info);
}


// Debug string to console
void debug(const char* msg, const LOG_LEVEL& logLevel)
{
	if ((int)logLevel < (int)LogLevel) return;
	char buffer[27];
	ZeroMemory(buffer, 27);
	FormatDate(buffer);

	printf("%s [%i] - %s\n", buffer, (int)logLevel, msg); fflush(stdout);

}

// Enum window callback
BOOL CALLBACK EnumWindowCallback(HWND hWnd, LPARAM lparam)
{
	ProcessWindow(hWnd);
	//EnumChildWindows(hWnd, EnumChildWindowCallback, lparam);

	return TRUE;
}

// Enum child window callback
BOOL CALLBACK EnumChildWindowCallback(HWND hWnd, LPARAM lParam)
{
	ProcessWindow(hWnd);
	return TRUE;
}


void ProcessWindow(const HWND& hWnd)
{
	int length = GetWindowTextLength(hWnd);
	char buffer[1024];
	ZeroMemory(buffer, 1024);
	DWORD dwProcId = 0;

	// List visible windows with a non-empty title
	if (IsWindowVisible(hWnd) && length != 0)
	{
		RunningProcess process;

		process.WindowHandle = hWnd;
		int wlen = length + 1;
		GetWindowText(hWnd, process.WindowTitle, wlen);

		RECT windowRect;
		int windowWidth = 0;
		int windowHeight = 0;

		if (GetWindowRect(hWnd, &windowRect))
		{
			windowWidth = windowRect.right - windowRect.left;
			windowHeight = windowRect.bottom - windowRect.top;
		}

		if (containsCI(process.WindowTitle, "flex"))
		{
			return;
		}

		if (!containsCI(process.WindowTitle, "Dialog"))
		{
			//return;
		}

		char className[256];
		GetClassNameA(process.WindowHandle, className, sizeof(className));

		sprintf_s(buffer, "MoveWindows.EnumWindowCallback, DISCOVER Window: hwnd: %p, Title: '%-55s', Executable: '%-55s', Min: '%-5s', Res: '%iWx%iH', Class: '%-55s', \0",
			process.WindowHandle, process.WindowTitle, process.ImageName, BOOL_TEXT(process.IsMinimized), windowWidth, windowHeight, className); debug(buffer, LOG_LEVEL::LOG_VERBOSE);

		if (windowHeight > 200)
		{
			return;
		}

		if (!ContainsProgressBar(process))
		{
			if (!containsCI(className, "Qt653QWindowToolSaveBits"))
			{
				if (!containsCI(process.WindowTitle, "progress"))
				{
					return;
				}
			}
		}

		if (containsCI(className, "tooltip"))
		{
			return;
		}


		GetWindowThreadProcessId(hWnd, &dwProcId);
		if (dwProcId)
		{
			HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
			if (hProc)
			{
				HMODULE hMod;
				DWORD cbNeeded;
				process.IsMinimized = IsIconic(hWnd);
				if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeeded))
				{
					GetModuleBaseNameA(hProc, hMod, process.ImageName, MAX_PATH);
				} //if
				CloseHandle(hProc);

				if (windowRect.top != WindowTopPosition)
				{
					sprintf_s(buffer, "MoveWindows.EnumWindowCallback, MOVE Window: hwnd: %p, Title: '%-55s', Executable: '%-55s', Min: '%-5s', Res: '%iWx%iH'\0",
						process.WindowHandle, process.WindowTitle, process.ImageName, BOOL_TEXT(process.IsMinimized), windowWidth, windowHeight); debug(buffer, LOG_LEVEL::LOG_INFO);

					SetWindowPos(hWnd, 0, WindowLeftPosition, WindowTopPosition, 0, 0, SWP_NOSIZE);
				}
				WindowLeftPosition += 50;
				WindowTopPosition += 30;

			} //if
		} //if
	}

}


// Static callback function
BOOL CALLBACK EnumChildProgressBars(HWND hChildWnd, LPARAM lParam)
{
	EnumChildParams* params = (EnumChildParams*)(lParam);
	const RunningProcess* process = params->process;

	char buffer[1024];
	ZeroMemory(buffer, 1024);

	char className[256];
	GetClassNameA(hChildWnd, className, sizeof(className));
	sprintf_s(buffer, "MoveWindows.ContainsProgressBar Window: '%s', child className: '%s'\0", process->WindowTitle, className); debug(buffer, LOG_LEVEL::LOG_VERBOSE);

	if (strcmp(className, PROGRESS_CLASS) == 0 || strstr(className, "progress") != nullptr)
	{
		// Found a progress bar
		params->hasProgressBar = true;
		sprintf_s(buffer, "MoveWindows.ContainsProgressBar Window: '%s', child className: '%s', FOUND A PROGRESS BAR!!\0", process->WindowTitle, className); debug(buffer, LOG_LEVEL::LOG_VERBOSE);
		return FALSE; // Stop enumerating
	}
	return TRUE; // Continue enumerating
}


bool ContainsProgressBar(const RunningProcess& process)
{
	EnumChildParams params;
	params.process = &process;
	params.hasProgressBar = false;

	EnumChildWindows(process.WindowHandle, EnumChildProgressBars, (LPARAM)&params);
	return params.hasProgressBar;
}


// Win main!
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
	UINT_PTR timerID = 0;
	char buffer[1024];
	ZeroMemory(buffer, 1024);
	int rc = 0;

	GetWindowRect(GetDesktopWindow(), &ScreenDimensions);
	sprintf_s(buffer, "MoveWindows.WinMain:-- START, ScreenWidth: %i, ScreenHeight: %i\0", ScreenDimensions.right, ScreenDimensions.bottom); debug(buffer, LOG_LEVEL::LOG_INFO);

	// Set a timer to call TimerProc every 5000 ms (5 seconds)
	timerID = SetTimer(NULL, 0, 3000, TimerProc);

	// Main message loop
	MSG msg;
	bool quit = false;
	while (!quit && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (FileExists("scripts/stop.stop"))
		{
			quit = true;
		} //if
	}

	// Cleanup and exit
	if (timerID != 0) 
	{
		debug("MoveWindows.WinMain Killing timer.", LOG_LEVEL::LOG_INFO);
		KillTimer(NULL, timerID);
	}
DONE:
	debug("MoveWindows.WinMain:-- DONE", LOG_LEVEL::LOG_INFO);
	return rc;
}


// Does file exist.
bool FileExists(const char* file)
{
	FILE* f = NULL;
	fopen_s(&f, file, "rb");
	if (f)
	{
		fclose(f);
		return  true;
	} //if

	return false;
}


// Timer callback function
VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	debug("MoveWindows.TimerProc:-- START", LOG_LEVEL::LOG_VERBOSE);
	// This code will be executed every 5 seconds
	WindowLeftPosition = ((ScreenDimensions.right - ScreenDimensions.left) / 2) - 250;
	WindowTopPosition = 50;
	EnumWindows(EnumWindowCallback, 0);
}


void split(const char* splitString, const char splitChar, std::vector<string>& strings)
{

	char logbuffer[1024];
	ZeroMemory(logbuffer, 1024);

	size_t bufferidx = 0;
	char buffer[1024];
	ZeroMemory(buffer, 1024);

	int len = (int)strnlen_s(splitString, 1024);
	for (int i = 0; i < len; i++)
	{
		if (splitString[i] == splitChar)
		{
			//sprintf_s(logbuffer, "Split \t : Adding: '%s'\0", buffer); debug(logbuffer);
			if (strnlen_s(buffer, 1024) > 0) strings.push_back(string(buffer));
			bufferidx = 0;
			ZeroMemory(buffer, 1024);
			continue;
		}
		buffer[bufferidx++] = splitString[i];
	}
	if (strnlen_s(buffer, 1024) > 0) strings.push_back(string(buffer));

}



// Endswith string
bool endswith(const char* string, const char* search)
{
	char logbuffer[1024];
	ZeroMemory(logbuffer, 1024);

	int stringlen = (int)strnlen_s(string, 1024);
	int searchlen = (int)strnlen_s(search, 1024);
	//sprintf_s(logbuffer, "endswith: \t stringlen: %i, searchlen: %i\0", stringlen, searchlen); debug(logbuffer);

	if (stringlen < searchlen) return false;
	const char* buffer = string + (stringlen - searchlen);

	bool match = strcmp(buffer, search) == 0;
	//sprintf_s(logbuffer, "endswith: \t\t search len: %i, buffer: '%s', match: '%s'\0", searchlen, buffer, BOOL_TEXT(match)); debug(logbuffer);


	return match;
}




// Starts with string
bool startswith(const char* string, const char* search)
{
	char logbuffer[1024];
	ZeroMemory(logbuffer, 1024);

	char buffer[1024];
	ZeroMemory(buffer, 1024);

	// copy string, into buffer, for length of search
	int len = (int)strnlen_s(search, 1024);
	//sprintf_s(logbuffer, "startswith: \t search len: %i\0", len); debug(logbuffer);

	CopyMemory(buffer, string, len);
	//sprintf_s(logbuffer, "startswith: \t search len: %i, buffer: '%s'\0", len, buffer); debug(logbuffer);

	bool match = strcmp(buffer, search) == 0;
	//sprintf_s(logbuffer, "startswith: \t\t search len: %i, buffer: '%s', match: '%s'\0", len, buffer, BOOL_TEXT(match)); debug(logbuffer);

	return match;
}




// is equal (CI)?
bool equals(const char* string1, const char* string2)
{
	char buffer[1024];
	ZeroMemory(buffer, 1024);

	char str1[1024];
	char str2[1024];
	ZeroMemory(str1, 1024);
	ZeroMemory(str2, 1024);

	int len = (int)strnlen_s(string1, 1024);
	int len2 = (int)strnlen_s(string2, 1024);

	if (len != len2) return false;

	CopyMemory(str1, string1, len);
	CopyMemory(str2, string2, len2);

	_strlwr_s(str1, len + 1);
	_strlwr_s(str2, len2 + 1);

	bool match = strcmp(str1, str2) == 0;
	//sprintf_s(buffer, "equals: \t\t search len: %i, str1: '%s', str2: '%s', match: '%s'\0", len, str1, str2, BOOL_TEXT(match)); debug(buffer);

	return match;

}
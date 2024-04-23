#include "Console.h"
#include <Windows.h>

static HANDLE				sConsoleHandle = nullptr;
static LogLevel				sVerbosity; // Automatically initialized to Trace

void Console::sInitialize()
{
	sConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Console::sSetVerbosity(LogLevel inType)
{
	sVerbosity = inType;
}

void Console::sLog(LogLevel inType, std::string_view inLogString)
{
	switch (inType)
	{
	case LogLevel::Trace:
		if (sConsoleHandle)
			SetConsoleTextAttribute(sConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		break;
	case LogLevel::Info:
		if (sConsoleHandle)
			SetConsoleTextAttribute(sConsoleHandle, FOREGROUND_GREEN);
		break;
	case LogLevel::Warning:
		if (sConsoleHandle)
			SetConsoleTextAttribute(sConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	case LogLevel::Error:
		if (sConsoleHandle)
			SetConsoleTextAttribute(sConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;
	case LogLevel::Fatal:
		if (sConsoleHandle)
			SetConsoleTextAttribute(sConsoleHandle, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;
	default:
		break;
	}

	printf("%s", inLogString.data());
	OutputDebugStringA(inLogString.data());

	if (sConsoleHandle)
		SetConsoleTextAttribute(sConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}

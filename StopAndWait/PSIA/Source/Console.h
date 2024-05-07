#pragma once
#include <string>
#include <cstdarg>

inline std::string StringFormat(const char* inFMT, ...)
{
	char buffer[1024];

	// A complete object type (in practice, a unique built-in type or char*)
	// suitable for holding the information needed by the macros such as va_start
	va_list list;

	// Enables access to variadic function arguments
	// The second parameter is the named parameter preceding the first variable parameter
	va_start(list, inFMT);

	// Converts data from 'list' to character string equivalents and writes the results to 'buffer'
	vsnprintf(buffer, sizeof(buffer), inFMT, list);

	// Ends traversal of the variadic function arguments
	va_end(list);

	return std::string(buffer);
}

enum class LogLevel
{
	Trace,
	Info,
	Warning,
	Error,
	Fatal
};

class Console
{
public:
	static void					sInitialize();

	template<typename... Args>
	static void					sPrintMessage(LogLevel inType, std::string_view inTag, std::string_view inMessage, Args&& ...inArgs);

	static void					sLog(LogLevel inType, std::string_view inLogString);

	// Set which level of messages to print
	static void					sSetVerbosity(LogLevel inType);

private:
	template <class T>
	static decltype(auto)		GetFormatArgument(const T& inArg)
	{
		return inArg;
	}

	static inline const char* GetFormatArgument(const std::string& inArg)
	{
		return inArg.c_str();
	}
};

#define PSIA_TRACE_TAG(tag, message, ...)		Console::sPrintMessage(LogLevel::Trace, tag, message, __VA_ARGS__)
#define PSIA_INFO_TAG(tag, message, ...)		Console::sPrintMessage(LogLevel::Info, tag, message, __VA_ARGS__)
#define PSIA_WARNING_TAG(tag, message, ...)		Console::sPrintMessage(LogLevel::Warning, tag, message, __VA_ARGS__)
#define PSIA_ERROR_TAG(tag, message, ...)		Console::sPrintMessage(LogLevel::Error, tag, message, __VA_ARGS__)
#define PSIA_FATAL_TAG(tag, message, ...)		Console::sPrintMessage(LogLevel::Fatal, tag, message, __VA_ARGS__)

#define PSIA_TRACE(message, ...)				Console::sPrintMessage(LogLevel::Trace, "", message, __VA_ARGS__)
#define PSIA_INFO(message, ...) 				Console::sPrintMessage(LogLevel::Info, "", message, __VA_ARGS__)
#define PSIA_WARNING(message, ...)				Console::sPrintMessage(LogLevel::Warning, "", message, __VA_ARGS__)
#define PSIA_ERROR(message, ...)				Console::sPrintMessage(LogLevel::Error, "", message, __VA_ARGS__)
#define PSIA_FATAL(message, ...)				Console::sPrintMessage(LogLevel::Fatal, "", message, __VA_ARGS__)

template<typename ...Args>
inline void Console::sPrintMessage(LogLevel inType, std::string_view inTag, std::string_view inMessage, Args&& ...inArgs)
{
#if defined(AG_DISTRIBUTION)
	return;
#endif

	std::string log_string = inTag.empty() ? "%s%s\n" : "[%s] %s\n";
	log_string = StringFormat(log_string.c_str(), inTag.data(), inMessage.data());
	log_string = StringFormat(log_string.c_str(), GetFormatArgument(std::forward<Args&&>(inArgs))...);

	sLog(inType, log_string.c_str());
}

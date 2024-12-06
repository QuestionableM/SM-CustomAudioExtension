#pragma once

#include <SmSdk/win_include.hpp>

#include <cwctype>
#include <string>

namespace String
{
	inline unsigned char HexStrtolSafe(char* v_ptr)
	{
		char* v_end_ptr;
		const long v_output = strtol(v_ptr, &v_end_ptr, 16);

		*v_ptr = 0;
		if (v_ptr == v_end_ptr) return 0;

		return static_cast<unsigned char>(v_output);
	}

	inline std::string ToUtf8(const std::wstring& wstr)
	{
		const int v_count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);

		std::string v_str(v_count, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &v_str[0], v_count, NULL, NULL);

		return v_str;
	}

	inline std::wstring ToWide(const std::string_view& v_str)
	{
		const int v_str_sz = static_cast<int>(v_str.size());
		const int v_count = MultiByteToWideChar(CP_UTF8, 0, v_str.data(), v_str_sz, NULL, 0);

		std::wstring v_wstr(v_count, 0);
		MultiByteToWideChar(CP_UTF8, 0, v_str.data(), v_str_sz, &v_wstr[0], v_count);

		return v_wstr;
	}

	inline std::wstring ToWide(const char* str)
	{
		const int v_str_sz = static_cast<int>(strlen(str));
		const int v_count = MultiByteToWideChar(CP_UTF8, 0, str, v_str_sz, NULL, 0);

		std::wstring v_wstr(v_count, 0);
		MultiByteToWideChar(CP_UTF8, 0, str, v_str_sz, &v_wstr[0], v_count);

		return v_wstr;
	}

	inline std::wstring ToWide(const std::string& str)
	{
		const int v_str_sz = static_cast<int>(str.size());
		const int v_count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), v_str_sz, NULL, 0);

		std::wstring v_wstr(v_count, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), v_str_sz, &v_wstr[0], v_count);

		return v_wstr;
	}
}
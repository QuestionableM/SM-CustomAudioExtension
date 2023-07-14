#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace File
{
	inline bool Exists(const std::string& path)
	{
		namespace fs = std::filesystem;

		std::error_code v_ec;
		const bool v_exists = fs::exists(path, v_ec);

		return !v_ec && v_exists;
	}

	inline bool ReadToString(const std::wstring& path, std::string& r_output)
	{
		std::ifstream input_file(path, std::ios::binary);
		if (!input_file.is_open()) return false;

		input_file.seekg(0, std::ios::end);
		r_output.resize(input_file.tellg());
		input_file.seekg(0, std::ios::beg);

		input_file.read(r_output.data(), r_output.size());
		input_file.close();

		return true;
	}

	inline bool ReadToStringED(const std::wstring& path, std::string& r_output)
	{
		std::ifstream v_input_file(path, std::ios::binary);
		if (!v_input_file.is_open()) return false;

		//Check the first 3 bytes of the file
		char v_encoding_buffer;
		v_input_file.read(&v_encoding_buffer, 1);

		const bool v_guess_has_encoding = (v_encoding_buffer < 0);
		const std::size_t v_file_offset = v_guess_has_encoding ? 3 : 0;

		v_input_file.seekg(0, std::ios::end);
		r_output.resize(static_cast<std::size_t>(v_input_file.tellg()) - v_file_offset);
		v_input_file.seekg(v_file_offset, std::ios::beg);

		v_input_file.read(r_output.data(), r_output.size());
		v_input_file.close();

		return true;
	}
}
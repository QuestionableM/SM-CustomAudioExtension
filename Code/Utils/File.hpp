#pragma once

#include <filesystem>
#include <string>

namespace File
{
	bool Exists(const std::string& path)
	{
		namespace fs = std::filesystem;

		std::error_code v_ec;
		const bool v_exists = fs::exists(path, v_ec);

		return !v_ec && v_exists;
	}
}
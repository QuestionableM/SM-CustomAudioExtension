#pragma once

#include <string_view>
#include <string>

#include <cstdint>
#include <cstddef>

namespace Hashing
{
	struct StringHash
	{
		using is_transparent = void;

		inline std::size_t operator()(const std::string& str) const noexcept
		{
			return std::hash<std::string>{}(str);
		}

		inline std::size_t operator()(const std::string_view& str) const noexcept
		{
			return std::hash<std::string_view>{}(str);
		}
	};
}
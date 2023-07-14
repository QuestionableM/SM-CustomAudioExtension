#pragma once

#include <simdjson\simdjson.h>

class JsonReader
{
public:
	//Only used by Simdjson as it doesn't support json with comments
	static void RemoveComments(std::string& json_string);

	//Parse simdjson unchecked
	static bool LoadParseSimdjson(const std::wstring& path, simdjson::dom::document& v_doc);
	//Parse simdjson with root node type check
	static bool LoadParseSimdjsonC(const std::wstring& path, simdjson::dom::document& v_doc, const simdjson::dom::element_type& type_check);
	//Parse simdjson with comments
	static bool LoadParseSimdjsonComments(const std::wstring& path, simdjson::dom::document& v_doc);
	//Parse simdjson with comments and root node type checking
	static bool LoadParseSimdjsonCommentsC(const std::wstring& path, simdjson::dom::document& v_doc, const simdjson::dom::element_type& type_check);
	//Parse simdjson from string
	static bool ParseSimdjsonString(const std::string& json_str, simdjson::dom::document& v_doc);


	//Should be used to get numbers from simdjson elements
	template<typename T, typename V>
	inline constexpr static T GetNumber(const V& v_elem)
	{
		static_assert(std::is_arithmetic_v<T>, "Json::GetNumber -> Template argument must be of arithmetic type!");
		static_assert(
			std::is_same_v<V, simdjson::dom::element> ||
			std::is_same_v<V, simdjson::simdjson_result<simdjson::dom::element>>,
			"Json::GetNumber -> Template can only be used with simdjson::dom::element");

		switch (v_elem.type())
		{
		case simdjson::dom::element_type::DOUBLE:
			{
				if constexpr (std::is_same_v<T, double>)
					return v_elem.get_double();
				else
					return static_cast<T>(v_elem.get_double());
			}
		case simdjson::dom::element_type::INT64:
			{
				if constexpr (std::is_same_v<T, long long>)
					return v_elem.get_int64();
				else
					return static_cast<T>(v_elem.get_int64());
			}
		case simdjson::dom::element_type::UINT64:
			{
				if constexpr (std::is_same_v<T, unsigned long long>)
					return v_elem.get_uint64();
				else
					return static_cast<T>(v_elem.get_uint64());
			}
		}

		return 0;
	}

private:
	JsonReader() = default;
	JsonReader(const JsonReader&) = delete;
	JsonReader(JsonReader&&) = delete;
	~JsonReader() = default;
};
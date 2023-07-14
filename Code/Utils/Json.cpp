#include "Json.hpp"

#include "Utils\Console.hpp"
#include "Utils\File.hpp"

void JsonReader::RemoveComments(std::string& json_string)
{
	std::string v_output;
	v_output.reserve(json_string.size());

	const char* const v_data_beg = json_string.data();
	const char* const v_data_end = v_data_beg + json_string.size();

	const char* v_data = v_data_beg;

	std::size_t v_data_ptr = 0;
	while (v_data != v_data_end)
	{
		switch (*v_data)
		{
		case '\"':
			{
				v_data = strchr(v_data + 1, '\"');
				if (!v_data) goto smc_escape_loop;

				break;
			}
		case '/':
			{
				const char* v_last_char = v_data++;
				if (v_data == v_data_end)
					goto smc_escape_loop;

				switch (*v_data)
				{
				case '/':
					{
						v_output.append(json_string.begin() + v_data_ptr, json_string.begin() + (v_last_char - v_data_beg));

						v_data = strchr(v_data, '\n');
						if (!v_data) goto smc_escape_loop;

						v_data_ptr = v_data - v_data_beg;
						continue;
					}
				case '*':
					{
						v_output.append(json_string.begin() + v_data_ptr, json_string.begin() + (v_last_char - v_data_beg));

						v_data = strstr(v_data, "*/");
						if (!v_data) goto smc_escape_loop;

						v_data_ptr = (v_data += 2) - v_data_beg;
						continue;
					}
				default:
					break;
				}

				break;
			}
		default:
			break;
		}
		/*if (*v_data == '\"')
		{
			v_data = strchr(v_data + 1, '\"');
			if (!v_data) break;
		}
		else if (*v_data == '/')
		{
			const char* v_last_char = v_data++;
			if (v_data == v_data_end)
			{
				break;
			}
			else if (*v_data == '/')
			{
				v_output.append(json_string.begin() + v_data_ptr, json_string.begin() + (v_last_char - v_data_beg));

				v_data = strchr(v_data, '\n');
				if (!v_data) break;

				v_data_ptr = v_data - v_data_beg;
				continue;
			}
			else if (*v_data == '*')
			{
				v_output.append(json_string.begin() + v_data_ptr, json_string.begin() + (v_last_char - v_data_beg));

				v_data = strstr(v_data, "* /");
				if (!v_data) break;

				v_data_ptr = (v_data += 2) - v_data_beg;
				continue;
			}
		}*/

		v_data++;
	}

smc_escape_loop:

	if (v_data)
	{
		const std::size_t v_ptr_diff = v_data - v_data_beg;
		const std::size_t v_diff_test = v_ptr_diff - v_data_ptr;
		if (v_diff_test != json_string.size())
		{
			v_output.append(json_string.begin() + v_data_ptr, json_string.begin() + v_ptr_diff);
			json_string = std::move(v_output);
		}
	}
	//else
	//{
	//	v_output.append(json_string.substr(v_data_ptr));
	//}
}

bool JsonReader::LoadParseSimdjson(const std::wstring& path, simdjson::dom::document& v_doc)
{
	try
	{
		std::string v_json_str;
		if (!File::ReadToString(path, v_json_str))
			return false;

		simdjson::dom::parser v_parser;
		v_parser.parse_into_document(v_doc, v_json_str);

		return true;
	}
#if defined(_DEBUG) || defined(DEBUG)
	catch (const simdjson::simdjson_error& v_err)
	{
		DebugErrorL("Couldn't parse: ", path, "\nError: ", v_err.what());
	}
#else
	catch (...) {}
#endif

	return false;
}

bool JsonReader::LoadParseSimdjsonC(const std::wstring& path, simdjson::dom::document& v_doc, const simdjson::dom::element_type& type_check)
{
	try
	{
		std::string v_json_str;
		if (!File::ReadToStringED(path, v_json_str))
			return false;

		simdjson::dom::parser v_parser;
		v_parser.parse_into_document(v_doc, v_json_str);

		const auto v_root = v_doc.root();
		if (v_root.type() != type_check)
		{
			DebugErrorL("Mismatching root json type!\nFile: ", path);
			return false;
		}

		return true;
	}
#if defined(_DEBUG) || defined(DEBUG)
	catch (const simdjson::simdjson_error& v_err)
	{
		DebugErrorL("Couldn't parse: ", path, "\nError: ", v_err.what());
	}
#else
	catch (...) {}
#endif

	return false;
}

bool JsonReader::LoadParseSimdjsonComments(const std::wstring& path, simdjson::dom::document& v_doc)
{
	try
	{
		std::string v_json_str;
		if (!File::ReadToString(path, v_json_str))
			return false;

		JsonReader::RemoveComments(v_json_str);

		simdjson::dom::parser v_parser;
		v_parser.parse_into_document(v_doc, v_json_str);

		return true;
	}
#if defined(_DEBUG) || defined(DEBUG)
	catch (const simdjson::simdjson_error& v_err)
	{
		DebugErrorL("Couldn't parse: ", path, "\nError: ", v_err.what());
	}
#else
	catch (...) {}
#endif

	return false;
}

bool JsonReader::LoadParseSimdjsonCommentsC(const std::wstring& path, simdjson::dom::document& v_doc, const simdjson::dom::element_type& type_check)
{
	try
	{
		std::string v_json_str;
		if (!File::ReadToString(path, v_json_str))
			return false;

		JsonReader::RemoveComments(v_json_str);

		simdjson::dom::parser v_parser;
		v_parser.parse_into_document(v_doc, v_json_str);

		const auto v_root = v_doc.root();
		if (v_root.type() != type_check)
		{
			DebugErrorL("Mismatching root json type!\nFile: ", path);
			return false;
		}

		return true;
	}
#if defined(_DEBUG) || defined(DEBUG)
	catch (const simdjson::simdjson_error& v_err)
	{
		DebugErrorL("Couldn't parse: ", path, "\nError: ", v_err.what());
	}
#else
	catch (...) {}
#endif

	return false;
}

bool JsonReader::ParseSimdjsonString(const std::string& json_str, simdjson::dom::document& v_doc)
{
	try
	{
		simdjson::dom::parser v_parser;
		v_parser.parse_into_document(v_doc, json_str);

		return true;
	}
#if defined(_DEBUG) || defined(DEBUG)
	catch (const simdjson::simdjson_error& v_err)
	{
		DebugErrorL("Couldn't parse a json string. Error: ", v_err.what());
	}
#else
	catch (...) {}
#endif

	return false;
}
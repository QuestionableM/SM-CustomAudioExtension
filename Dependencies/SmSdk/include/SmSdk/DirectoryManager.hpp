#pragma once

#include "SmSdk/Util/Hashing.hpp"
#include "SmSdk/config.hpp"

#include <unordered_map>
#include <string>

class DirectoryManager
{
	SINGLETON_CLASS(DirectoryManager);

public:
	static DirectoryManager* GetInstance();

	// The replacement object is owned by the game, so be careful with using. You might want to copy it for long term storage
	bool getReplacement(const std::string_view& key, std::string_view& replacement);
	bool replacePathR(std::string& path);

	// The replacement object is owned by the game, so be careful with using. You might want to copy it for long term storage
	static bool GetReplacement(const std::string_view& key, std::string_view& replacement);
	static bool ReplacePathR(std::string& path);

private:
	char unk_data1[8];
public:
	std::unordered_map<std::string, std::string, Hashing::StringHash, std::equal_to<>> m_contentKeyToPathList;
};
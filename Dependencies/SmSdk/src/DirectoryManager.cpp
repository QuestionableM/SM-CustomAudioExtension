#include "SmSdk/DirectoryManager.hpp"

bool DirectoryManager::getReplacement(const std::string_view& key, std::string_view& replacement)
{
	auto v_iter = m_contentKeyToPathList.find(key);
	if (v_iter == m_contentKeyToPathList.end())
		return false;

	replacement = v_iter->second;
	return true;
}

bool DirectoryManager::replacePathR(std::string& path)
{
	if (path.empty() || path[0] != L'$')
		return false;

	const char* v_key_beg = path.data();
	const char* v_key_ptr = std::strchr(v_key_beg, L'/');
	if (v_key_ptr == nullptr) return false;

	const std::size_t v_keyIdx = v_key_ptr - v_key_beg;
	const std::string_view v_keyChunk = std::string_view(path).substr(0, v_keyIdx);

	const auto v_iter = m_contentKeyToPathList.find(v_keyChunk);
	if (v_iter == m_contentKeyToPathList.end()) return false;

	path.replace(
		path.begin(),
		path.begin() + v_keyIdx,
		v_iter->second
	);

	return true;
}

bool DirectoryManager::GetReplacement(const std::string_view& key, std::string_view& replacement)
{
	DirectoryManager* v_pDirMgr = DirectoryManager::GetInstance();
	if (!v_pDirMgr) return false;

	return v_pDirMgr->getReplacement(key, replacement);
}

bool DirectoryManager::ReplacePathR(std::string& path)
{
	DirectoryManager* v_pDirMgr = DirectoryManager::GetInstance();
	if (!v_pDirMgr) return false;

	return v_pDirMgr->replacePathR(path);
}
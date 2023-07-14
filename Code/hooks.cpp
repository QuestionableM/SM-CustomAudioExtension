#include "hooks.hpp"

#include "fmod_hooks.hpp"

#include "DirectoryManager.hpp"
#include "AudioManager.hpp"

#include "Utils/Console.hpp"
#include "Utils/String.hpp"
#include "Utils/File.hpp"
#include "Utils/Json.hpp"

#include <MinHook.h>

void replace_content_key_data(std::string& path, const std::string& key_repl)
{
	if (path.empty() || path[0] != '$')
		return;

	const std::size_t v_slash_idx = path.find('/');
	if (v_slash_idx == std::string::npos)
		return;

	const std::string v_chunk = path.substr(0, v_slash_idx);
	if (v_chunk != "$CONTENT_DATA")
		return;

	path = key_repl + path.substr(v_slash_idx);
}

void load_sound_config(const std::string& key_repl)
{
	const std::string config_path = key_repl + "/sm_dlm_config.json";
	if (!File::Exists(config_path))
		return;

	const std::wstring v_wide_path = String::ToWide(config_path);
	simdjson::dom::document v_document;
	if (!JsonReader::LoadParseSimdjsonCommentsC(v_wide_path, v_document, simdjson::dom::element_type::OBJECT))
	{
		DebugErrorL("[DLM] Couldn't load the DLM sound config file: ", config_path);
		return;
	}

	const auto v_root = v_document.root();
	const auto v_sound_list = v_root["soundList"];
	if (!v_sound_list.is_object())
	{
		DebugErrorL("[DLM] No sound list: ", config_path);
		return;
	}

	for (auto& v_sound_list : v_sound_list.get_object())
	{
		if (!v_sound_list.value.is_object())
			continue;

		const auto v_sound_path_node = v_sound_list.value["path"];
		const auto v_sound_is_3d_node = v_sound_list.value["is3D"];

		if (!v_sound_path_node.is_string())
			continue;

		const bool is_sound_3d = v_sound_is_3d_node.is_bool() ? v_sound_is_3d_node.get_bool().value() : false;
		const std::string v_sound_name(v_sound_list.key.data(), v_sound_list.key.size());

		std::string v_sound_path(v_sound_path_node.get_string().value().data());
		replace_content_key_data(v_sound_path, key_repl);

		SoundStorage::PreloadSound(v_sound_path, v_sound_name, is_sound_3d);
	}

	/*
	"soundList": {
		"CustomSoundName": {
			"path": "$CONTENT_DATA/CustomAudio/1_2_Oatmeal.mp3",
			"is3D": true
		},
		"CustomSoundName2": {
			"path": "$CONTENT_DATA/CustomAudio/1_2_Oatmeal.mp3",
			"is3D": false
		}
	}
	*/
}

bool separate_key(std::string& path)
{
	const std::size_t v_idx = path.find('/');
	if (v_idx == std::string::npos)
		return false;

	path = path.substr(0, v_idx);
	return true;
}

void preload_sounds(const std::string& shape_set_path)
{
	std::string v_key = shape_set_path;
	if (!separate_key(v_key))
	{
		DebugErrorL("Couldn't separate the key from: ", shape_set_path);
		return;
	}

	std::string v_replacement;
	if (!SM::DirectoryManager::GetReplacement(v_key, v_replacement))
	{
		DebugErrorL("Couldn't find a replacement for: ", shape_set_path);
		return;
	}

	load_sound_config(v_replacement);
}

void Hooks::h_LoadShapesetsFunction(void* shape_manager, const std::string& shape_set, int some_flag)
{
	preload_sounds(shape_set);
	return Hooks::o_LoadShapesetsFunction(shape_manager, shape_set, some_flag);
}

void Hooks::h_InitShapeManager(const char* file_data, unsigned int file_line)
{
	DebugOutL(__FUNCTION__, " -> Clearing sounds!");
	SoundStorage::ClearSounds();

	return Hooks::o_InitShapeManager(file_data, file_line);
}

void Hooks::RunHooks()
{
	const std::uintptr_t v_module_handle = std::uintptr_t(GetModuleHandle(NULL));
	const std::uintptr_t v_load_shapesets_addr = v_module_handle + 0x5C4250;
	const std::uintptr_t v_init_shape_manager_addr = v_module_handle + 0x378170;

	if (MH_CreateHook(
		(LPVOID)v_load_shapesets_addr,
		(LPVOID)Hooks::h_LoadShapesetsFunction,
		(LPVOID*)&Hooks::o_LoadShapesetsFunction) != MH_OK)
	{
		DebugErrorL("Couldn't hook the shapesets function!");
	}

	if (MH_CreateHook(
		(LPVOID)v_init_shape_manager_addr,
		(LPVOID)Hooks::h_InitShapeManager,
		(LPVOID*)&Hooks::o_InitShapeManager) != MH_OK)
	{
		DebugErrorL("Couldn't hoook the lua manager init function!");
	}
}
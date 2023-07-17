#include "hooks.hpp"

#include "fmod_hooks.hpp"

#include "SM/DirectoryManager.hpp"
#include "SM/AudioManager.hpp"

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

inline static std::unordered_map<std::string, int> g_reverbStringToIdx =
{
	{ "MOUNTAINS" , 0 },
	{ "CAVE"      , 1 },
	{ "GENERIC"   , 2 },
	{ "UNDERWATER", 3 }
};

int get_reverb_setting(const simdjson::dom::document_stream::iterator::value_type& v_reverb_data)
{
	if (!v_reverb_data.is_string())
		return -1;

	std::string_view v_reverb_string_view = v_reverb_data.get_string();
	std::string v_reverb_str(v_reverb_string_view.data(), v_reverb_string_view.size());

	auto v_iter = g_reverbStringToIdx.find(v_reverb_str);
	if (v_iter == g_reverbStringToIdx.end())
	{
		DebugErrorL("Invalid reverb preset name: ", v_reverb_str);
		return -1;
	}

	return v_iter->second;
}

void load_min_max_distance(const simdjson::dom::element& cur_sound, SoundEffectData& effect_data)
{
	const auto v_min_distance = cur_sound["min_distance"];
	const auto v_max_distance = cur_sound["max_distance"];

	effect_data.min_distance = v_min_distance.is_number() ? JsonReader::GetNumber<float>(v_min_distance) : 0.0f;
	effect_data.max_distance = v_max_distance.is_number() ? JsonReader::GetNumber<float>(v_max_distance) : 10000.0f;
}

void load_effect_data(const simdjson::dom::element& cur_sound, SoundEffectData& effect_data)
{
	const auto v_sound_is_3d_node = cur_sound["is3D"];
	const auto v_reverb_node = cur_sound["reverb"];

	effect_data.is_3d = v_sound_is_3d_node.is_bool() ? v_sound_is_3d_node.get_bool().value() : false;
	effect_data.reverb_idx = get_reverb_setting(v_reverb_node);

	load_min_max_distance(cur_sound, effect_data);
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
		DebugErrorL("Couldn't load the DLM sound config file: ", config_path);
		return;
	}

	const auto v_root = v_document.root();
	const auto v_sound_list = v_root["soundList"];
	if (!v_sound_list.is_object())
	{
		DebugErrorL("No sound list: ", config_path);
		return;
	}

	SoundEffectData v_effect_data;

	for (auto& v_sound_list : v_sound_list.get_object())
	{
		if (!v_sound_list.value.is_object())
			continue;

		const auto v_sound_path_node = v_sound_list.value["path"];
		if (!v_sound_path_node.is_string())
			continue;

		load_effect_data(v_sound_list.value, v_effect_data);

		std::string v_sound_path(v_sound_path_node.get_string().value().data());
		replace_content_key_data(v_sound_path, key_repl);

		const std::string v_sound_name(v_sound_list.key.data(), v_sound_list.key.size());
		SoundStorage::PreloadSound(v_sound_path, v_sound_name, v_effect_data);
	}
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
	FMODHooks::UpdateReverbProperties();

	return Hooks::o_InitShapeManager(file_data, file_line);
}


struct lua_State {};

struct LuaVM
{
	lua_State* state;
};

using luaL_loadstring_func = int(*)(lua_State* state, const char* s);
using lua_pcall_func = int(*)(lua_State* state, int nargs, int nresults, int errfunc);
static luaL_loadstring_func luaL_loadstring_ptr = nullptr;
static lua_pcall_func lua_pcall_ptr = nullptr;

int __fastcall Hooks::h_LuaInitFunc(LuaVM* lua_vm, void** some_ptr, int some_number)
{
	const int v_result = Hooks::o_LuaInitFunc(lua_vm, some_ptr, some_number);
	if (!v_result)
	{
		const int v_inject_result = luaL_loadstring_ptr(lua_vm->state, "unsafe_env.sm.dlm_injected = true");
		if (!v_inject_result)
			return lua_pcall_ptr(lua_vm->state, 0, -1, 0);

		return v_inject_result;
	}

	return v_result;
}

void Hooks::RunHooks()
{
	const std::uintptr_t v_module_handle = std::uintptr_t(GetModuleHandle(NULL));
	const std::uintptr_t v_load_shapesets_addr = v_module_handle + 0x5C4250;
	const std::uintptr_t v_init_shape_manager_addr = v_module_handle + 0x378170;
	const std::uintptr_t v_lua_init_addr = v_module_handle + 0x5784D0;

	HMODULE v_lua_dll = GetModuleHandleA("lua51.dll");
	if (v_lua_dll)
	{
		luaL_loadstring_ptr = (luaL_loadstring_func)GetProcAddress(v_lua_dll, "luaL_loadstring");
		lua_pcall_ptr = (lua_pcall_func)GetProcAddress(v_lua_dll, "lua_pcall");
	}

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

	if (MH_CreateHook(
		(LPVOID)v_lua_init_addr,
		(LPVOID)Hooks::h_LuaInitFunc,
		(LPVOID*)&Hooks::o_LuaInitFunc) != MH_OK)
	{
		DebugErrorL("Couldn't hook the lua init function!");
	}
}
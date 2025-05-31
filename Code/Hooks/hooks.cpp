#include "hooks.hpp"

#include "fmod_hooks.hpp"

#include <SmSdk/DirectoryManager.hpp>
#include <SmSdk/AudioManager.hpp>

#include "Utils/Console.hpp"
#include "Utils/String.hpp"
#include "Utils/File.hpp"
#include "Utils/Json.hpp"

#include "offsets.hpp"

#include <MinHook.h>

void replace_content_key_data(std::string& path, const std::string_view& keyRepl)
{
	if (path.empty() || path[0] != '$')
		return;

	const std::size_t v_slashIdx = path.find('/');
	if (v_slashIdx == std::string::npos)
		return;

	if (std::string_view(path).substr(0, v_slashIdx) != "$CONTENT_DATA")
		return;

	path.replace(
		path.begin(),
		path.begin() + v_slashIdx,
		keyRepl
	);
}

inline static std::unordered_map<std::string_view, int> g_reverbStringToIdx =
{
	{ "MOUNTAINS" , 0 },
	{ "CAVE"      , 1 },
	{ "GENERIC"   , 2 },
	{ "UNDERWATER", 3 }
};

int getReverbSetting(const simdjson::dom::document_stream::iterator::value_type& reverbData)
{
	if (!reverbData.is_string()) return -1;

	auto v_iter = g_reverbStringToIdx.find(reverbData.get_string());
	if (v_iter == g_reverbStringToIdx.end())
	{
		DebugErrorL("Invalid reverb preset name: ", reverbData.get_string().value_unsafe());
		return -1;
	}

	return v_iter->second;
}

void load_min_max_distance(const simdjson::dom::element& curSound, SoundEffectData& effectData)
{
	const auto v_minDistance = curSound["min_distance"];
	const auto v_maxDistance = curSound["max_distance"];

	effectData.fMinDistance = v_minDistance.is_number() ? JsonReader::GetNumber<float>(v_minDistance) : 0.0f;
	effectData.fMaxDistance = v_maxDistance.is_number() ? JsonReader::GetNumber<float>(v_maxDistance) : 10000.0f;
}

void load_effect_data(const simdjson::dom::element& curSound, SoundEffectData& effectData)
{
	const auto v_soundIs3dNode = curSound["is3D"];
	const auto v_reverbNode = curSound["reverb"];

	effectData.is3D = v_soundIs3dNode.is_bool() ? v_soundIs3dNode.get_bool().value() : false;
	effectData.reverbIdx = getReverbSetting(v_reverbNode);

	load_min_max_distance(curSound, effectData);
}

void load_sound_config(const std::string& keyRepl)
{
	std::string v_configPath = keyRepl + "/sm_cae_config.json";
	if (!File::Exists(v_configPath))
	{
		v_configPath = keyRepl + "/sm_dlm_config.json";
		if (!File::Exists(v_configPath))
			return;

		DebugWarningL(keyRepl, " is using a legacy version of CustomAudioExtension config");
	}

	simdjson::dom::document v_document;
	if (!JsonReader::LoadParseSimdjsonCommentsC(
		String::ToWide(v_configPath),
		v_document,
		simdjson::dom::element_type::OBJECT))
	{
		DebugErrorL("Couldn't load the CAE sound config file: ", v_configPath);
		return;
	}

	const auto v_soundList = v_document.root()["soundList"];
	if (!v_soundList.is_object())
	{
		DebugErrorL("No sound list: ", v_configPath);
		return;
	}

	SoundEffectData v_effectData;

	for (auto& v_soundListObj : v_soundList.get_object())
	{
		if (!v_soundListObj.value.is_object()) continue;

		const auto v_soundPathNode = v_soundListObj.value["path"];
		if (!v_soundPathNode.is_string()) continue;

		load_effect_data(v_soundListObj.value, v_effectData);

		std::string v_soundPath(v_soundPathNode.get_string().value().data());
		replace_content_key_data(v_soundPath, keyRepl);

		SoundStorage::PreloadSound(v_soundPath, v_soundListObj.key, v_effectData);
	}
}

bool separate_key(const std::string_view& path, std::string_view& outKey)
{
	const std::size_t v_idx = path.find('/');
	if (v_idx == std::string::npos) return false;

	outKey = path.substr(0, v_idx);
	return true;
}

void preload_sounds(const std::string& shapeSetPath)
{
	std::string_view v_key;
	if (!separate_key(shapeSetPath, v_key))
	{
		DebugErrorL("Couldn't separate the key from: ", shapeSetPath);
		return;
	}

	std::string_view v_replacement;
	if (!DirectoryManager::GetReplacement(v_key, v_replacement))
	{
		DebugErrorL("Couldn't find a replacement for: ", shapeSetPath);
		return;
	}

	load_sound_config(std::string(v_replacement));
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
		const int v_inject_result = luaL_loadstring_ptr(lua_vm->state, "unsafe_env.sm.cae_injected = true");
		if (!v_inject_result)
			return lua_pcall_ptr(lua_vm->state, 0, -1, 0);

		return v_inject_result;
	}

	return v_result;
}

void Hooks::RunHooks()
{
	const std::uintptr_t v_module_handle = std::uintptr_t(GetModuleHandle(NULL));
	const std::uintptr_t v_load_shapesets_addr = v_module_handle + OFF_LOAD_SHAPESETS_FUNCTION;
	const std::uintptr_t v_init_shape_manager_addr = v_module_handle + OFF_INIT_SHAPESET_MANAGER_FUNCTION;
	const std::uintptr_t v_lua_init_addr = v_module_handle + OFF_INIT_LUA_MANAGER_FUNCTION;

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
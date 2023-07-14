#pragma once

#include <string>

struct LuaVM;

class Hooks
{
public:
	using LoadShapesetsSig = void(__fastcall*)(void* shape_manager, const std::string& shape_set, int some_flag);
	using InitShapeManagerSig = void(__fastcall*)(const char* file_data, unsigned int file_line);
	using LuaInitializeFunc = int(__fastcall*)(LuaVM* lua_vm, void** some_ptr, int some_number);

	inline static LoadShapesetsSig o_LoadShapesetsFunction = nullptr;
	static void __fastcall h_LoadShapesetsFunction(void* shape_manager, const std::string& shape_set, int some_flag);

	inline static InitShapeManagerSig o_InitShapeManager = nullptr;
	static void __fastcall h_InitShapeManager(const char* file_data, unsigned int file_line);

	inline static LuaInitializeFunc o_LuaInitFunc = nullptr;
	static int __fastcall h_LuaInitFunc(LuaVM* lua_vm, void** some_ptr, int some_number);

	static void RunHooks();
};
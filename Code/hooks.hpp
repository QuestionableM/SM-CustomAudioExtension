#pragma once

#include <string>

class Hooks
{
public:
	using LoadShapesetsSig = void(__fastcall*)(void* shape_manager, const std::string& shape_set, int some_flag);
	using InitShapeManagerSig = void(__fastcall*)(const char* file_data, unsigned int file_line);

	inline static LoadShapesetsSig o_LoadShapesetsFunction = nullptr;
	static void __fastcall h_LoadShapesetsFunction(void* shape_manager, const std::string& shape_set, int some_flag);

	inline static InitShapeManagerSig o_InitShapeManager = nullptr;
	static void __fastcall h_InitShapeManager(const char* file_data, unsigned int file_line);

	static void RunHooks();
};
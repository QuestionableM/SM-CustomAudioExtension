#include "win_include.hpp"
#include <MinHook.h>

#include "fmod_hooks.hpp"
#include "hooks.hpp"

#include "Utils/Console.hpp"

void dll_initialize()
{
	AttachDebugConsole();

	if (MH_Initialize() == MH_OK)
	{
		FMODHooks::Hook();

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
			return;
		
		while (true)
		{
			Sleep(100);
		}

		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
}

void dll_entry_func(HMODULE module)
{
	Sleep(1500);
	dll_initialize();

	FreeLibraryAndExitThread(module, 0);
}

void dll_attach()
{
	AttachDebugConsole();
	DebugOutL("DLM: Attached");

	FMODHooks::Hook();

	MH_EnableHook(MH_ALL_HOOKS);
}

void dll_detach()
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)dll_entry_func, hModule, NULL, NULL);
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
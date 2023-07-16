#include "win_include.hpp"
#include <MinHook.h>

#include "Hooks/fmod_hooks.hpp"
#include "Hooks/hooks.hpp"

#include "Utils/Console.hpp"

void dll_initialize()
{
	if (MH_Initialize() == MH_OK)
	{
		FMODHooks::Hook();
		Hooks::RunHooks();

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
			return;
		
		while (true) { Sleep(100); }

		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
	}
}

void dll_entry_func(HMODULE module)
{
	Sleep(100);
	dll_initialize();

	FreeLibraryAndExitThread(module, 0);
}

static bool g_mhInitialized = false;
static bool g_mhAttached = false;

void dll_attach()
{
	if (MH_Initialize() == MH_OK)
	{
		g_mhInitialized = true;

		FMODHooks::Hook();
		Hooks::RunHooks();

		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
			g_mhAttached = true;
	}
}

void dll_detach()
{
	if (g_mhInitialized)
	{
		if (g_mhAttached)
			MH_DisableHook(MH_ALL_HOOKS);

		MH_Uninitialize();
	}
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)dll_entry_func, hModule, NULL, NULL);
		dll_attach();
		break;
	case DLL_PROCESS_DETACH:
		dll_detach();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
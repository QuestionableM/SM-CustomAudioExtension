#pragma once

#include "Hooks/offsets.hpp"
#include "win_include.hpp"

#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <string>
#include <map>

#include <fmod/fmod_studio.hpp>
#include <fmod/fmod.hpp>

namespace SM
{
	struct Task
	{
		virtual void func0();
	};

	struct AudioEvent {};
	
	struct AudioEventManager
	{
		/* 0x0000 */ std::vector<std::shared_ptr<AudioEvent>> event_list;
	}; // Size: 0x18

	struct AudioManager : public Task
	{
		/* 0x0008 */ std::shared_ptr<AudioEventManager> audio_event_mgr;
		/* 0x0018 */ char pad_0x18[0x50];
		/* 0x0068 */ FMOD::Studio::System* fmod_studio_system;
		/* 0x0070 */ FMOD::System* fmod_system;

		inline static AudioManager* GetInstance()
		{
			return *reinterpret_cast<AudioManager**>(std::uintptr_t(GetModuleHandle(NULL)) + OFF_AUDIO_MANAGER_PTR);
		}

	}; // Size: 0x2D8
}
#include "fmod_hooks.hpp"

#include "DirectoryManager.hpp"
#include "AudioManager.hpp"

#include "win_include.hpp"

#include "Utils/Console.hpp"
#include "Utils/File.hpp"

#include <MinHook.h>

#define FAKE_EVENT_CAST(event_name) reinterpret_cast<FakeEventDescription*>(event_name)


FMOD::Sound* SoundStorage::CreateSound(const std::string& path)
{
	const std::size_t hash = std::hash<std::string>{}(path);

	auto v_iter = SoundStorage::HashToSound.find(hash);
	if (v_iter != SoundStorage::HashToSound.end())
		return v_iter->second;

	SM::AudioManager* v_audio_mgr = SM::AudioManager::GetInstance();
	if (!v_audio_mgr) return nullptr;

	FMOD::Sound* v_custom_sound;
	if (v_audio_mgr->fmod_system->createSound(path.c_str(), FMOD_ACCURATETIME, nullptr, &v_custom_sound) != FMOD_OK)
		return nullptr;

	SoundStorage::HashToSound.emplace(hash, v_custom_sound);
	return v_custom_sound;
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_release(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->release();

	return FMODHooks::o_FMOD_Studio_EventInstance_release(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_start(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		bool is_playing = false;
		v_fake_event->channel->isPlaying(&is_playing);

		FMOD::ChannelGroup* v_channel_group = nullptr;
		v_fake_event->channel->getChannelGroup(&v_channel_group);

		if (!is_playing)
		{
			FMOD_RESULT v_result = SM::AudioManager::GetInstance()->fmod_system->playSound(
				v_fake_event->sound, nullptr, false, &v_fake_event->channel);

			if (v_result == FMOD_OK)
			{
				v_fake_event->channel->setMode(FMOD_3D);
				v_fake_event->channel->set3DConeSettings(75.0f, 360.0f, 0.1f);
			}

			return v_result;
		}

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_start(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_stop(FMOD::Studio::EventInstance* event_instance, FMOD_STUDIO_STOP_MODE mode)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		bool is_playing = false;
		v_fake_event->channel->isPlaying(&is_playing);

		if (is_playing)
			return v_fake_event->channel->stop();

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_stop(event_instance, mode);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_get3DAttributes(FMOD::Studio::EventInstance* event_instance, FMOD_3D_ATTRIBUTES* attributes)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		v_fake_event->channel->get3DConeOrientation(&attributes->forward);
		v_fake_event->channel->get3DAttributes(&attributes->position, &attributes->velocity);
		attributes->up = { 0.0f, 0.0f, 1.0f };

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_get3DAttributes(event_instance, attributes);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_set3DAttributes(FMOD::Studio::EventInstance* event_instance, const FMOD_3D_ATTRIBUTES* attributes)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		FMOD_VECTOR v_forward_cpy = attributes->forward;
		v_fake_event->channel->set3DConeOrientation(&v_forward_cpy);

		return v_fake_event->channel->set3DAttributes(&attributes->position, &attributes->velocity);
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_set3DAttributes(event_instance, attributes);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getVolume(FMOD::Studio::EventInstance* event_instance, float* volume, float* final_volume)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		FMOD_RESULT v_result = v_fake_event->channel->getVolume(volume);

		if (v_result == FMOD_OK && final_volume)
			*final_volume = *volume;

		return v_result;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getVolume(event_instance, volume, final_volume);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setVolume(FMOD::Studio::EventInstance* event_instance, float volume)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		v_fake_event->channel->setVolume(volume);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setVolume(event_instance, volume);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getDescription(FMOD::Studio::EventInstance* event_instance, FMOD::Studio::EventDescription** event_description)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		*event_description = reinterpret_cast<FMOD::Studio::EventDescription*>(v_fake_event);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getDescription(event_instance, event_description);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getPlaybackState(FMOD::Studio::EventInstance* event_instance, FMOD_STUDIO_PLAYBACK_STATE* state)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		bool v_is_playing = false;
		v_fake_event->channel->isPlaying(&v_is_playing);

		*state = v_is_playing ? FMOD_STUDIO_PLAYBACK_PLAYING : FMOD_STUDIO_PLAYBACK_STOPPED;
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getPlaybackState(event_instance, state);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getTimelinePosition(FMOD::Studio::EventInstance* event_instance, int* position)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->channel->getPosition(reinterpret_cast<unsigned int*>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_getTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setTimelinePosition(FMOD::Studio::EventInstance* event_instance, int position)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->channel->setPosition(static_cast<unsigned int>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_setTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getPitch(FMOD::Studio::EventInstance* event_instance, float* pitch, float* finalpitch)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		FMOD_RESULT v_result = v_fake_event->channel->getPitch(pitch);

		if (v_result == FMOD_OK && finalpitch)
			*finalpitch = *pitch;

		return v_result;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getPitch(event_instance, pitch, finalpitch);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setPitch(FMOD::Studio::EventInstance* event_instance, float pitch)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		v_fake_event->channel->setPitch(pitch);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setPitch(event_instance, pitch);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_getLength(FMOD::Studio::EventDescription* event_desc, int* length)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_desc);
	if (v_fake_event->isValidHook())
		return v_fake_event->sound->getLength(reinterpret_cast<unsigned int*>(length), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventDescription_getLength(event_desc, length);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_createInstance(FMOD::Studio::EventDescription* event_desc, FMOD::Studio::EventInstance** instance)
{
	std::string v_sound_path;
	if (SoundStorage::GetPath(reinterpret_cast<std::size_t>(event_desc), v_sound_path))
	{
		SM::AudioManager* v_audio_mgr = SM::AudioManager::GetInstance();
		if (v_audio_mgr)
		{
			FMOD::Sound* v_sound = SoundStorage::CreateSound(v_sound_path);
			if (v_sound)
			{
				FMOD::Channel* v_channel;
				if (v_audio_mgr->fmod_system->playSound(v_sound, nullptr, false, &v_channel) == FMOD_OK)
				{
					v_channel->setMode(FMOD_3D);

					FakeEventDescription* v_new_fake_event = new FakeEventDescription(v_sound, v_channel);
					*instance = reinterpret_cast<FMOD::Studio::EventInstance*>(v_new_fake_event);

					return FMOD_OK;
				}
			}
		}
	}

	return FMODHooks::o_FMOD_Studio_EventDescription_createInstance(event_desc, instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_hasSustainPoint(FMOD::Studio::EventDescription* event_desc, bool* has_sustain)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_desc);
	if (v_fake_event->isValidHook())
	{
		*has_sustain = false;
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventDescription_hasSustainPoint(event_desc, has_sustain);
}

#define FMOD_HOOK_FAKE_GUID_SECRET 0xf0f0f0f0f0f0f0f0

union FAKE_GUID_DATA
{
	FMOD_GUID guid;
	struct
	{
		std::size_t v_hash;
		std::size_t v_secret;
	} fake;
};

FMOD_RESULT FMODHooks::h_FMOD_Studio_System_lookupID(FMOD::Studio::System* system, const char* path, FMOD_GUID* id)
{
	std::string v_replaced_path = path;
	SM::DirectoryManager::ReplacePathR(v_replaced_path);

	if (File::Exists(v_replaced_path))
	{
		FAKE_GUID_DATA* v_fake_guid = reinterpret_cast<FAKE_GUID_DATA*>(id);

		v_fake_guid->fake.v_hash = SoundStorage::SavePath(v_replaced_path);
		v_fake_guid->fake.v_secret = FMOD_HOOK_FAKE_GUID_SECRET;

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_System_lookupID(system, path, id);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_System_getEventByID(FMOD::Studio::System* system, const FMOD_GUID* id, FMOD::Studio::EventDescription** event_id)
{
	const FAKE_GUID_DATA* v_guid_data = reinterpret_cast<const FAKE_GUID_DATA*>(id);
	if (v_guid_data->fake.v_secret == FMOD_HOOK_FAKE_GUID_SECRET)
	{
		if (SoundStorage::IsHashValid(v_guid_data->fake.v_hash))
		{
			*event_id = reinterpret_cast<FMOD::Studio::EventDescription*>(v_guid_data->fake.v_hash);
			return FMOD_OK;
		}
	}

	return FMODHooks::o_FMOD_Studio_System_getEventByID(system, id, event_id);
}

struct FMODHookData
{
	const char* proc_name;
	LPVOID detour;
	LPVOID* original;
};

static FMODHookData g_fmodHookData[] =
{
	{
		"?release@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_release,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_release
	},
	{
		"?start@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_start,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_start
	},
	{
		"?stop@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@W4FMOD_STUDIO_STOP_MODE@@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_stop,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_stop
	},
	{
		"?get3DAttributes@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAUFMOD_3D_ATTRIBUTES@@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_get3DAttributes,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_get3DAttributes
	},
	{
		"?set3DAttributes@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@PEBUFMOD_3D_ATTRIBUTES@@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_set3DAttributes,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_set3DAttributes
	},
	{
		"?getVolume@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAM0@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_getVolume,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_getVolume
	},
	{
		"?setVolume@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_setVolume,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_setVolume
	},
	{
		"?getDescription@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAPEAVEventDescription@23@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_getDescription,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_getDescription
	},
	{
		"?getLength@EventDescription@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAH@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventDescription_getLength,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventDescription_getLength
	},
	{
		"?getPlaybackState@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAW4FMOD_STUDIO_PLAYBACK_STATE@@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_getPlaybackState,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_getPlaybackState
	},
	{
		"?getTimelinePosition@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAH@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_getTimelinePosition,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_getTimelinePosition
	},
	{
		"?setTimelinePosition@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@H@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_setTimelinePosition,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_setTimelinePosition
	},
	{
		"?getPitch@EventInstance@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAM0@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_getPitch,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_getPitch
	},
	{
		"?setPitch@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_setPitch,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_setPitch
	},
	{
		"?lookupID@System@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEBDPEAUFMOD_GUID@@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_System_lookupID,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_System_lookupID
	},
	{
		"?getEventByID@System@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEBUFMOD_GUID@@PEAPEAVEventDescription@23@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_System_getEventByID,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_System_getEventByID
	},
	{
		"?createInstance@EventDescription@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEAPEAVEventInstance@23@@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventDescription_createInstance,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventDescription_createInstance
	},
	{
		"?hasSustainPoint@EventDescription@Studio@FMOD@@QEBA?AW4FMOD_RESULT@@PEA_N@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventDescription_hasSustainPoint,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventDescription_hasSustainPoint
	}
};

void FMODHooks::Hook()
{
	HMODULE v_fmod_studio = GetModuleHandleA("fmodstudio.dll");
	if (!v_fmod_studio)
	{
		DebugErrorL("Couldn't get fmodstudio.dll module");
		return;
	}

	constexpr std::size_t v_hook_count = sizeof(g_fmodHookData) / sizeof(FMODHookData);
	for (std::size_t a = 0; a < v_hook_count; a++)
	{
		FMODHookData& v_cur_hook = g_fmodHookData[a];

		if (MH_CreateHook(
			GetProcAddress(v_fmod_studio, v_cur_hook.proc_name),
			v_cur_hook.detour,
			v_cur_hook.original) != MH_OK)
		{
			DebugErrorL("Couldn't hook the specified function: ", v_cur_hook.proc_name);
			return;
		}
	}

	DebugOutL("Successfully hooked all FMOD functions!");
}
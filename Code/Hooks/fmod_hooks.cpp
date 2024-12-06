#include "fmod_hooks.hpp"

#include <SmSdk/DirectoryManager.hpp>
#include <SmSdk/AudioManager.hpp>
#include <SmSdk/GameSettings.hpp>
#include <SmSdk/win_include.hpp>

#include "Utils/Console.hpp"
#include "Utils/File.hpp"

#include <MinHook.h>

#define FAKE_EVENT_CAST(event_name) reinterpret_cast<FakeEventDescription*>(event_name)

FMOD_RESULT FakeEventDescription::setVolume(float new_volume)
{
	this->custom_volume = new_volume;
	return this->updateVolume();
}

FMOD_RESULT FakeEventDescription::setPosition(float new_position)
{
	return this->channel->setPosition(static_cast<unsigned int>(new_position * 1000.0f), FMOD_TIMEUNIT_MS);
}

FMOD_RESULT FakeEventDescription::updateVolume()
{
	return this->channel->setVolume(this->custom_volume * GameSettings::GetEffectsVolume());
}

void FakeEventDescription::updateReverbData()
{
	for (int a = 0; a < 4; a++)
		this->channel->setReverbProperties(a, (this->reverb_idx == a) ? 1.0f : 0.0f);
}

void FakeEventDescription::playSound()
{
	AudioManager* v_pAudioMgr = AudioManager::GetInstance();
	if (!v_pAudioMgr || this->isPlaying()) return;

	if (v_pAudioMgr->fmod_system->playSound(this->sound, nullptr, true, &this->channel) != FMOD_OK)
		return;

	this->channel->setVolume(GameSettings::GetEffectsVolume());
	this->channel->set3DMinMaxDistance(this->min_distance, this->max_distance);
	this->channel->set3DDistanceFilter(false, 1.0f, 10000.0f);

	if (this->is_3d)
		this->channel->setMode(FMOD_3D);

	this->updateReverbData();
}

FMOD::Sound* SoundStorage::CreateSound(const std::string& path)
{
	const std::size_t hash = std::hash<std::string>{}(path);

	auto v_iter = SoundStorage::PathHashToSound.find(hash);
	if (v_iter != SoundStorage::PathHashToSound.end())
		return v_iter->second;

	AudioManager* v_pAudioMgr = AudioManager::GetInstance();
	if (!v_pAudioMgr)
	{
		DebugErrorL("AudioManager is not initialized!");
		return nullptr;
	}

	FMOD::Sound* v_custom_sound;
	if (v_pAudioMgr->fmod_system->createSound(path.c_str(), FMOD_ACCURATETIME, nullptr, &v_custom_sound) != FMOD_OK)
	{
		DebugErrorL("Couldn't load the specified sound file: ", path);
		return nullptr;
	}

	DebugOutL(__FUNCTION__, " -> Loaded a sound: ", path);
	SoundStorage::PathHashToSound.emplace(hash, v_custom_sound);
	return v_custom_sound;
}

void SoundStorage::PreloadSound(const std::string& sound_path, const std::string& sound_name, const SoundEffectData& effect_data)
{
	FMOD::Sound* v_sound = SoundStorage::CreateSound(sound_path);
	if (!v_sound) return;
	
	const std::size_t v_name_hash = std::hash<std::string>{}(sound_name);

	auto v_name_iter = SoundStorage::NameHashToSound.find(v_name_hash);
	if (v_name_iter != SoundStorage::NameHashToSound.end())
	{
		DebugWarningL("The specified sound name is already occupied! (", sound_name, ")");
		return;
	}

	SoundStorage::NameHashToSound.emplace(v_name_hash, SoundData{
		.sound = v_sound,
		.effect_data = effect_data
	});
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_release(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->decodePointer()->release();

	return FMODHooks::o_FMOD_Studio_EventInstance_release(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_start(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		v_fake_event->decodePointer()->channel->setPaused(false);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_start(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_stop(FMOD::Studio::EventInstance* event_instance, FMOD_STUDIO_STOP_MODE mode)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		v_fake_event = v_fake_event->decodePointer();

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
		v_fake_event = v_fake_event->decodePointer();

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
		v_fake_event = v_fake_event->decodePointer();

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
		FMOD_RESULT v_result = v_fake_event->decodePointer()->channel->getVolume(volume);
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
		v_fake_event->decodePointer()->updateVolume();
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
		v_fake_event->decodePointer()->channel->isPlaying(&v_is_playing);

		*state = v_is_playing ? FMOD_STUDIO_PLAYBACK_PLAYING : FMOD_STUDIO_PLAYBACK_STOPPED;
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getPlaybackState(event_instance, state);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getTimelinePosition(FMOD::Studio::EventInstance* event_instance, int* position)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->decodePointer()->channel->getPosition(reinterpret_cast<unsigned int*>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_getTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setTimelinePosition(FMOD::Studio::EventInstance* event_instance, int position)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
		return v_fake_event->decodePointer()->channel->setPosition(static_cast<unsigned int>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_setTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getPitch(FMOD::Studio::EventInstance* event_instance, float* pitch, float* finalpitch)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		FMOD_RESULT v_result = v_fake_event->decodePointer()->channel->getPitch(pitch);
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
		v_fake_event->decodePointer()->channel->setPitch(pitch);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setPitch(event_instance, pitch);
}

using v_fmod_set_parameter_function = FMOD_RESULT(*)(FakeEventDescription* fake_event, float value);

static FMOD_RESULT fake_event_desc_setPitch(FakeEventDescription* fake_event, float value)
{
	fake_event->channel->setPitch(value);
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setVolume(FakeEventDescription* fake_event, float volume)
{
	fake_event->setVolume(volume);
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setReverb(FakeEventDescription* fake_event, float reverb)
{
	if (fake_event->reverb_idx != -1)
		fake_event->channel->setReverbProperties(fake_event->reverb_idx, reverb);

	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setReverbIndex(FakeEventDescription* fake_event, float reverb_idx)
{
	const int v_reverb_idx = static_cast<int>(reverb_idx);
	if (v_reverb_idx >= 0 && v_reverb_idx <= 3)
		fake_event->reverb_idx = v_reverb_idx;
	else
		fake_event->reverb_idx = -1;

	fake_event->updateReverbData();
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setPosition(FakeEventDescription* fake_event, float position)
{
	fake_event->setPosition(position);
	return FMOD_OK;
}

inline static std::unordered_map<std::string, v_fmod_set_parameter_function> g_fake_event_parameter_table =
{
	//Legacy audio parameters
	{ "DLM_Pitch"    , fake_event_desc_setPitch       },
	{ "DLM_Volume"   , fake_event_desc_setVolume      },
	{ "DLM_Reverb"   , fake_event_desc_setReverb      },
	{ "DLM_ReverbIdx", fake_event_desc_setReverbIndex },

	//New audio parameters
	{ "CAE_Pitch"    , fake_event_desc_setPitch       },
	{ "CAE_Volume"   , fake_event_desc_setVolume      },
	{ "CAE_Reverb"   , fake_event_desc_setReverb      },
	{ "CAE_ReverbIdx", fake_event_desc_setReverbIndex },
	{ "CAE_Position" , fake_event_desc_setPosition    }
};

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setParameterByName(FMOD::Studio::EventInstance* event_instance, const char* name, float value, bool ignoreseekspeed)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_instance);
	if (v_fake_event->isValidHook())
	{
		auto v_iter = g_fake_event_parameter_table.find(std::string(name));
		if (v_iter != g_fake_event_parameter_table.end())
			return v_iter->second(v_fake_event->decodePointer(), value);
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setParameterByName(event_instance, name, value, ignoreseekspeed);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_getLength(FMOD::Studio::EventDescription* event_desc, int* length)
{
	FakeEventDescription* v_fake_event = FAKE_EVENT_CAST(event_desc);
	if (v_fake_event->isValidHook())
		return v_fake_event->decodePointer()->sound->getLength(reinterpret_cast<unsigned int*>(length), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventDescription_getLength(event_desc, length);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_createInstance(FMOD::Studio::EventDescription* event_desc, FMOD::Studio::EventInstance** instance)
{
	SoundData* v_sound_data = SoundStorage::GetSoundData(reinterpret_cast<std::size_t>(event_desc));
	if (v_sound_data)
	{
		FakeEventDescription* v_new_fake_event = new FakeEventDescription(v_sound_data, nullptr);
		v_new_fake_event->playSound();

		*instance = reinterpret_cast<FMOD::Studio::EventInstance*>(v_new_fake_event->encodePointer());
		
		return FMOD_OK;
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
	const std::size_t sound_hash = std::hash<std::string>{}(std::string(path));
	if (SoundStorage::SoundExists(sound_hash))
	{
		FAKE_GUID_DATA* v_fake_guid = reinterpret_cast<FAKE_GUID_DATA*>(id);

		v_fake_guid->fake.v_hash = sound_hash;
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
		if (SoundStorage::SoundExists(v_guid_data->fake.v_hash))
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
	},
	{
		"?setParameterByName@EventInstance@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDM_N@Z",
		(LPVOID)FMODHooks::h_FMOD_Studio_EventInstance_setParameterByName,
		(LPVOID*)&FMODHooks::o_FMOD_Studio_EventInstance_setParameterByName
	}
};

void FMODHooks::UpdateReverbProperties()
{
	AudioManager* v_pAudioMgr = AudioManager::GetInstance();
	if (!v_pAudioMgr) return;

	DebugOutL("Injecting reverb properties");

	FMOD_REVERB_PROPERTIES v_reverb_preset = FMOD_PRESET_MOUNTAINS;
	v_pAudioMgr->fmod_system->setReverbProperties(0, &v_reverb_preset);
	FMOD_REVERB_PROPERTIES v_reverb_preset2 = FMOD_PRESET_CAVE;
	v_pAudioMgr->fmod_system->setReverbProperties(1, &v_reverb_preset2);
	FMOD_REVERB_PROPERTIES v_reverb_preset3 = FMOD_PRESET_GENERIC;
	v_pAudioMgr->fmod_system->setReverbProperties(2, &v_reverb_preset3);
	FMOD_REVERB_PROPERTIES v_reverb_preset4 = FMOD_PRESET_UNDERWATER;
	v_pAudioMgr->fmod_system->setReverbProperties(3, &v_reverb_preset4);
}

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
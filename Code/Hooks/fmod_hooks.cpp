#include "fmod_hooks.hpp"

#include <SmSdk/DirectoryManager.hpp>
#include <SmSdk/AudioManager.hpp>
#include <SmSdk/GameSettings.hpp>
#include <SmSdk/win_include.hpp>

#include "Utils/Console.hpp"
#include "Utils/File.hpp"

#include <MinHook.h>

#define FAKE_EVENT_CAST(event_name) reinterpret_cast<FakeEventDescription*>(event_name)

FakeEventDescription::FakeEventDescription(
	const SoundData* pSoundData,
	FMOD::Channel* pChannel
) :
	m_pSound(pSoundData->sound),
	m_pChannel(pChannel),
	m_fCustomVolume(1.0f),
	m_reverbIdx(pSoundData->effectData.reverbIdx),
	m_fMinDistance(pSoundData->effectData.fMinDistance),
	m_fMaxDistance(pSoundData->effectData.fMaxDistance),
	m_is3D(pSoundData->effectData.is3D)
{}

FMOD_RESULT FakeEventDescription::setVolume(float newVolume)
{
	m_fCustomVolume = newVolume;
	return this->updateVolume();
}

FMOD_RESULT FakeEventDescription::setPosition(float newPosition)
{
	return m_pChannel->setPosition(static_cast<std::uint32_t>(newPosition * 1000.0f), FMOD_TIMEUNIT_MS);
}

FMOD_RESULT FakeEventDescription::updateVolume()
{
	return m_pChannel->setVolume(m_fCustomVolume * GameSettings::GetEffectsVolume());
}

void FakeEventDescription::updateReverbData()
{
	for (int a = 0; a < 4; a++)
		m_pChannel->setReverbProperties(a, (m_reverbIdx == a) ? 1.0f : 0.0f);
}

void FakeEventDescription::playSound()
{
	AudioManager* v_pAudioMgr = AudioManager::GetInstance();
	if (!v_pAudioMgr || this->isPlaying()) return;

	if (v_pAudioMgr->fmod_system->playSound(m_pSound, nullptr, true, &m_pChannel) != FMOD_OK)
		return;

	m_pChannel->setVolume(GameSettings::GetEffectsVolume());
	m_pChannel->set3DMinMaxDistance(m_fMinDistance, m_fMaxDistance);
	m_pChannel->set3DDistanceFilter(false, 1.0f, 10000.0f);

	if (m_is3D)
		m_pChannel->setMode(FMOD_3D);

	this->updateReverbData();
}

bool FakeEventDescription::isPlaying() const
{
	if (!m_pChannel) return false;

	bool v_isPlaying = false;
	m_pChannel->isPlaying(&v_isPlaying);

	return v_isPlaying;
}

bool FakeEventDescription::isValidHook() const noexcept
{
	return (reinterpret_cast<std::uintptr_t>(this) & (1ull << 63));
}

FakeEventDescription* FakeEventDescription::encodePointer() noexcept
{
	const std::uintptr_t v_encodedPtr = reinterpret_cast<std::uintptr_t>(this) | (1ULL << 63);
	return reinterpret_cast<FakeEventDescription*>(v_encodedPtr);
}

FakeEventDescription* FakeEventDescription::decodePointer() noexcept
{
	const std::uintptr_t v_decodedPtr = reinterpret_cast<std::uintptr_t>(this) & ~(1ULL << 63);
	return reinterpret_cast<FakeEventDescription*>(v_decodedPtr);
}

FMOD_RESULT FakeEventDescription::release()
{
	delete this;
	return FMOD_OK;/*return m_pSound->release();*/
}

///////////////// SOUND STORAGE ////////////////////

void SoundStorage::ClearSounds()
{
	for (auto& [v_soundHash, v_pSound] : SoundStorage::PathHashToSound)
		v_pSound->release();

	SoundStorage::PathHashToSound.clear();
	SoundStorage::NameHashToSound.clear();

	SoundStorage::HashToPath.clear();
}

bool SoundStorage::SoundExists(const std::size_t nameHash)
{
	return SoundStorage::NameHashToSound.contains(nameHash);
}

SoundData* SoundStorage::GetSoundData(const std::size_t nameHash)
{
	auto v_iter = SoundStorage::NameHashToSound.find(nameHash);
	if (v_iter != SoundStorage::NameHashToSound.end())
		return &v_iter->second;
	else
		return nullptr;
}

std::size_t SoundStorage::SavePath(const std::string_view& path)
{
	const std::size_t v_stringHash = std::hash<std::string_view>{}(path);

	auto v_iter = SoundStorage::HashToPath.find(v_stringHash);
	if (v_iter != SoundStorage::HashToPath.end())
		return v_stringHash;

	SoundStorage::HashToPath.emplace(v_stringHash, path);
	return v_stringHash;
}

bool SoundStorage::GetPath(const std::size_t hash, std::string& outPath)
{
	auto v_iter = SoundStorage::HashToPath.find(hash);
	if (v_iter == SoundStorage::HashToPath.end())
		return false;

	outPath = v_iter->second;
	return true;
}

FMOD::Sound* SoundStorage::CreateSound(const std::string_view& path)
{
	const std::size_t v_hash = std::hash<std::string_view>{}(path);

	auto v_iter = SoundStorage::PathHashToSound.find(v_hash);
	if (v_iter != SoundStorage::PathHashToSound.end())
		return v_iter->second;

	AudioManager* v_pAudioMgr = AudioManager::GetInstance();
	if (!v_pAudioMgr)
	{
		DebugErrorL("AudioManager is not initialized!");
		return nullptr;
	}

	FMOD::Sound* v_pCustomSound;
	if (v_pAudioMgr->fmod_system->createSound(path.data(), FMOD_ACCURATETIME | FMOD_NONBLOCKING, nullptr, &v_pCustomSound) != FMOD_OK)
	{
		DebugErrorL("Couldn't load the specified sound file: ", path);
		return nullptr;
	}

	DebugOutL(__FUNCTION__, " -> Loaded a sound: ", path);
	SoundStorage::PathHashToSound.emplace(v_hash, v_pCustomSound);
	return v_pCustomSound;
}

void SoundStorage::PreloadSound(
	const std::string_view& sound_path,
	const std::string_view& sound_name,
	const SoundEffectData& effect_data)
{
	FMOD::Sound* v_pSound = SoundStorage::CreateSound(sound_path);
	if (!v_pSound) return;
	
	const std::size_t v_nameHash = std::hash<std::string_view>{}(sound_name);

	auto v_name_iter = SoundStorage::NameHashToSound.find(v_nameHash);
	if (v_name_iter != SoundStorage::NameHashToSound.end())
	{
		DebugWarningL("The specified sound name is already occupied! (", sound_name, ")");
		return;
	}

	SoundStorage::NameHashToSound.emplace(v_nameHash, SoundData{
		.sound = v_pSound,
		.effectData = effect_data
	});
}

///////////////////// FMOD HOOKS ///////////////////

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_release(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
		return v_pFakeEvent->decodePointer()->release();

	return FMODHooks::o_FMOD_Studio_EventInstance_release(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_start(FMOD::Studio::EventInstance* event_instance)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent->decodePointer()->m_pChannel->setPaused(false);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_start(event_instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_stop(
	FMOD::Studio::EventInstance* event_instance,
	FMOD_STUDIO_STOP_MODE mode)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent = v_pFakeEvent->decodePointer();

		bool v_isPlaying = false;
		v_pFakeEvent->m_pChannel->isPlaying(&v_isPlaying);

		if (v_isPlaying)
			return v_pFakeEvent->m_pChannel->stop();

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_stop(event_instance, mode);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_get3DAttributes(
	FMOD::Studio::EventInstance* event_instance,
	FMOD_3D_ATTRIBUTES* attributes)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent = v_pFakeEvent->decodePointer();

		v_pFakeEvent->m_pChannel->get3DConeOrientation(&attributes->forward);
		v_pFakeEvent->m_pChannel->get3DAttributes(&attributes->position, &attributes->velocity);
		attributes->up = { 0.0f, 0.0f, 1.0f };

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_get3DAttributes(event_instance, attributes);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_set3DAttributes(
	FMOD::Studio::EventInstance* event_instance,
	const FMOD_3D_ATTRIBUTES* attributes)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent = v_pFakeEvent->decodePointer();

		FMOD_VECTOR v_forward_cpy = attributes->forward;
		v_pFakeEvent->m_pChannel->set3DConeOrientation(&v_forward_cpy);

		return v_pFakeEvent->m_pChannel->set3DAttributes(&attributes->position, &attributes->velocity);
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_set3DAttributes(event_instance, attributes);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getVolume(
	FMOD::Studio::EventInstance* event_instance,
	float* volume,
	float* final_volume)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		FMOD_RESULT v_result = v_pFakeEvent->decodePointer()->m_pChannel->getVolume(volume);
		if (v_result == FMOD_OK && final_volume)
			*final_volume = *volume;

		return v_result;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getVolume(event_instance, volume, final_volume);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setVolume(
	FMOD::Studio::EventInstance* event_instance,
	float volume)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent->decodePointer()->updateVolume();
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setVolume(event_instance, volume);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getDescription(
	FMOD::Studio::EventInstance* event_instance,
	FMOD::Studio::EventDescription** event_description)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		*event_description = reinterpret_cast<FMOD::Studio::EventDescription*>(v_pFakeEvent);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getDescription(event_instance, event_description);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getPlaybackState(
	FMOD::Studio::EventInstance* event_instance,
	FMOD_STUDIO_PLAYBACK_STATE* state)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		const bool v_isPlaying = v_pFakeEvent->decodePointer()->isPlaying();

		*state = v_isPlaying ? FMOD_STUDIO_PLAYBACK_PLAYING : FMOD_STUDIO_PLAYBACK_STOPPED;
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getPlaybackState(event_instance, state);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getTimelinePosition(
	FMOD::Studio::EventInstance* event_instance,
	int* position)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
		return v_pFakeEvent->decodePointer()->m_pChannel->getPosition(reinterpret_cast<std::uint32_t*>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_getTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setTimelinePosition(
	FMOD::Studio::EventInstance* event_instance,
	int position)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
		return v_pFakeEvent->decodePointer()->m_pChannel->setPosition(static_cast<std::uint32_t>(position), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventInstance_setTimelinePosition(event_instance, position);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_getPitch(
	FMOD::Studio::EventInstance* event_instance,
	float* pitch,
	float* finalpitch)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		FMOD_RESULT v_result = v_pFakeEvent->decodePointer()->m_pChannel->getPitch(pitch);
		if (v_result == FMOD_OK && finalpitch)
			*finalpitch = *pitch;

		return v_result;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_getPitch(event_instance, pitch, finalpitch);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setPitch(
	FMOD::Studio::EventInstance* event_instance,
	float pitch)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		v_pFakeEvent->decodePointer()->m_pChannel->setPitch(pitch);
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setPitch(event_instance, pitch);
}

using v_fmod_set_parameter_function = FMOD_RESULT(*)(FakeEventDescription* fake_event, float value);

static FMOD_RESULT fake_event_desc_setPitch(FakeEventDescription* fake_event, float value)
{
	fake_event->m_pChannel->setPitch(value);
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setVolume(FakeEventDescription* fake_event, float volume)
{
	fake_event->setVolume(volume);
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setReverb(FakeEventDescription* fake_event, float reverb)
{
	if (fake_event->m_reverbIdx != -1)
		fake_event->m_pChannel->setReverbProperties(fake_event->m_reverbIdx, reverb);

	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setReverbIndex(FakeEventDescription* fake_event, float reverb_idx)
{
	const int v_reverbIdx = static_cast<int>(reverb_idx);
	if (v_reverbIdx >= 0 && v_reverbIdx <= 3)
		fake_event->m_reverbIdx = v_reverbIdx;
	else
		fake_event->m_reverbIdx = -1;

	fake_event->updateReverbData();
	return FMOD_OK;
}

static FMOD_RESULT fake_event_desc_setPosition(FakeEventDescription* fake_event, float position)
{
	fake_event->setPosition(position);
	return FMOD_OK;
}

inline static std::unordered_map<std::string_view, v_fmod_set_parameter_function> g_fakeEventParameterTable =
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

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventInstance_setParameterByName(
	FMOD::Studio::EventInstance* event_instance,
	const char* name,
	float value,
	bool ignoreseekspeed)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_instance);
	if (v_pFakeEvent->isValidHook())
	{
		auto v_iter = g_fakeEventParameterTable.find(std::string_view(name));
		if (v_iter != g_fakeEventParameterTable.end())
			return v_iter->second(v_pFakeEvent->decodePointer(), value);
	}

	return FMODHooks::o_FMOD_Studio_EventInstance_setParameterByName(event_instance, name, value, ignoreseekspeed);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_getLength(
	FMOD::Studio::EventDescription* event_desc,
	int* length)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_desc);
	if (v_pFakeEvent->isValidHook())
		return v_pFakeEvent->decodePointer()->m_pSound->getLength(reinterpret_cast<std::uint32_t*>(length), FMOD_TIMEUNIT_MS);

	return FMODHooks::o_FMOD_Studio_EventDescription_getLength(event_desc, length);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_createInstance(
	FMOD::Studio::EventDescription* event_desc,
	FMOD::Studio::EventInstance** instance)
{
	SoundData* v_pSoundData = SoundStorage::GetSoundData(reinterpret_cast<std::size_t>(event_desc));
	if (v_pSoundData)
	{
		FakeEventDescription* v_newFakeEvent = new FakeEventDescription(v_pSoundData, nullptr);
		v_newFakeEvent->playSound();

		*instance = reinterpret_cast<FMOD::Studio::EventInstance*>(v_newFakeEvent->encodePointer());
		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_EventDescription_createInstance(event_desc, instance);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_EventDescription_hasSustainPoint(
	FMOD::Studio::EventDescription* event_desc,
	bool* has_sustain)
{
	FakeEventDescription* v_pFakeEvent = FAKE_EVENT_CAST(event_desc);
	if (v_pFakeEvent->isValidHook())
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
		std::size_t hash;
		std::size_t secret;
	} fake;
};

FMOD_RESULT FMODHooks::h_FMOD_Studio_System_lookupID(
	FMOD::Studio::System* system,
	const char* path,
	FMOD_GUID* id)
{
	const std::size_t sound_hash = std::hash<std::string>{}(std::string(path));
	if (SoundStorage::SoundExists(sound_hash))
	{
		FAKE_GUID_DATA* v_fake_guid = reinterpret_cast<FAKE_GUID_DATA*>(id);

		v_fake_guid->fake.hash = sound_hash;
		v_fake_guid->fake.secret = FMOD_HOOK_FAKE_GUID_SECRET;

		return FMOD_OK;
	}

	return FMODHooks::o_FMOD_Studio_System_lookupID(system, path, id);
}

FMOD_RESULT FMODHooks::h_FMOD_Studio_System_getEventByID(
	FMOD::Studio::System* system,
	const FMOD_GUID* id,
	FMOD::Studio::EventDescription** event_id)
{
	const FAKE_GUID_DATA* v_guid_data = reinterpret_cast<const FAKE_GUID_DATA*>(id);
	if (v_guid_data->fake.secret == FMOD_HOOK_FAKE_GUID_SECRET)
	{
		if (SoundStorage::SoundExists(v_guid_data->fake.hash))
		{
			*event_id = reinterpret_cast<FMOD::Studio::EventDescription*>(v_guid_data->fake.hash);
			return FMOD_OK;
		}
	}

	return FMODHooks::o_FMOD_Studio_System_getEventByID(system, id, event_id);
}

struct FMODHookData
{
	const char* procName;
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

	const FMOD_REVERB_PROPERTIES v_reverbPreset = FMOD_PRESET_MOUNTAINS;
	v_pAudioMgr->fmod_system->setReverbProperties(0, &v_reverbPreset);
	const FMOD_REVERB_PROPERTIES v_reverbPreset2 = FMOD_PRESET_CAVE;
	v_pAudioMgr->fmod_system->setReverbProperties(1, &v_reverbPreset2);
	const FMOD_REVERB_PROPERTIES v_reverbPreset3 = FMOD_PRESET_GENERIC;
	v_pAudioMgr->fmod_system->setReverbProperties(2, &v_reverbPreset3);
	const FMOD_REVERB_PROPERTIES v_reverbPreset4 = FMOD_PRESET_UNDERWATER;
	v_pAudioMgr->fmod_system->setReverbProperties(3, &v_reverbPreset4);
}

void FMODHooks::Hook()
{
	HMODULE v_fmodStudio = GetModuleHandleA("fmodstudio.dll");
	if (!v_fmodStudio)
	{
		DebugErrorL("Couldn't get fmodstudio.dll module");
		return;
	}

	for (const FMODHookData& v_curHook : g_fmodHookData)
	{
		if (MH_CreateHook(
			GetProcAddress(v_fmodStudio, v_curHook.procName),
			v_curHook.detour,
			v_curHook.original) != MH_OK)
		{
			DebugErrorL("Couldn't hook the specified function: ", v_curHook.procName);
			return;
		}
	}

	DebugOutL("Successfully hooked all FMOD functions!");
}
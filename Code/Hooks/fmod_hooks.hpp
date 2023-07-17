#pragma once

#include <fmod/fmod_studio.hpp>
#include <fmod/fmod.hpp>

#include <unordered_map>
#include <string>

#include <cstddef>

namespace FEventInstance
{
	using Release = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*);
	using Start = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*);
	using Stop = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_STOP_MODE);
	using Get3DAttributes = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, FMOD_3D_ATTRIBUTES*);
	using Set3DAttributes = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, const FMOD_3D_ATTRIBUTES*);
	using GetVolume = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, float*, float*);
	using SetVolume = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, float);
	using GetDescription = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, FMOD::Studio::EventDescription**);
	using GetPlaybackState = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_PLAYBACK_STATE*);
	using GetTimelinePosition = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, int*);
	using SetTimelinePosition = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, int);
	using GetPitch = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, float*, float*);
	using SetPitch = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, float);
	using SetParameterByName = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventInstance*, const char*, float, bool);
}

namespace FEventDescription
{
	using GetLength = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventDescription*, int*);
	using CreateInstance = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventDescription*, FMOD::Studio::EventInstance**);
	using HasSustainPoint = FMOD_RESULT(__fastcall*)(FMOD::Studio::EventDescription*, bool*);
}

namespace FStudioSystem
{
	using LookupId = FMOD_RESULT(__fastcall*)(FMOD::Studio::System*, const char*, FMOD_GUID*);
	using GetEventById = FMOD_RESULT(__fastcall*)(FMOD::Studio::System*, const FMOD_GUID*, FMOD::Studio::EventDescription**);
}

struct SoundEffectData
{
	bool is_3d;
	int reverb_idx;
	float min_distance;
	float max_distance;
};

struct SoundData
{
	FMOD::Sound* sound;
	SoundEffectData effect_data;
};

#define FAKE_EVENT_DESC_MAGIC 13372281488

struct FakeEventDescription
{
	std::size_t v_secret_number = FAKE_EVENT_DESC_MAGIC;

	FMOD::Sound* sound;
	FMOD::Channel* channel;

	float custom_volume = 1.0f;
	int reverb_idx;

	float min_distance;
	float max_distance;

	bool is_3d;

	FakeEventDescription(const SoundData* snd_data, FMOD::Channel* v_channel)
	{
		this->sound = snd_data->sound;
		this->channel = v_channel;

		this->reverb_idx = snd_data->effect_data.reverb_idx;
		this->min_distance = snd_data->effect_data.min_distance;
		this->max_distance = snd_data->effect_data.max_distance;
		this->is_3d = snd_data->effect_data.is_3d;
	}

	FMOD_RESULT setVolume(float new_volume);
	FMOD_RESULT updateVolume();

	void updateReverbData();
	void playSound();

	bool isPlaying() const
	{
		if (!this->channel) return false;

		bool is_playing = false;
		this->channel->isPlaying(&is_playing);

		return is_playing;
	}

	bool isValidHook() const
	{
		if (this < reinterpret_cast<FakeEventDescription*>(0xfffffff))
			return false;

		return v_secret_number == FAKE_EVENT_DESC_MAGIC;
	}

	FMOD_RESULT release()
	{
		delete this;
		return FMOD_OK;/*return sound->release();*/
	}
};

class SoundStorage
{
public:
	inline static std::unordered_map<std::size_t, std::string> HashToPath;

	inline static std::unordered_map<std::size_t, FMOD::Sound*> PathHashToSound;
	inline static std::unordered_map<std::size_t, SoundData> NameHashToSound;

	inline static void ClearSounds()
	{
		for (auto& [sound_hash, sound_ptr] : SoundStorage::PathHashToSound)
			sound_ptr->release();

		SoundStorage::PathHashToSound.clear();
		SoundStorage::NameHashToSound.clear();

		SoundStorage::HashToPath.clear();
	}

	inline static bool SoundExists(std::size_t name_hash)
	{
		return SoundStorage::NameHashToSound.find(name_hash) != SoundStorage::NameHashToSound.end();
	}

	inline static SoundData* GetSoundData(std::size_t name_hash)
	{
		auto v_iter = SoundStorage::NameHashToSound.find(name_hash);
		if (v_iter == SoundStorage::NameHashToSound.end())
			return nullptr;

		return &v_iter->second;
	}

	inline static std::size_t SavePath(const std::string& path)
	{
		const std::size_t v_string_hash = std::hash<std::string>{}(path);

		auto v_iter = SoundStorage::HashToPath.find(v_string_hash);
		if (v_iter != SoundStorage::HashToPath.end())
			return v_string_hash;

		SoundStorage::HashToPath.emplace(v_string_hash, path);
		return v_string_hash;
	}

	inline static bool GetPath(std::size_t hash, std::string& path)
	{
		auto v_iter = SoundStorage::HashToPath.find(hash);
		if (v_iter == SoundStorage::HashToPath.end())
			return false;

		path = v_iter->second;
		return true;
	}

	static FMOD::Sound* CreateSound(const std::string& path);
	static void PreloadSound(const std::string& sound_path, const std::string& sound_name, const SoundEffectData& effect_data);
};

class FMODHooks
{
public:
	inline static FEventInstance::Release o_FMOD_Studio_EventInstance_release = nullptr;
	inline static FEventInstance::Start o_FMOD_Studio_EventInstance_start = nullptr;
	inline static FEventInstance::Stop o_FMOD_Studio_EventInstance_stop = nullptr;
	inline static FEventInstance::Get3DAttributes o_FMOD_Studio_EventInstance_get3DAttributes = nullptr;
	inline static FEventInstance::Set3DAttributes o_FMOD_Studio_EventInstance_set3DAttributes = nullptr;
	inline static FEventInstance::GetVolume o_FMOD_Studio_EventInstance_getVolume = nullptr;
	inline static FEventInstance::SetVolume o_FMOD_Studio_EventInstance_setVolume = nullptr;
	inline static FEventInstance::GetDescription o_FMOD_Studio_EventInstance_getDescription = nullptr;
	inline static FEventInstance::GetPlaybackState o_FMOD_Studio_EventInstance_getPlaybackState = nullptr;
	inline static FEventInstance::GetTimelinePosition o_FMOD_Studio_EventInstance_getTimelinePosition = nullptr;
	inline static FEventInstance::SetTimelinePosition o_FMOD_Studio_EventInstance_setTimelinePosition = nullptr;
	inline static FEventInstance::GetPitch o_FMOD_Studio_EventInstance_getPitch = nullptr;
	inline static FEventInstance::SetPitch o_FMOD_Studio_EventInstance_setPitch = nullptr;
	inline static FEventInstance::SetParameterByName o_FMOD_Studio_EventInstance_setParameterByName = nullptr;

	static FMOD_RESULT h_FMOD_Studio_EventInstance_release(FMOD::Studio::EventInstance* event_instance);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_start(FMOD::Studio::EventInstance* event_instance);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_stop(FMOD::Studio::EventInstance* event_instance, FMOD_STUDIO_STOP_MODE mode);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_get3DAttributes(FMOD::Studio::EventInstance* event_instance, FMOD_3D_ATTRIBUTES* attributes);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_set3DAttributes(FMOD::Studio::EventInstance* event_instance, const FMOD_3D_ATTRIBUTES* attributes);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_getVolume(FMOD::Studio::EventInstance* event_instance, float* volume, float* final_volume);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_setVolume(FMOD::Studio::EventInstance* event_instance, float volume);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_getDescription(FMOD::Studio::EventInstance* event_instance, FMOD::Studio::EventDescription** event_description);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_getPlaybackState(FMOD::Studio::EventInstance* event_instance, FMOD_STUDIO_PLAYBACK_STATE* state);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_getTimelinePosition(FMOD::Studio::EventInstance* event_instance, int* position);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_setTimelinePosition(FMOD::Studio::EventInstance* event_instance, int position);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_getPitch(FMOD::Studio::EventInstance* event_instance, float* pitch, float* finalpitch);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_setPitch(FMOD::Studio::EventInstance* event_instance, float pitch);
	static FMOD_RESULT h_FMOD_Studio_EventInstance_setParameterByName(FMOD::Studio::EventInstance* event_instance, const char* name, float value, bool ignoreseekspeed);

	//FMOD EVENT DESCRIPTION HOOKS

	inline static FEventDescription::GetLength o_FMOD_Studio_EventDescription_getLength = nullptr;
	inline static FEventDescription::CreateInstance o_FMOD_Studio_EventDescription_createInstance = nullptr;
	inline static FEventDescription::HasSustainPoint o_FMOD_Studio_EventDescription_hasSustainPoint = nullptr;

	static FMOD_RESULT h_FMOD_Studio_EventDescription_getLength(FMOD::Studio::EventDescription* event_desc, int* length);
	static FMOD_RESULT h_FMOD_Studio_EventDescription_createInstance(FMOD::Studio::EventDescription* event_desc, FMOD::Studio::EventInstance** instance);
	static FMOD_RESULT h_FMOD_Studio_EventDescription_hasSustainPoint(FMOD::Studio::EventDescription* event_desc, bool* has_sustain);

	//FMOD STUDIO SYSTEM HOOKS

	inline static FStudioSystem::LookupId o_FMOD_Studio_System_lookupID = nullptr;
	inline static FStudioSystem::GetEventById o_FMOD_Studio_System_getEventByID = nullptr;

	static FMOD_RESULT h_FMOD_Studio_System_lookupID(FMOD::Studio::System* system, const char* path, FMOD_GUID* id);
	static FMOD_RESULT h_FMOD_Studio_System_getEventByID(FMOD::Studio::System* system, const FMOD_GUID* id, FMOD::Studio::EventDescription** event_id);

	static void UpdateReverbProperties();

	static void Hook();
};
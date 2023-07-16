#pragma once

#include "win_include.hpp"

#include <unordered_map>
#include <string>

namespace SM
{
	struct GameSettings
	{
		std::unordered_map<std::string, int> int_settings;
		std::unordered_map<std::string, float> float_settings;
		std::unordered_map<std::string, std::string> string_settings;

		inline static GameSettings* GetInstance()
		{
			return *reinterpret_cast<GameSettings**>(std::uintptr_t(GetModuleHandle(NULL)) + 0x12A7840);
		}

		inline float get_effect_volume()
		{
			auto v_iter = this->float_settings.find("EffectVolume");
			if (v_iter != this->float_settings.end())
				return v_iter->second;

			return 1.0f;
		}

		inline float get_master_volume()
		{
			auto v_iter = this->float_settings.find("MasterVolume");
			if (v_iter != this->float_settings.end())
				return v_iter->second;

			return 1.0f;
		}

		inline static float GetEffectsVolume()
		{
			SM::GameSettings* v_game_settings = SM::GameSettings::GetInstance();
			if (v_game_settings)
				return v_game_settings->get_effect_volume() * v_game_settings->get_master_volume();

			return 1.0f; //Default volume
		}

	private:
		GameSettings() = delete;
		GameSettings(const GameSettings&) = delete;
		GameSettings(GameSettings&&) = delete;
		~GameSettings() = delete;
	};
}
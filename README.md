# SM-DLM
A dll that adds custom audio support for Scrap Mechanic mods

# How to download and enable

There are 2 ways to enable the DLM module:

<details>
<summary><small>SM-DLL-Injector</small></summary>

- Download the latest release of <b>[SM-DLL-Injector](https://github.com/QuestionableM/SM-DLL-Injector/releases/latest)</b> and follow the instructions listed in the <b>[README](https://github.com/QuestionableM/SM-DLL-Injector#readme)</b> file
- Download the latest release of the `DLM.dll` <b>[here](https://github.com/QuestionableM/SM-DLM/releases/latest)</b>
- Move the `DLM.dll` to `Steam/steamapps/common/Scrap Mechanic/Release/DLLModules` directory created by <b>[SM-DLL-Injector](https://github.com/QuestionableM/SM-DLL-Injector/releases/latest)</b> installer
- Launch the game

</details>

<details>
<summary><small>Manual Injection</small></summary>

- Download the latest release of the `DLM.dll` <b>[here](https://github.com/QuestionableM/SM-DLM/releases/latest)</b>
- Launch the game
- Inject `DLM.dll` by using a DLL Injector of your choice
  
</details>

# How to use
- Create `sm_dlm_config.json` in the root directory of your mod.
- An example of how `sm_dlm_config.json` structure should look like:
```jsonc
{
  //Make the names more unique to avoid name collisions with other mods
  "soundList": {
    //You can reference the same sound multiple times, but configure it differently
    "ExampleSoundName": {
      "path": "$CONTENT_DATA/Effects/Audio/example_sound.mp3",
      "is3D": true,
      "reverb": "MOUNTAINS", //Reverb is optional, possible parameters: GENERIC, MOUNTAINS, CAVE, UNDERWATER
    },
    "ExampleSoundName2": {
      "path": "$CONTENT_DATA/Effects/Audio/example_sound.mp3",
      "is3D": false
    }
  }
}
```
- The names specified in `sm_dlm_config.json` can then be used in effects!
```json
"ExampleEffect": {
  "parameterList": {
    "DLM_Volume": 1.0,
    "DLM_Pitch": 1.0,
    "DLM_Reverb": 1.0,
    "DLM_ReverbIdx": -1.0
  },
  "effectList": [
    {
      "type": "audio",
      "name": "ExampleSoundName",
      "parameters": [ "DLM_Volume", "DLM_Pitch", "DLM_Reverb", "DLM_ReverbIdx" ]
    }
  ]
}
```
- If you want to add DLM specific effects you can use the `sm.dlm_injected` flag to check if the DLM is present

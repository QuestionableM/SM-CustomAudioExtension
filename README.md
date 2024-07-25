# SM-CustomAudioExtension
A dll that adds custom audio support for Scrap Mechanic mods<br/>

![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/QuestionableM/SM-CustomAudioExtension/total)

# How to download and enable

There are 2 ways to enable the CustomAudioExtension module:

<details>
<summary><small>SM-DLL-Injector</small></summary>

- Download the latest release of <b>[SM-DLL-Injector](https://github.com/QuestionableM/SM-DLL-Injector/releases/latest)</b> and follow the instructions listed in the <b>[README](https://github.com/QuestionableM/SM-DLL-Injector#readme)</b> file
- Download the latest release of the `CustomAudioExtension.dll` <b>[here](https://github.com/QuestionableM/SM-CustomAudioExtension/releases/latest)</b>
- Move the `CustomAudioExtension.dll` to `Steam/steamapps/common/Scrap Mechanic/Release/DLLModules` directory created by <b>[SM-DLL-Injector](https://github.com/QuestionableM/SM-DLL-Injector/releases/latest)</b> installer
- Launch the game

</details>

<details>
<summary><small>Manual Injection</small></summary>

- Download the latest release of the `CustomAudioExtension.dll` <b>[here](https://github.com/QuestionableM/SM-CustomAudioExtension/releases/latest)</b>
- Launch the game
- Inject `CustomAudioExtension.dll` by using a DLL Injector of your choice
  
</details>

# How to use
- Create `sm_cae_config.json` in the root directory of your mod.
- An example of how `sm_cae_config.json` structure should look like:
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
- The names specified in `sm_cae_config.json` can then be used in effects!
```jsonc
"ExampleEffect": {
  "parameterList": {
    "CAE_Volume": 1.0, //1.0 - max volume
    "CAE_Pitch": 1.0, //1.0 - normal pitch
    "CAE_Reverb": 1.0, //1.0 - max reverb
    "CAE_ReverbIdx": -1.0,
    "CAE_Position": 0.0 //Measured in seconds
  },
  "effectList": [
    {
      "type": "audio",
      "name": "ExampleSoundName",
      "parameters": [ "CAE_Volume", "CAE_Pitch", "CAE_Reverb", "CAE_ReverbIdx", "CAE_Position" ]
    }
  ]
}
```
- If you want to add CustomAudioExtension specific effects you can use the `sm.cae_injected` flag to check if the CAE is present

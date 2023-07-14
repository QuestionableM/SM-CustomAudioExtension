# SM-DLM
A dll that adds custom audio support for Scrap Mechanic mods

# How to use
- Create `sm_dlm_config.json` in the root directory of your mod.
- An example of how `sm_dlm_config.json` structure should look like:
```jsonc
{
  //Make the names more unique to avoid name collisions with other mods
  "soundList": {
    //You can reference the same sound multiple times, but configure them differently
    "ExampleSoundName": {
      "path": "$CONTENT_DATA/Effects/Audio/example_sound.mp3",
      "is3D": true
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
  "effectList": [
    {
      "type": "audio",
      "name": "ExampleSoundName"
    }
  ]
}
```
- If you want to add DLM specific effects you can use the `sm.dlm_injected` flag to check if the DLM is present

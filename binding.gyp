{
  "targets": [
    {
      "target_name": "codecadon",
      "sources": [ "src/codecadon.cc", "src/Encoder.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")", "ffmpeg/include" ],
      "configurations": {
        "Release": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeTypeInfo": "true"
            }
          }
        }
      },
      "libraries": [ 
        "-l../ffmpeg/bin/avcodec.lib",
        "-l../ffmpeg/bin/avutil.lib"
      ],
      "copies": [
        { 
          "destination": "build/Release/",
          "files": [
            "ffmpeg/bin/avcodec-57.dll",
            "ffmpeg/bin/avutil-55.dll",
            "ffmpeg/bin/openh264.dll"
          ]
        }
      ]
    }
  ]
}

{
  "targets": [
    {
      "target_name": "codecadon",
      "sources": [ "src/codecadon.cc", 
                   "src/Encoder.cc", 
                   "src/OpenH264Encoder.cc",
                   "src/ScaleConverter.cc",
                   "src/Packers.cc" ],
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
        "-l../ffmpeg/bin/avutil.lib",
        "-l../ffmpeg/bin/swscale.lib"
      ],
      "copies": [
        { 
          "destination": "build/Release/",
          "files": [
            "ffmpeg/bin/avcodec-57.dll",
            "ffmpeg/bin/avutil-55.dll",
            "ffmpeg/bin/swscale-4.dll",
            "ffmpeg/bin/openh264.dll"
          ]
        }
      ]
    }
  ]
}

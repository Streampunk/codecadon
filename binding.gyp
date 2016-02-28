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
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++11',
              '-stdlib=libc++'
            ]
          },
          "link_settings": {
            "libraries": [
              "libavcodec.dylib",
              "libavutil.dylib",
              "libswscale.dylib",
              "libopenh264.dylib"
            ],
          },
          "copies": [
            { 
              "destination": "build/Release/",
              "files": [
                "<!@(ls -1 ffmpeg/bin/libavcodec*.dylib)",
                "<!@(ls -1 ffmpeg/bin/libavutil*.dylib)",
                "<!@(ls -1 ffmpeg/bin/libswscale*.dylib)",
                "<!@(ls -1 ffmpeg/bin/libopenh264*.dylib)"
              ]
            }
          ]
        }],
        ['OS=="win"', {
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
        }]
      ],
    }
  ]
}

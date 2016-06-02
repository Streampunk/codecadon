{
  "targets": [
    {
      "target_name": "codecadon",
      "sources": [ "src/codecadon.cc", 
                   "src/Concater.cc", 
                   "src/ScaleConverter.cc", 
                   "src/Decoder.cc",
                   "src/Encoder.cc",
                   "src/DecoderFF.cc",
                   "src/EncoderFF.cc",
                   "src/ScaleConverterFF.cc",
                   "src/Packers.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")", "ffmpeg/include" ],
      'conditions': [
        ['OS=="linux"', {
          "cflags_cc!": [ 
            "-fno-rtti" 
          ],
          "cflags_cc": [
            "-std=c++11"
          ],
          "link_settings": {
            "libraries": [
              "<@(module_root_dir)/build/Release/libavcodec.so.57",
              "<@(module_root_dir)/build/Release/libavutil.so.55",
              "<@(module_root_dir)/build/Release/libswscale.so.4",
              "<@(module_root_dir)/build/Release/libopenh264.so.1",
              "<@(module_root_dir)/build/Release/libvpx.so.3"
            ],
            "ldflags": [
              "-L<@(module_root_dir)/build/Release",
              "-Wl,-rpath,<@(module_root_dir)/build/Release"
            ]
          },
          "copies": [
            {
              "destination": "build/Release/",
              "files": [
                "<!@(ls -1 ffmpeg/bin/libavcodec.so*)",
                "<!@(ls -1 ffmpeg/bin/libavutil.so*)",
                "<!@(ls -1 ffmpeg/bin/libswscale.so*)",
                "<!@(ls -1 ffmpeg/bin/libopenh264*.so*)",
                "<!@(ls -1 ffmpeg/bin/libvpx*.so*)"
              ]
            }
          ] 
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++11',
              '-stdlib=libc++'
            ],
            'OTHER_LDFLAGS': [
              "-Wl,-rpath,<@(module_root_dir)/build/Release"
            ]
          },
          "link_settings": {
            "libraries": [
              "<@(module_root_dir)/build/Release/libavcodec.dylib",
              "<@(module_root_dir)/build/Release/libavutil.dylib",
              "<@(module_root_dir)/build/Release/libswscale.dylib",
              "<@(module_root_dir)/build/Release/libopenh264.dylib"
            ]
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

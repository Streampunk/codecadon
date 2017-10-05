{
  "targets": [
    {
      "target_name": "codecadon",
      "sources": [ "src/codecadon.cc",
                   "src/Concater.cc",
                   "src/Flipper.cc",
                   "src/Packer.cc",
                   "src/ScaleConverter.cc",
                   "src/Decoder.cc",
                   "src/Encoder.cc",
                   "src/Stamper.cc",
                   "src/ScaleConverterFF.cc",
                   "src/DecoderFF.cc",
                   "src/EncoderFF.cc",
                   "src/Packers.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")", "ffmpeg/include" ],
      'conditions': [
        ['OS=="linux"', {
          "conditions": [
            ['target_arch=="arm"',
              {
                "cflags_cc!": [
                  "-fno-rtti",
                  "-fno-exceptions"
                ],
                "cflags_cc": [
                  "-std=c++11",
                  "-fexceptions"
                ],
                "link_settings": {
                  "libraries": [
                    "<@(module_root_dir)/build/Release/libavcodec.so.57",
                    "<@(module_root_dir)/build/Release/libavutil.so.55",
                    "<@(module_root_dir)/build/Release/libswscale.so.4",
                    "<@(module_root_dir)/build/Release/libopenh264.so.3",
                    "<@(module_root_dir)/build/Release/libvpx.so.4"
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
                      "<!@(ls -1 ffmpeg/bin_armhf/libavcodec.so*)",
                      "<!@(ls -1 ffmpeg/bin_armhf/libavutil.so*)",
                      "<!@(ls -1 ffmpeg/bin_armhf/libswscale.so*)",
                      "<!@(ls -1 ffmpeg/bin_armhf/libopenh264*.so*)",
                      "<!@(ls -1 ffmpeg/bin_armhf/libvpx*.so*)"
                    ]
                  }
                ]
              },
              { # ia32 or x64
                "defines": [
                  "__STDC_CONSTANT_MACROS"
                ],
                "cflags_cc!": [
                  "-fno-rtti",
                  "-fno-exceptions"
                 ],
                 "cflags_cc": [
                   "-std=c++11",
                   "-fexceptions"
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
             }]
           ]
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
            'OTHER_CPLUSPLUSFLAGS': [
              '-std=c++11',
              '-stdlib=libc++',
              '-fexceptions'
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
                  "RuntimeTypeInfo": "true",
                  "ExceptionHandling": 1
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

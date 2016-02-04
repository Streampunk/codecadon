{
  "targets": [
    {
      "target_name": "codecadon",
      "sources": [ "src/codecadon.cc", "src/Encoder.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")" ]
    }
  ]
}

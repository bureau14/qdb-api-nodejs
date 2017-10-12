{
  "targets": [
    {
      "target_name": "<(module_name)",
      "sources": [
        "src/qdb_api.cpp",
        "src/entry.hpp",
        "src/expirable_entry.hpp",
        "src/blob.cpp",
        "src/blob.hpp",
        "src/cluster.cpp",
        "src/cluster.hpp",
        "src/deque.cpp",
        "src/deque.hpp",
        "src/error.cpp",
        "src/error.hpp",
        "src/hset.cpp",
        "src/hset.hpp",
        "src/integer.cpp",
        "src/integer.hpp",
        "src/prefix.cpp",
        "src/prefix.hpp",
        "src/range.cpp",
        "src/range.hpp",
        "src/tag.cpp",
        "src/tag.hpp",
        "src/time_series.cpp",
        "src/time_series.hpp",
        "src/points.cpp",
        "src/points.hpp",
        "src/column.cpp",
        "src/column.hpp",
        "src/cluster_data.hpp",
        "src/utilities.hpp",
        "src/utilities.cpp",
        "test/test.js"
      ],
      "conditions": [
        [
          "OS=='mac'",
          {
            "include_dirs": [
              "/usr/local/include",
              "<(module_root_dir)/deps/qdb/include"
            ],
            "libraries": [
              "-L<(module_root_dir)/deps/qdb/lib",
              "-lqdb_api",
              "-Wl,-rpath,@loader_path"
            ],
            "xcode_settings": {
              "OTHER_CFLAGS": [
                "-std=c++11",
                "-stdlib=libc++",
                "-Wno-strict-aliasing",
                "-mmacosx-version-min=10.7"
              ]
            }
          }
        ],
        [
          "OS=='freebsd'",
          {
            "include_dirs": [
              "/usr/local/include",
              "<(module_root_dir)/deps/qdb/include"
            ],
            "libraries": [
              "-L/usr/local/lib",
              "-L<(module_root_dir)/deps/qdb/lib",
              "-lqdb_api",
              "-Wl,-rpath=\'$$ORIGIN\'"
            ],
            "cflags": [
              "-std=c++11",
              "-stdlib=libc++",
              "-Wno-strict-aliasing",
              "-Wno-deprecated-declarations",
              "-U_LIBCPP_TRIVIAL_PAIR_COPY_CTOR"
            ]
          }
        ],
        [
          "OS=='linux'",
          {
            "include_dirs": [
              "/usr/local/include",
              "<(module_root_dir)/deps/qdb/include"
            ],
            "libraries": [
              "-L/usr/local/lib",
              "-L<(module_root_dir)/deps/qdb/lib",
              "-lqdb_api",
              "-Wl,-rpath=\'$$ORIGIN\'"
            ],
            "cflags": [
              "-std=c++11",
              "-Wno-strict-aliasing"
            ]
          }
        ],
        [
          "OS=='win'",
          {
            "include_dirs": [
              "<(module_root_dir)/deps/qdb/include"
            ],
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": "2",
                "DisableSpecificWarnings": [
                  "4530"
                ]
              }
            },
            "link_settings": {
              "libraries": [
                "<(module_root_dir)/deps/qdb/lib/qdb_api.lib"
              ]
            }
          }
        ]
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [
        "<(module_name)"
      ],
      "conditions": [
        [
          "OS=='mac'",
          {
            "copies": [
              {
                "destination": "<(module_path)",
                "files": [
                  "<(PRODUCT_DIR)/<(module_name).node",
                  "<(module_root_dir)/deps/qdb/lib/libqdb_api.dylib"
                ]
              }
            ]
          }
        ],
        [
          "OS=='freebsd' or OS=='linux'",
          {
            "copies": [
              {
                "destination": "<(module_path)",
                "files": [
                  "<(PRODUCT_DIR)/<(module_name).node",
                  "<(module_root_dir)/deps/qdb/lib/libqdb_api.so"
                ]
              }
            ]
          }
        ],
        [
          "OS=='win'",
          {
            "copies": [
              {
                "destination": "<(module_path)",
                "files": [
                  "<(PRODUCT_DIR)/<(module_name).node",
                  "<(module_root_dir)/deps/qdb/bin/qdb_api.dll"
                ]
              }
            ]
          }
        ]
      ]
    }
  ]
}

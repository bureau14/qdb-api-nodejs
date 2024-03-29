{
    "variables": {
        "copy_c_api": "no",
        "c_api_path": "<(module_root_dir)/qdb",
    },
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
                "src/error.cpp",
                "src/error.hpp",
                "src/integer.cpp",
                "src/integer.hpp",
                "src/prefix.cpp",
                "src/prefix.hpp",
                "src/query_find.cpp",
                "src/query_find.hpp",
                "src/query.cpp",
                "src/query.hpp",
                "src/range.cpp",
                "src/range.hpp",
                "src/suffix.cpp",
                "src/suffix.hpp",
                "src/tag.cpp",
                "src/tag.hpp",
                "src/time_series.cpp",
                "src/time_series.hpp",
                "src/ts_column.cpp",
                "src/ts_column.hpp",
                "src/ts_point.cpp",
                "src/ts_point.hpp",
                "src/ts_range.cpp",
                "src/ts_range.hpp",
                "src/ts_aggregation.cpp",
                "src/ts_aggregation.hpp",
                "src/cluster_data.hpp",
                "src/utilities.cpp",
                "src/utilities.hpp",
                "src/time.cpp",
                "src/time.hpp",
                "test/blobTest.js",
                "test/clusterTest.js",
                "test/config.js",
                "test/deamonRunner.js",
                "test/integerTest.js",
                "test/prefixTest.js",
                "test/queryTest.js",
                "test/rangeTest.js",
                "test/suffixTest.js",
                "test/tagTest.js",
                "test/tsBlobTest.js",
                "test/tsDoubleTest.js",
                "test/tsGeneralTest.js",
                "test/tsInt64Test.js",
                "test/tsStringTest.js",
                "test/tsTimestampTest.js",
            ],
            "conditions": [
                [
                    "OS=='mac'",
                    {
                        "include_dirs": [
                            "<(c_api_path)/include"
                        ],
                        "libraries": [
                            "-L<(c_api_path)/lib",
                            "-lqdb_api",
                            "-Wl,-rpath,@loader_path"
                        ],
                        "xcode_settings": {
                            # We need to use these variables, because otherwise node-gyp will set them anyway and the order of options will be important.
                            # These results in problems, because OTHER_CFLAGS are appended to compile flags, but OTHER_LDFLAGS are prepended to linker flags.
                            # Cf. https://github.com/nodejs/node-gyp/blob/d7687d55666fa77928cce270b8991b8e819c5094/gyp/pylib/gyp/xcode_emulation.py#L557
                            "CLANG_CXX_LIBRARY": "libc++",
                            "CLANG_CXX_LANGUAGE_STANDARD": "c++14",
                            "MACOSX_DEPLOYMENT_TARGET": "10.14",
                            "OTHER_CFLAGS": [
                                "-Werror",
                                "-Wno-strict-aliasing",
                                "-Wno-unused-result",
                            ],
                        }
                    }
                ],
                [
                    "OS=='freebsd'",
                    {
                        "include_dirs": [
                            "/usr/local/include",
                            "<(c_api_path)/include"
                        ],
                        "libraries": [
                            "-L/usr/local/lib",
                            "-L<(c_api_path)/lib",
                            "-lqdb_api",
                            "-Wl,-rpath=\'$$ORIGIN\'"
                        ],
                        "cflags": [
                            "-std=c++14",
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
                            "<(c_api_path)/include"
                        ],
                        "libraries": [
                            "-L/usr/local/lib",
                            "-L<(c_api_path)/lib",
                            "-lqdb_api",
                            "-Wl,-rpath=\'$$ORIGIN\'",
                            "-static-libgcc",
                            "-static-libstdc++"
                        ],
                        "cflags": [
                            "-std=c++14",
                            "-Werror",
                            "-Wno-error=cast-function-type",
                            "-Wno-strict-aliasing",
                            "-Wno-unused-result"
                        ]
                    }
                ],
                [
                    "OS=='win'",
                    {
                        "include_dirs": [
                            "<(c_api_path)/include"
                        ],
                        # Cf. https://github.com/nodejs/node-gyp/blob/master/gyp/pylib/gyp/MSVSSettings.py.
                        "msvs_settings": {
                            "VCCLCompilerTool": {
                                "ExceptionHandling": "2",
                                # "VerboseOutput": "true",
                                "WarnAsError": "true",
                                "DisableSpecificWarnings": [
                                    "4309" # 'static_cast': truncation of constant value
                                ]
                            }
                        },
                        "link_settings": {
                            "libraries": [
                                "<(c_api_path)/lib/qdb_api.lib"
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
                                    "<(PRODUCT_DIR)/<(module_name).node"
                                ],
                                "conditions": [
                                    [
                                        "copy_c_api=='yes'",
                                        {
                                            "files": [
                                                "<(c_api_path)/lib/libqdb_api.dylib"
                                            ]
                                        }
                                    ]
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
                                    "<(PRODUCT_DIR)/<(module_name).node"
                                ],
                                "conditions": [
                                    [
                                        "copy_c_api=='yes'",
                                        {
                                            "files": [
                                                "<(c_api_path)/lib/libqdb_api.so"
                                            ]
                                        }
                                    ]
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
                                    "<(PRODUCT_DIR)/<(module_name).node"
                                ],
                                "conditions": [
                                    [
                                        "copy_c_api=='yes'",
                                        {
                                            "files": [
                                                "<(c_api_path)/bin/qdb_api.dll"
                                            ]
                                        }
                                    ]
                                ]
                            }
                        ]
                    }
                ]
            ]
        }
    ]
}

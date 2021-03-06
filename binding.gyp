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
                            "/usr/local/include",
                            "<(c_api_path)/include"
                        ],
                        "libraries": [
                            "-L<(c_api_path)/lib",
                            "-lqdb_api",
                            "-Wl,-rpath,@loader_path"
                        ],
                        "xcode_settings": {
                            "OTHER_CFLAGS": [
                                "-std=c++14",
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
                            "-Wno-strict-aliasing"
                        ]
                    }
                ],
                [
                    "OS=='win'",
                    {
                        "include_dirs": [
                            "<(c_api_path)/include"
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
                                                "<(c_api_path)/lib/libc++.1.0.dylib",
                                                "<(c_api_path)/lib/libc++.1.dylib",
                                                "<(c_api_path)/lib/libc++.LICENSE.TXT",
                                                "<(c_api_path)/lib/libc++.dylib",
                                                "<(c_api_path)/lib/libc++abi.1.0.dylib",
                                                "<(c_api_path)/lib/libc++abi.1.dylib",
                                                "<(c_api_path)/lib/libc++abi.LICENSE.TXT",
                                                "<(c_api_path)/lib/libc++abi.dylib",
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

{
  "conditions": [ [ "OS=='freebsd'", {"make_global_settings": [ ["CC", "/usr/bin/clang"], ["CXX", "/usr/bin/clang++"] ]} ] ],

  "targets": [
    {
      "target_name": "qdb",
      "sources": [ "src/qdb_api.cpp", "src/blob.cpp", "src/cluster.cpp", "src/integer.cpp", "src/queue.cpp", "src/set.cpp" ],
      "conditions": [ 
            [ "OS=='win'", { "copies": [ { 
                                            "destination" : "<(module_root_dir)/build/Release/",
                                            "files": [ "<(module_root_dir)/deps/qdb/bin/qdb_api.dll" ] 
                                        } ],
                              "include_dirs": [ "<(module_root_dir)/deps/qdb/include" ], 
                              "msvs_settings": { "VCCLCompilerTool": { "ExceptionHandling": "2", "DisableSpecificWarnings": [ "4530" ] } },
                              "link_settings": { "libraries": [ "<(module_root_dir)/deps/qdb/lib/qdb_api.lib" ] } }, 
                            { "include_dirs": [ "/usr/local/include", "<(module_root_dir)/deps/qdb/include" ], 
                              "libraries": [ "-L/usr/local/lib", "-L<(module_root_dir)/deps/qdb/lib/qdb_api.lib", "-lqdb_api"],
                               "cflags": [ "-std=c++11" ] } ]
        ]
    }
  ]
}


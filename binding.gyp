{
  "conditions": [ [ "OS=='freebsd'", {"make_global_settings": [ ["CC", "/usr/bin/clang"], ["CXX", "/usr/bin/clang++"] ]} ] ],

  "targets": [
    {
      "target_name": "qdb",
      "sources": [ "src/qdb_api.cpp", "src/blob.cpp", "src/cluster.cpp", "src/integer.cpp", "src/queue.cpp", "src/set.cpp" ],
      "conditions": [ 
            [ "OS=='win'", { "link_settings": { "libraries": [ "qdb_api.lib" ] } }, 
                            { "include_dirs": [ "/usr/local/include" ], 
                              "libraries": [ "-L/usr/local/lib", "-lqdb_api"],
                               "cflags": [ "-std=c++11" ] } ]
        ]
    }
  ]
}


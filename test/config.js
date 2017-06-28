var address = "127.0.0.1:3030";

module.exports = {
    "qdbd_path" : __dirname + '/../deps/qdb/bin/qdbd',
    "qdbd_args" : [ '--security=false', '--address=' + address, '--transient' ],
    "cluster_uri" : "qdb://" + address
};
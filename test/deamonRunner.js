const spawn = require('child_process').spawn;
const config = require('./config');

var qdbd = null;

function log(str) {
    console.log("[deamonRunner] " + str);
}

before('start qdbd', function(done) {
    log("starting qdbd...");

    this.timeout(5000);
    qdbd = spawn(config.qdbd_path, config.qdbd_args);

    // wait 2 seconds to make sure the deamon started
    setTimeout(function() {
        log("starting qdbd... pid=" + qdbd.pid);
        done();
    }, 2000);
});

after('stop qdbd', function(done) {
    log("stopping qdbd...");
    this.timeout(5000);
    qdbd.on('exit', function() {
        log("stopping qdbd... OK");
        done();
    });
    qdbd.kill('SIGTERM');
});
var test = require('unit.js');
var qdb = require('..');
var config = require('./config');

describe('Cluster', function() {
    var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

    // tests for race condition in connect
    describe('connect', function () {
        var wrong_cluster = new qdb.Cluster('qdb://127.0.0.1:4444');

        it('should not connect but not crash', function(done)
        {
            this.timeout(100000);

            wrong_cluster.connect(function(){}, function(err)
            {
                wrong_cluster.connect(function(){}, function(err)
                {
                    wrong_cluster.connect(function(){}, function(err)
                    {
                        wrong_cluster.connect(function(){}, function(err)
                        {
                            test.must(err).not.be.equal(null);

                            err.message.must.not.be.empty;
                            err.code.must.be.a.number();
                            err.severity.must.be.a.number();
                            err.origin.must.be.a.number();
                            err.informational.must.be.false();

                            done();
                        });
                    });
                });
            });
        });
    }); // connect
    
      
    describe('connect on secure cluster', function () {
        var secureCluster = new qdb.Cluster(config.secure_cluster_uri, config.cluster_public_key_file, config.user_credentials_file);
  
        it('should connect on insecure cluster', function(done) {
            secureCluster.connect(function(){}, function(err) {
                done();
            })
        });
        it('should set the timeout to 5 seconds', function() {
            secureCluster.setTimeout(5000);
        });

        it('should refuse to set the timeout to 400 ms', function() {
            test.exception(function() {
                secureCluster.setTimeout(400);
            });
        });
    });

    describe('setTimeout', function() {
        it('should set the timeout to 5 seconds', function() {
            insecureCluster.setTimeout(5000);
        });

        it('should refuse to set the timeout to 400 ms', function() {
            test.exception(function() {
                insecureCluster.setTimeout(400);
            });
        });
    }); // setTimeout

    describe('getTimeout', function() {
        it('should return the default timeout of 5 seconds', function(done) {
            test.must(insecureCluster.getTimeout()).be.equal(5000);
            done();
        });
        it('should return the new timeout after changing the default timeout', function(done) {
            insecureCluster.setTimeout(3000);
            test.must(insecureCluster.getTimeout()).be.equal(3000);
            done();
        });
    }); // getTimeout
});

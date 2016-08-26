var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

describe('Cluster', function() {
    var cluster = new qdb.Cluster(config.cluster_uri);

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

                            test.must(err.message).not.be.empty;
                            test.must(err.code).be.a.number();
                            test.must(err.severity).be.a.number();
                            test.must(err.origin).be.a.number();
                            test.must(err.informational).be.false();

                            done();
                        });
                    });
                });
            });
        });
    }); // connect

    describe('setTimeout', function() {
        it('should set the timeout to 5\'000 ms', function() {
            cluster.setTimeout(5000);
        });

        it('should refuse to set the timeout to 400 ms', function() {
            test.exception(function() {
                cluster.setTimeout(400);
            });
        });
    }); // setTimeout
});

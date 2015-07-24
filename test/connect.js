/*
var spawn = require('child_process').spawn;
var execSync = require('child_process').execSync;
var qdb = require('../build/Release/qdb.node');
var test = require('unit.js');
var cluster = new qdb.Cluster('qdb://127.0.0.1:3030');
var qdbd = null; 

describe('qdb', function()
{
    it('test suite', function()
    {
        describe('connect', function()
        {

            this.timeout(50000);

            it('should not connect', function(done)
            {
                cluster.connect(function(){}, function(err) { done(); });
            });               

            it('should spawn the daemon successfully', function(done)
            {
                qdbd = spawn('./deps/qdb/bin/qdbd', ['--transient','--address=127.0.0.1:3030']);
                this.timeout(5000);                    
                setTimeout(done, 3000);
            });    

            it('should connect', function(done)
            {
                cluster.connect(function(){ done(); }, function(err) {});
            }); 

            it('should set the value to 0', function(done)
            {
                i = cluster.integer('int');

                i.update(0, function(err)
                {
                    test.must(err).be.a.number();
                    test.must(err).be.equal(0);

                    done();
                });
            });

            it('should terminate the daemon', function(done)
            {
                qdbd.kill('SIGTERM');
                qdbd.on('exit', function(code)
                {
                    done();
                });
            });

        }); // connect

    }); // test suite

}); // qdb
    

*/
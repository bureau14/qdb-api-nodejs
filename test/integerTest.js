var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

describe('Integer', function () {
    var i;

    before('connect', function(done) {
        insecureCluster.connect(done, done);
    });

    before('init', function() {
        i = insecureCluster.integer('int_test');
    });

    it('should be of correct type', function() {
        test.object(i).isInstanceOf(qdb.Integer);
    });

    it("should have an 'alias' property", function() {
        test.object(i).hasProperty('alias');
        test.must(i.alias()).be.a.string();
        test.must(i.alias()).be.equal('int_test');
    });

    describe('update/add/add/remove', function() {
        it('should set the value to 0', function(done) {
            i.update(0, function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return correct entry type', function(done) {
            i.getMetadata(function(err, meta) {
                test.must(err).be.equal(null);
                test.must(meta.type).be.a.number();
                test.must(meta.type).be.equal(qdb.ENTRY_INTEGER);

                done();
            });
        });

        it('should after add be equal to 42', function(done) {
            i.add(42, function(err, data) {
                test.must(err).be.equal(null);

                test.must(data).be.a.number();
                test.must(data).be.equal(42);

                done();
            });
        });

        it('should after add be equal to 20', function(done) {
            i.add(-22, function(err, data) {
                test.must(err).be.equal(null);

                test.must(data).be.a.number();
                test.must(data).be.equal(20);

                done();
            });
        });

        it('should remove without an error', function(done) {
            i.remove(function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should set the value to 10', function(done) {
            i.put(10, function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should get the value of 10', function(done) {
            i.get(function(err, data) {
                test.must(err).be.equal(null);

                test.must(data).be.a.number();
                test.must(data).be.equal(10);

                done();
            });
        });

        it('should remove again without an error', function(done) {
            i.remove(function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

    }); // update/add/add/remove

    // work on a different integer
    describe('expiry', function() {
        var i;

        before('init', function() {
            i = insecureCluster.integer('expiry_int');
        });

        it('should update with an expiry in 2s without error', function(done) {
            var test_exp = new Date();
            test_exp.setSeconds(test_exp.getSeconds() + 2);

            i.update(10, test_exp, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should wait three seconds', function(done)
        {
            this.timeout(5000);
            setTimeout(done, 3000);
        });

        it('should expire after more than 2s', function(done)
        {
            i.get(function(err, data)
            {
                err.message.must.be.a.string();
                err.message.must.not.be.empty();
                err.code.must.be.equal(qdb.E_ALIAS_NOT_FOUND);
                err.severity.must.be.equal(qdb.E_SEVERITY_WARNING);
                err.origin.must.be.a.equal(qdb.E_ORIGIN_OPERATION);
                err.origin.must.be.a.number();
                err.informational.must.be.false();

                done();
            });
        });

        it('should put with an expiry in 2030', function(done)
        {
            var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

            i.put(20, test_exp, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });

        });

        it('should return the exact supplied expiry', function(done)
        {
            var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

            i.getExpiry(function(err, entry_expiry)
            {
                test.must(err).be.equal(null);

                test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                done();
            });
        });

        it('should set the expiry to 2040', function(done)
        {
            var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

            i.expiresAt(test_exp, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });

        });

        it('should return the new supplied expiry', function(done)
        {
            var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

            i.getExpiry(function(err, entry_expiry)
            {
                test.must(err).be.equal(null);

                test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                done();
            });
        });

        it('should expire in 2 seconds from now', function(done)
        {
            i.expiresFromNow(2, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should wait three seconds', function(done)
        {
            this.timeout(5000);
            setTimeout(done, 3000);
        });

        it('should expire after more than 3s', function(done)
        {
            i.get(function(err, data)
            {
                err.message.must.be.a.string();
                err.message.must.not.be.empty();
                err.code.must.be.equal(qdb.E_ALIAS_NOT_FOUND);
                                err.origin.must.be.equal(qdb.E_ORIGIN_OPERATION);

                err.origin.must.be.a.number();
                err.informational.must.be.false();

                done();
            });
        });

    }); // expiry

}); // integer

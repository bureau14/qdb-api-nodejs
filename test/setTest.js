var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.cluster_uri);

describe('Set', function() {
    var s;

    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function() {
        s = cluster.set('s1');
    });

    it('should be of correct type', function() {
        test.object(s).isInstanceOf(qdb.Set);
    });

    it("should have an 'alias' property", function() {
        test.object(s).hasProperty('alias');
        test.must(s.alias()).be.a.string();
        test.must(s.alias()).be.equal('s1');
    });

    describe('contains/insert/contains/erase/contains', function() {
        it('should not contain the entry', function(done) {
            s.contains(new Buffer('da'), function(err) {
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.a.number();
                test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();
                test.must(err.informational).be.false();

                done();
            });
        });

        it('should insert without an error', function(done) {
            s.insert(new Buffer('da'), function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return correct entry type', function(done) {
            s.getMetadata(function(err, meta) {
                test.must(err).be.equal(null);
                test.must(meta.type).be.a.number();
                test.must(meta.type).be.equal(qdb.ENTRY_HSET);

                done();
            });
        });

        it('should insert with an error', function(done) {
            s.insert(new Buffer('da'), function(err) {
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.a.number();
                test.must(err.code).be.equal(qdb.E_ELEMENT_ALREADY_EXISTS);
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();
                test.must(err.informational).be.true();

                done();
            });
        });

        it('should now contain the entry', function(done) {
            s.contains(new Buffer('da'), function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should erase without an error', function(done) {
            s.erase(new Buffer('da'), function(err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should no longer contain the entry', function(done) {
            s.contains(new Buffer('da'), function(err) {
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.a.number();
                test.must(err.code).be.equal(qdb.E_ELEMENT_NOT_FOUND);
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();
                test.must(err.informational).be.true();

                done();
            });
        });
    });
}); // set

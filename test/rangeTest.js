var test = require('unit.js');
var Promise = require('bluebird'); // Using default Node.js Promise is very slow

var qdb = require('..');
var config = require('./config')

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

function put_blob(alias) {
    const content = new Buffer('prefix_blob_content');
    return new Promise(function(resolve, reject) {
        insecureCluster.blob(alias).update(content, function(err) {
            if (err) reject(err);
            else resolve();
        });
    });
}

describe('Range', function() {
    var r = null;
    var prefix = 'range';
    var content = 'pattern';
    var matchingAliases = [ prefix + '1', prefix + '2', prefix + '3', prefix + '4' ];

    before('connect', function(done) {
        insecureCluster.connect(done, done);
    });

    before('init', function() {
        r = insecureCluster.range();
    });

    before('put blobs', function() {
        var promises = matchingAliases.map(function(a) {
            return new Promise(function(resolve, reject) {
                insecureCluster.blob(a).update(new Buffer(content + a), function(err) {
                    if (err) reject(err);
                    else resolve();
                });
            });
        });

        return Promise.all(promises);
    });

    it('should be of correct type', function() {
        test.object(r).isInstanceOf(qdb.Range);
    });

    it("shouldn't have an 'alias' property", function() {
        test.object(r).hasNotProperty('alias');
    });

    it("should have a 'blobScan()' method", function() {
        test.object(r).hasProperty('blobScan');
        test.must(r.blobScan).be.a.function();
    });

    it("should have a 'blobScanRegex()' method", function() {
        test.object(r).hasProperty('blobScanRegex');
        test.must(r.blobScanRegex).be.a.function();
    });

    describe('blobScan()', function() {
        var non_matching_pattern = 'unexisting_pattern';
        var matching_pattern = content;

        it('should return a proper error object', function(done) {
            r.blobScan(function(err, aliases) {
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.a.number();
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();

                test.must(err.informational).be.boolean();

                done();
            });
        });

        it('should return E_INVALID_ARGUMENT when pattern is missing', function(done) {
            r.blobScan(function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it.skip('should return E_INVALID_ARGUMENT when maxCount is missing', function(done) {
            r.blobScan(non_matching_pattern, function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return E_ALIAS_NOT_FOUND and an empty list', function(done) {
            r.blobScan(non_matching_pattern, 10, function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return a list of alias', function(done) {
            r.blobScan(matching_pattern, /*maxCount=*/100, function(err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(matchingAliases.length);
                test.array(aliases).contains(matchingAliases);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });

        it('should truncate list when maxCount is small', function(done) {
            r.blobScan(matching_pattern, /*maxCount=*/2, function(err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(2);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });
    }); // blobScan()

    describe('blobScanRegex()', function() {
        var non_matching_pattern = 'unexisting_pattern';
        var matching_pattern = content + '[a-z]*[0-9]+';

        it('should return a proper error object', function(done) {
            r.blobScanRegex(function(err, aliases) {
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.a.number();
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();

                test.must(err.informational).be.boolean();

                done();
            });
        });

        it('should return E_INVALID_ARGUMENT when pattern is missing', function(done) {
            r.blobScanRegex(function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it.skip('should return E_INVALID_ARGUMENT when maxCount is missing', function(done) {
            r.blobScanRegex(non_matching_pattern, function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return E_ALIAS_NOT_FOUND and an empty list', function(done) {
            r.blobScanRegex(non_matching_pattern, 10, function(err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return a list of alias', function(done) {
            r.blobScanRegex(matching_pattern, /*maxCount=*/100, function(err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(matchingAliases.length);
                test.array(aliases).contains(matchingAliases);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });

        it('should truncate list when maxCount is small', function(done) {
            r.blobScanRegex(matching_pattern, /*maxCount=*/2, function(err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(2);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });
    }); // blobScanRegex
});

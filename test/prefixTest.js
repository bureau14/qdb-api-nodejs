var test = require('unit.js');
var qdb = require('..');
var config = require('./config')
var Promise = require('bluebird'); // Using default Node.js Promise is very slow

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

function put_blob(alias) {
    const content = new Buffer('prefix_blob_content');
    return new Promise(function (resolve, reject) {
        insecureCluster.blob(alias).update(content, function (err) {
            if (err) reject(err);
            else resolve();
        });
    });
}

describe('Prefix', function () {
    var prefix = 'prefix';
    var p = null;
    var matchingAliases = [prefix + '1', prefix + '2', prefix + '3', prefix + '4'];

    before('connect', function (done) {
        insecureCluster.connect(done, done);
    });

    before('init', function () {
        p = insecureCluster.prefix(prefix);
    });

    it('should be of correct type', function () {
        test.object(p).isInstanceOf(qdb.Prefix);
    });

    it("shouldn't have an 'alias' property", function () {
        test.object(p).hasNotProperty('alias');
    });

    it("should have a 'prefix' property", function () {
        test.object(p).hasProperty('prefix');
        test.must(p.prefix()).be.a.string();
        test.must(p.prefix()).be.equal(prefix);
    });

    it("should have 'getEntries' method", function () {
        test.object(p).hasProperty('getEntries');
        test.must(p.getEntries).be.a.function();
    });

    describe("getEntries()", function () {

        before('put samples', function () {
            return Promise.all(matchingAliases.map(put_blob));
        });

        it('should say E_INVALID_ARGUMENT when maxCount is missing', function (done) {
            p.getEntries(function (err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.informational).be.false();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return E_ALIAS_NOT_FOUND and an empty list', function (done) {
            var p = insecureCluster.prefix('not matching');
            p.getEntries(10, function (err, aliases) {
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                test.must(err.informational).be.false();
                test.must(err.severity).be.a.number();
                test.must(err.origin).be.a.number();

                aliases.must.be.empty();

                done();
            });
        });

        it('should return an alias list', function (done) {
            p.getEntries(/*maxCount=*/100, function (err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(matchingAliases.length);
                test.array(aliases).contains(matchingAliases);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });

        it('should truncate list when maxCount is too small', function (done) {
            p.getEntries(/*maxCount=*/2, function (err, aliases) {
                test.must(err).be.equal(null);

                test.array(aliases).isNotEmpty();
                test.value(aliases.length).isEqualTo(2);
                test.array(matchingAliases).contains(aliases);

                done();
            });
        });
    }); // getEntries
});

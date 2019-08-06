var test = require('unit.js');
var qdb = require('..');
var config = require('./config');

var cluster = new qdb.Cluster(config.insecure_cluster_uri);
var my_tag1 = 'my_tag1';
var my_tag2 = 'my_tag2';
var blob_name = 'blob_query_test';

describe('QueryFind', function (done) {
    var b
    before('connect', function (done) { cluster.connect(done, done); });

    before('init', function (done) {
        b = cluster.blob(blob_name);

        b.put(new Buffer('boom'), function (err) {
            test.must(err).be.equal(null);
            b.attachTag(my_tag1, function (err) {
                test.must(err).be.equal(null);
                b.attachTag(my_tag2, function (err) {
                    test.must(err).be.equal(null);
                    done();
                });
            });
        });
    });
    after('QueryFind', function(done) {
        b.remove(function (err) {
            test.must(err).be.equal(null);
            
            done();
        });
    })


    it('should say E_INVALID_QUERY when invalid query string is used', function (done) {
        var query = "sdljdflsdkjf";
        cluster.queryFind(query).run(function (err, output) {
            err.code.must.be.equal(qdb.E_INVALID_QUERY);
            output.must.have.length(0);
            done();
        });
    });

    it('shouldn\'t return the alias name when querying with tag and type=integer', function (done) {
        var query = "find(Tag='" + my_tag1 + "' AND type=integer)";
        cluster.queryFind(query).run(function (err, output) {
            test.must(err).be.equal(null);
            output.must.have.length(0);
            done();
        });
    });

    it('should return the alias name when querying with tag and type=blob', function (done) {
        var query = "find(Tag='" + my_tag1 + "' AND type=blob)";
        cluster.queryFind(query).run(function (err, output) {
            test.must(err).be.equal(null);
            output.must.have.length(1);
            test.must(output.indexOf(blob_name)).be.gte(0);
            done();
        });
    });

    it('should return the alias name when querying with a single tag1', function (done) {
        var query = "find(Tag='" + my_tag1 + "')";
        cluster.queryFind(query).run(function (err, output) {
            test.must(err).be.equal(null);
            output.must.have.length(1);
            test.must(output.indexOf(blob_name)).be.gte(0);
            done();
        });
    });

    it('should return the alias name when querying with a single tag2', function (done) {
        var query = "find(Tag='" + my_tag2 + "')";
        cluster.queryFind(query).run(function (err, output) {
            test.must(err).be.equal(null);
            output.must.have.length(1);
            test.must(output.indexOf(blob_name)).be.gte(0);
            done();
        });
    });

    it('should return the alias name when querying with two tags', function (done) {
        var query = "find(Tag='" + my_tag1 + "' AND tag='" + my_tag2 + "')";
        cluster.queryFind(query).run(function (err, output) {
            test.must(err).be.equal(null);
            output.must.have.length(1);
            test.must(output.indexOf(blob_name)).be.gte(0);
            done();
        });
    });
});

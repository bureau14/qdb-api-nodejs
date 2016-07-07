var binary = require('node-pre-gyp');
var path = require('path')
var qdb_path = binary.find(path.resolve(path.join(__dirname,'../package.json')));
var qdb = require(qdb_path);

var spawn = require('child_process').spawn;
var execSync = require('child_process').execSync;
var test = require('unit.js');
var Promise = require('bluebird'); // Using default Node.js Promise is very slow

var qdbd = null;
var cluster = new qdb.Cluster('qdb://127.0.0.1:3030');

describe('quasardb', function() {
    before('run qdb daemon', function(done) {
        this.timeout(50000);
        qdbd = spawn(__dirname + '/../deps/qdb/bin/qdbd', [ '--address=127.0.0.1:3030' ]);

        // wait 2 seconds then try to connect
        setTimeout(function() {
            cluster.connect(done, function(err) {
                throw new Error("an error occurred in cluster launch: " + err.message);
            });
        }, 2000);
    });

    after('terminate qdb daemon', function(done) {
        this.timeout(10000);
        qdbd.kill('SIGTERM');
        qdbd.on('exit', function(code) {
            done();
        });
    });

    describe('cluster', function() {
        it('is of correct type', function(done) {
            test.object(cluster).isInstanceOf(qdb.Cluster);
            done();
        });
    }); // cluster

    describe('prefix', function () {
        var prefix = 'prefix';
        var p = null;

        before('init', function(done) {
            p = cluster.prefix(prefix);
            done();
        });

        describe('object', function() {
            it('is of correct type', function(done) {
                test.object(p).isInstanceOf(qdb.Prefix);
                done();
            });

            it('has not alias property', function (done) {
                test.object(p).hasNotProperty('alias');
                done();
            });

            it('has prefix property', function(done) {
                test.object(p).hasProperty('prefix');
                test.must(p.prefix()).be.a.string();
                test.must(p.prefix()).be.equal(prefix);
                done();
            });

            it('has getEntries method', function(done) {
                test.object(p).hasProperty('getEntries');
                test.must(p.getEntries).be.a.function();
                done();
            });

            it('getEntries error', function(done) {
                p.getEntries(10, function(err, aliases) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();

                    test.must(err.informational).be.boolean();

                    done();
                });
            });

            it('getEntries should say invalid argument when maxCount is missing', function(done) {
                p.getEntries(function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });

            it('getEntries should say entry not found and get an empty alias list', function(done) {
                p.getEntries(10, function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });
        }); // object

        // NOTE: prefix works only in persistent mode
        describe('with blobs', function() {
            var matchingAliases = [ prefix + '1', prefix + '2', prefix + '3', prefix + '4' ];

            before('put blobs', function(done) {
                var promises = matchingAliases.map(function(a) {
                    return new Promise(function(resolve, reject) {
                        cluster.blob(a).update(new Buffer('prefix_blob_content'), function(err) {
                            test.must(err).be.equal(null);
                            if (err) reject(err);

                            resolve();
                        });
                    });
                });

                Promise.all(promises)
                    .then(function() {
                        done();
                    })
                    .catch(console.error);
            });

            it('getEntries should return a non-empty alias list', function(done) {
                p.getEntries(/*maxCount=*/100, function(err, aliases) {
                    test.must(err).be.equal(null);

                    test.array(aliases).isNotEmpty();
                    test.value(aliases.length).isEqualTo(matchingAliases.length);
                    test.array(aliases).contains(matchingAliases);
                    test.array(matchingAliases).contains(aliases);

                    done();
                });
            });

            it('getEntries should return a shortened alias list', function(done) {
                p.getEntries(/*maxCount=*/2, function(err, aliases) {
                    test.must(err).be.equal(null);

                    test.array(aliases).isNotEmpty();
                    test.value(aliases.length).isEqualTo(2);
                    test.array(matchingAliases).contains(aliases);

                    done();
                });
            });
        }); // with blobs
    });     // prefix

    describe('range', function() {
        var r = null;

        before('init', function(done) {
            r = cluster.range();
            done();
        });

        describe('object', function () {
            it('is of correct type', function(done) {
                test.object(r).isInstanceOf(qdb.Range);
                done();
            });

            it('has not alias property', function(done) {
                test.object(r).hasNotProperty('alias');
                done();
            });
        });

        describe('blobScan', function() {
            var scan_pattern = 'unexisting_pattern';

            it('method exists', function(done) {
                test.object(r).hasProperty('blobScan');
                test.must(r.blobScan).be.a.function();
                done();
            });

            it('error type is correct', function(done) {
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

            it('should say invalid argument when pattern is missing', function(done) {
                r.blobScan(function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });

            it.skip('should say invalid argument when maxCount is missing', function(done) {
                r.blobScan(scan_pattern, function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });

            it('should say entry not found and get an empty alias list', function(done) {
                r.blobScan(scan_pattern, 10, function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });
        }); // blobScan

        describe('blobScanRegex', function() {
            var scan_pattern = 'unexisting_pattern';

            it('method exists', function(done) {
                test.object(r).hasProperty('blobScanRegex');
                test.must(r.blobScanRegex).be.a.function();
                done();
            });

            it('error type is correct', function(done) {
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

            it('should say invalid argument when pattern is missing', function(done) {
                r.blobScanRegex(function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });

            it.skip('should say invalid argument when maxCount is missing', function(done) {
                r.blobScanRegex(scan_pattern, function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });

            it('should say entry not found and get an empty alias list', function(done) {
                r.blobScanRegex(scan_pattern, 10, function(err, aliases) {
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();

                    aliases.must.be.empty();

                    done();
                });
            });
        }); // blobScanRegex

        describe('with blobs', function() {
            var alias = 'range';
            var content = 'pattern';
            var matchingAliases = [ alias + '1', alias + '2', alias + '3', alias + '4' ];

            before('put blobs', function(done) {
                var promises = matchingAliases.map(function(a) {
                    return new Promise(function(resolve, reject) {
                        cluster.blob(a).update(new Buffer(content + a), function(err) {
                            test.must(err).be.equal(null);
                            if (err) reject(err);

                            resolve();
                        });
                    });
                });

                Promise.all(promises)
                    .then(function() {
                        done();
                    })
                    .catch(console.error);
            });

            describe('blobScan', function() {
                var scan_pattern = content;

                it('should return a non-empty alias list', function(done) {
                    r.blobScan(scan_pattern, /*maxCount=*/100, function(err, aliases) {
                        test.must(err).be.equal(null);

                        test.array(aliases).isNotEmpty();
                        test.value(aliases.length).isEqualTo(matchingAliases.length);
                        test.array(aliases).contains(matchingAliases);
                        test.array(matchingAliases).contains(aliases);

                        done();
                    });
                });

                it('should return a shortened alias list', function(done) {
                    r.blobScan(scan_pattern, /*maxCount=*/2, function(err, aliases) {
                        test.must(err).be.equal(null);

                        test.array(aliases).isNotEmpty();
                        test.value(aliases.length).isEqualTo(2);
                        test.array(matchingAliases).contains(aliases);

                        done();
                    });
                });
            });

            describe('blobScanRegex', function() {
                var scan_pattern = content + '[a-z]*[0-9]+';

                it('should return a non-empty alias list', function(done) {
                    r.blobScanRegex(scan_pattern, /*maxCount=*/100, function(err, aliases) {
                        test.must(err).be.equal(null);

                        test.array(aliases).isNotEmpty();
                        test.value(aliases.length).isEqualTo(matchingAliases.length);
                        test.array(aliases).contains(matchingAliases);
                        test.array(matchingAliases).contains(aliases);

                        done();
                    });
                });

                it('should return a shortened alias list', function(done) {
                    r.blobScanRegex(scan_pattern, /*maxCount=*/2, function(err, aliases) {
                        test.must(err).be.equal(null);

                        test.array(aliases).isNotEmpty();
                        test.value(aliases.length).isEqualTo(2);
                        test.array(matchingAliases).contains(aliases);

                        done();
                    });
                });
            });
        }); // with blobs
    });     // range

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

    describe('blob', function () {
        var b = null;

        before('init', function (done) {
            b = cluster.blob('bam');
            done();
        });

        it('is of correct type', function (done) {
            test.object(b).isInstanceOf(qdb.Blob);
            done();
        });

        it('has alias property', function (done) {
            test.object(b).hasProperty('alias');
            test.must(b.alias()).be.a.string();
            test.must(b.alias()).be.equal('bam');
            done();
        });

        describe('update/get/delete/put/get/delete', function() {
            var tagName = 'myTag';

            it('should say entry not found and get an empty tag list', function (done) {
                b.getTags(function (err, tags) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.false();

                    tags.must.be.empty();

                    done();
                });

            });

            it('should update without error', function (done) {
                b.update(new Buffer('bam_content'), function (err) {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should return correct entry type', function (done) {
                b.getType(function (err, type) {
                    test.must(err).be.equal(null);
                    test.must(type).be.a.number();
                    test.must(type).be.equal(qdb.ENTRY_BLOB);

                    done();
                });
            });

            it('should get the previous value', function (done) {
                b.get(function (err, data) {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('bam_content');

                    done();
                });
            });

            it('should remove without error', function (done) {
                b.remove(function (err) {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should put again without an error', function (done) {
                b.put(new Buffer('boom_content'), function (err) {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should get the new value', function (done) {
                b.get(function (err, data) {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('boom_content');

                    done();
                });
            });

            it('should get an empty tag list', function (done) {
                b.getTags(function (err, tags) {
                    test.must(err).be.equal(null);

                    tags.must.have.length(0);

                    done();
                });

            });

            it('should not find the tag', function (done) {
                b.hasTag(tagName, function (err) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_TAG_NOT_SET);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.true();

                    done();
                });

            });

            it('should tag without an error', function (done) {
                b.addTag(tagName, function (err) {
                    test.must(err).be.equal(null);

                    done();
                });

            });

            it('should tag again with an error', function (done) {
                b.addTag(tagName, function (err) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_TAG_ALREADY_SET);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.true();

                    done();
                });

            });

            it('should find the tag', function (done) {
                b.hasTag(tagName, function (err) {
                    test.must(err).be.equal(null);

                    done();
                });

            });

            it('should get a tag list with the tag', function (done) {
                b.getTags(function (err, tags) {
                    test.must(err).be.equal(null);

                    tags.must.have.length(1);
                    tags[0].must.be.equal(tagName);

                    done();
                });

            });


            it('should remove the tag without an error', function (done) {
                b.removeTag(tagName, function (err) {
                    test.must(err).be.equal(null);

                    done();
                });

            });

            it('should remove the tag again with an error', function (done) {
                b.removeTag(tagName, function (err) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_TAG_NOT_SET);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.true();

                    done();
                });

            });

            it('should not find the tag', function (done) {
                b.hasTag(tagName, function (err) {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_TAG_NOT_SET);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.true();

                    done();
                });

            });

            it('should get an empty tag list again', function (done) {
                b.getTags(function (err, tags) {
                    test.must(err).be.equal(null);

                    tags.must.have.length(0);

                    done();
                });

            });

            it('should remove without error', function (done) {
                b.remove(function (err) {
                    test.must(err).be.equal(null);

                    done();
                });
            });

        }); // update/get/delete
    });

    // work on a different blob
    describe('blob', function() {
        var b = null;

        before('init', function(done) {
            b = cluster.blob('expiry_bam');
            done();
        });

        describe('expiry', function()
        {
            it('constants should be defined', function()
            {
                test.must(qdb.NEVER_EXPIRES).be.a.number();
                test.must(qdb.PRESERVE_EXPIRATION).be.a.number();
            });

            it('should update with an expiry in 2s without error', function(done)
            {
                var test_exp = new Date();
                test_exp.setSeconds(test_exp.getSeconds() + 2);

                b.update(new Buffer('bam_content'), test_exp, function(err)
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
                b.get(function(err, data)
                {
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

            it('should put with an expiry in 2030', function(done)
            {
                var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                b.put(new Buffer('bam_content'), test_exp, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });

            });

            it('should return the exact supplied expiry', function(done)
            {
                var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                b.getExpiry(function(err, entry_expiry)
                {
                    test.must(err).be.equal(null);

                    test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                    done();
                });
            });

            it('should leave the expiry untouched', function(done)
            {
                b.update(new Buffer('bim_content'), qdb.PRESERVE_EXPIRATION, function(err)
                {
                    test.must(err).be.equal(null);
                    done();
                });

            });

            it('should return the untouched expiry', function(done)
            {
                var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                b.getExpiry(function(err, entry_expiry)
                {
                    test.must(err).be.equal(null);

                    test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                    done();
                });
            });

            it('should set the expiry to 2040', function(done)
            {
                var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                b.expiresAt(test_exp, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });

            });

            it('should return the new supplied expiry', function(done)
            {
                var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                b.getExpiry(function(err, entry_expiry)
                {
                    test.must(err).be.equal(null);

                    test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                    done();
                });
            });

            it('should expire in 2 seconds from now', function(done)
            {
                b.expiresFromNow(2, function(err)
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
                b.get(function(err, data)
                {
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

        }); // expiry

    }); // blob

    describe('tag', function() {
        var dasTag = 'u96';
        var t = null;

        before('init', function(done) {
            t = cluster.tag(dasTag);
            done();
        });

        it('is of correct type', function(done) {
            test.object(t).isInstanceOf(qdb.Tag);
            done();
        });

        it('has alias property', function(done) {
            test.object(t).hasProperty('alias');
            test.must(t.alias()).be.a.string();
            test.must(t.alias()).be.equal(dasTag);
            done();
        });

        describe('entries list test', function() {
            var b, i, q, s;

            before('init', function(done) {
                b = cluster.blob('blob_tag_test');
                i = cluster.integer('int_tag_test');
                q = cluster.deque('deque_tag_test');
                s = cluster.set('set_tag_test');

                done();
            });

            it('should return an empty tag list', function(done) {
                t.getEntries(function(err, entries)
                {
                    test.must(err).be.equal(null);

                    entries.must.have.length(0);

                    done();
                });
            });

            it('should create the blob for tag test', function(done)
            {
                b.put(new Buffer('untz'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should create the integer for tag test', function(done)
            {
                i.update(0, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should create the deque for tag test', function(done)
            {
                q.pushBack(new Buffer('mmm ok'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should create the set for tag test', function(done)
            {
                s.insert(new Buffer('b00m'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should tag the blob successfully', function(done)
            {
                b.addTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should tag the integer successfully', function(done)
            {
                i.addTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should tag the deque successfully', function(done)
            {
                q.addTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should tag the set successfully', function(done)
            {
                s.addTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should return correct entry type', function (done) {
                t.getType(function (err, type) {
                    test.must(err).be.equal(null);
                    test.must(type).be.a.number();
                    test.must(type).be.equal(qdb.ENTRY_TAG);

                    done();
                });
            });

            it('should return the whole list of entries', function(done)
            {
                t.getEntries(function(err, entries)
                {
                    test.must(err).be.equal(null);

                    entries.must.have.length(4);

                    test.must(entries.indexOf('blob_tag_test')).be.gte(0);
                    test.must(entries.indexOf('int_tag_test')).be.gte(0);
                    test.must(entries.indexOf('deque_tag_test')).be.gte(0);
                    test.must(entries.indexOf('set_tag_test')).be.gte(0);

                    done();
                });
            });

            it('should untag the blob successfully', function(done)
            {
                b.removeTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should untag the integer successfully', function(done)
            {
                i.removeTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should untag the deque successfully', function(done)
            {
                q.removeTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should untag the set successfully', function(done)
            {
                s.removeTag(dasTag, function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should return an empty tag list again', function(done)
            {
                t.getEntries(function(err, entries)
                {
                    test.must(err).be.equal(null);

                    entries.must.have.length(0);

                    done();
                });
            });

            it('should remove the blob for tag test', function(done)
            {
                b.remove(function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should remove the integer for tag test', function(done)
            {
                i.remove(function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should remove the deque for tag test', function(done)
            {
                q.remove(function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should remove the set for tag test', function(done)
            {
                s.remove(function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

        }); // entries list test

        describe('multiple tags', function() {
            var tags = [ 'line720_0', 'line720_1' ];
            var b = null;
            var ts = null;

            before('init', function(done) {
                b = cluster.blob('blob_tag_test');
                ts = tags.map(function(t) {
                    return cluster.tag(t);
                });
                done();
            });

            it('should return an empty tag list', function(done) {
                ts.forEach(function (t)
                {
                    t.getEntries(function (err, entries)
                    {
                        test.must(err).be.equal(null);

                        entries.must.have.length(0);
                    });
                });

                done();
            });

            it('should return false (tag not set) for every tag', function(done)
            {
                b.hasTags(tags, function (err, success_count, result)
                {
                    test.must(err).be.equal(null);
                    test.must(success_count).be.equal(0);

                    tags.forEach(function (t)
                    {
                        test.must(result[t]).be.false();
                    });

                    done();
                });
            });

            it('should update the blob for tag test', function(done)
            {
                b.update(new Buffer('untz'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            describe('addTags', function ()
            {

                it('should add multiple tags successfully', function(done)
                {
                    b.addTags(tags, function(err)
                    {
                        test.must(err).be.equal(null);

                        done();
                    });
                });

                it('should find the tags', function(done)
                {
                    tags.forEach(function (tagName)
                    {
                        b.hasTag(tagName, function(err)
                        {
                            test.must(err).be.equal(null);
                        });
                    });

                    done();
                });

                it('should return the whole list of entries', function(done)
                {
                    ts.forEach(function (t)
                    {
                        t.getEntries(function(err, entries)
                        {
                            test.must(err).be.equal(null);

                            entries.must.have.length(1);

                            test.must(entries.indexOf('blob_tag_test')).be.gte(0);
                        });
                    });

                    done();
                });

            }); // addTags

            describe.skip('hasTags', function ()
            {

                it('should return true (tag set) for every tag', function(done)
                {
                    b.hasTags(tags, function (err, success_count, result)
                    {
                        test.must(err).be.equal(null);
                        test.must(success_count).be.equal(tags.length);

                        tags.forEach(function (t)
                        {
                            test.must(result[t]).be.true();
                        });

                        done();
                    });
                });

            }); // hasTags

            describe('removeTags', function ()
            {

                it('should remove multiple tags successfully', function(done)
                {
                    b.removeTags(tags, function(err)
                    {
                        test.must(err).be.equal(null);

                        done();
                    });
                });

                it('should return an empty tag list', function(done)
                {
                    ts.forEach(function (t)
                    {
                        t.getEntries(function (err, entries)
                        {
                            test.must(err).be.equal(null);

                            entries.must.have.length(0);
                        });
                    });

                    done();
                });

            }); // removeTags

            describe('hasTags', function ()
            {

                it('should return false (tag not set) for every tag', function(done)
                {
                    b.hasTags(tags, function (err, success_count, result)
                    {
                        test.must(err).be.equal(null);
                        test.must(success_count).be.equal(0);

                        tags.forEach(function (t)
                        {
                            test.must(result[t]).be.false();
                        });

                        done();
                    });
                });

            }); // hasTags

        }); // multiple tags

    }); // tag

    describe('integer', function () {
        var i;

        before('init', function(done) {
            i = cluster.integer('int_test');
            done();
        });

        it('is of correct type', function(done) {
            test.object(i).isInstanceOf(qdb.Integer);
            done();
        });

        it('has alias property', function(done) {
            test.object(i).hasProperty('alias');
            test.must(i.alias()).be.a.string();
            test.must(i.alias()).be.equal('int_test');
            done();
        });

        describe('update/add/add/remove', function() {
            it('should set the value to 0', function(done) {
                i.update(0, function(err) {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should return correct entry type', function(done) {
                i.getType(function(err, type) {
                    test.must(err).be.equal(null);
                    test.must(type).be.a.number();
                    test.must(type).be.equal(qdb.ENTRY_INTEGER);

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

            before('init', function(done) {
                i = cluster.integer('expiry_int');
                done();
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

        }); // expiry

    }); // integer

    describe('deque', function() {
        var q;

        before('init', function(done) {
            q = cluster.deque('q1');
            done();
        });

        it('is of correct type', function(done) {
            test.object(q).isInstanceOf(qdb.Deque);
            done();
        });

        it('has alias property', function(done) {
            test.object(q).hasProperty('alias');
            test.must(q.alias()).be.a.string();
            test.must(q.alias()).be.equal('q1');
            done();
        });

        // some back push/pop
        // note that at every moment you can have back() and front() (these functions do not pop)
        describe('push back/pop front/push front/pop back', function()
        {

            it('should not get entry 0', function(done)
            {
                q.getAt(0, function(err, data)
                {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();
                    done();
                });
            });

            it('should not set entry 0', function(done)
            {
                q.setAt(0, Buffer('a'), function(err)
                {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();
                    done();
                });
            });

            it('should not find the size', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                    test.must(err.informational).be.false();

                    done();
                });
            });

            it('should push back without error', function(done)
            {
                q.pushBack(new Buffer('a'), function(err)
                {
                    test.must(err).be.equal(null);
                    done();
                });
            });

            it('should return correct entry type', function (done) {
                q.getType(function (err, type) {
                    test.must(err).be.equal(null);
                    test.must(type).be.a.number();
                    test.must(type).be.equal(qdb.ENTRY_DEQUE);

                    done();
                });
            });

            it('should have a size of 1', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err).be.equal(null);

                    test.must(s).be.a.number();
                    test.must(s).be.equal(1);

                    done();
                });
            });

            it('should get entry 0', function(done)
            {
                q.getAt(0, function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('a');

                    done();
                });
            });

            it('should set entry 0', function(done)
            {
                q.setAt(0, Buffer('c'), function (err)
                {
                    test.must(err).be.equal(null);
                    done();
                });

            });

            it('should get updated value of entry 0', function(done)
            {
                q.getAt(0, function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('c');

                    done();
                });
            });

            it('should not get entry 1', function(done)
            {
                q.getAt(1, function(err, data)
                {
                    test.must(err.message).be.a.string();
                    test.must(err.message).not.be.empty();
                    test.must(err.code).be.a.number();
                    test.must(err.code).be.equal(qdb.E_OUT_OF_BOUNDS);
                    test.must(err.severity).be.a.number();
                    test.must(err.origin).be.a.number();
                    test.must(err.informational).be.false();

                    done();
                });
            });

            it('back should be equal to the value we inserted', function(done)
            {
                q.back(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('c');

                    done();
                });
            });

            it('front should be equal to the value we inserted', function(done)
            {
                q.front(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('c');

                    done();
                });
            });

            it('should pop front the previous value', function(done)
            {
                q.popFront(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('c');

                    done();
                });
            });

            it('should have a size of 0', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err).be.equal(null);

                    test.must(s).be.a.number();
                    test.must(s).be.equal(0);

                    done();
                });
            });

            it('should push front without error', function(done)
            {
                q.pushFront(new Buffer('b'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should have a size of 1', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err).be.equal(null);

                    test.must(s).be.a.number();
                    test.must(s).be.equal(1);

                    done();
                });
            });

            it('should push front without error', function(done)
            {
                q.pushFront(new Buffer('d'), function(err)
                {
                    test.must(err).be.equal(null);

                    done();
                });
            });

            it('should have a size of 2', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err).be.equal(null);

                    test.must(s).be.a.number();
                    test.must(s).be.equal(2);

                    done();
                });
            });

            it('should get entry 1', function(done)
            {
                q.getAt(1, function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('b');

                    done();
                });
            });

            it('back should be equal to the new value we inserted', function(done)
            {
                q.back(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('b');

                    done();
                });
            });

            it('front should be equal to the new value we inserted', function(done)
            {
                q.front(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('d');

                    done();
                });
            });

            it('should pop back the previous value', function(done)
            {
                q.popBack(function(err, data)
                {
                    test.must(err).be.equal(null);

                    test.must(data.toString()).be.equal('b');

                    done();
                });
            });

            it('should have a size of 1', function(done)
            {
                q.size(function(err, s)
                {
                    test.must(err).be.equal(null);

                    test.must(s).be.a.number();
                    test.must(s).be.equal(1);

                    done();
                });
            });

        });
    }); // deque

    describe('set', function() {
        var s;

        before('init', function(done) {
            s = cluster.set('s1');
            done();
        });

        it('is of correct type', function(done) {
            test.object(s).isInstanceOf(qdb.Set);
            done();
        });

        it('has alias property', function(done) {
            test.object(s).hasProperty('alias');
            test.must(s.alias()).be.a.string();
            test.must(s.alias()).be.equal('s1');
            done();
        });

        // some back push/pop
        // note that at every moment you can have back() and front() (these functions do not pop)

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
                s.getType(function(err, type) {
                    test.must(err).be.equal(null);
                    test.must(type).be.a.number();
                    test.must(type).be.equal(qdb.ENTRY_HSET);

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

}); // quasardb

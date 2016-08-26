var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.cluster_uri);

describe('Blob', function () {
    var b = null;

    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function () {
        b = cluster.blob('bam');
    });

    it('should be of correct type', function () {
        test.object(b).isInstanceOf(qdb.Blob);
    });

    it("should have an 'alias' property", function () {
        test.object(b).hasProperty('alias');
        test.must(b.alias()).be.a.string();
        test.must(b.alias()).be.equal('bam');
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
            b.getMetadata(function (err, meta) {
                test.must(err).be.equal(null);
                test.must(meta.type).be.a.number();
                test.must(meta.type).be.equal(qdb.ENTRY_BLOB);

                done();
            });
        });

        it('should return correct entry metadata', function (done) {
            b.getMetadata(function (err, meta) {
                test.must(err).be.equal(null);

                describe('reference', function() {
                    test.object(meta).hasProperty('reference');
                    test.object(meta.reference).isInstanceOf(Array);
                    test.must(meta.reference.length).be.equal(4);
                });

                describe('size', function() {
                    test.object(meta).hasProperty('size');
                    test.must(meta.size).be.a.number();
                    test.must(meta.size).be.equal('bam_content'.length);
                });

                describe('type', function() {
                    test.object(meta).hasProperty('type');
                    test.must(meta.type).be.a.number();
                    test.must(meta.type).be.equal(qdb.ENTRY_BLOB);
                });

                describe('creation_time', function() {
                    test.object(meta).hasProperty('creation_time');
                    if (meta.creation_time != undefined) test.object(meta.creation_time).isInstanceOf(Date);
                });

                describe('modification_time', function() {
                    test.object(meta).hasProperty('modification_time');
                    test.object(meta.modification_time).isInstanceOf(Date);
                });

                describe('expiry_time', function() {
                    test.object(meta).hasProperty('expiry_time');
                    if (meta.expiry_time != undefined) test.object(meta.expiry_time).isInstanceOf(Date);
                });

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
            b.attachTag(tagName, function (err) {
                test.must(err).be.equal(null);

                done();
            });

        });

        it('should tag again with an error', function (done) {
            b.attachTag(tagName, function (err) {
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
            b.detachTag(tagName, function (err) {
                test.must(err).be.equal(null);

                done();
            });

        });

        it('should remove the tag again with an error', function (done) {
            b.detachTag(tagName, function (err) {
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

    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function() {
        b = cluster.blob('expiry_bam');
    });

    describe('expiry', function()
    {
        it('constants should be defined', function()
        {
            test.must(qdb.NEVER_EXPIRES).be.a.number();
            test.must(qdb.PRESERVE_EXPIRATION).be.a.number();
        });

        it('should put without expiry', function(done)
        {
            b.put(new Buffer('bam_content'), function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return undefined expiry', function(done)
        {
            b.getExpiry(function(err, entry_expiry)
            {
                test.must(err).be.equal(null);
                test.must(entry_expiry).be.equal(undefined);

                done();
            });
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

        it('should update without expiry', function(done)
        {
            b.update(new Buffer('bam_content'), qdb.NEVER_EXPIRES, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return undefined expiry', function(done)
        {
            b.getExpiry(function(err, entry_expiry)
            {
                test.must(err).be.equal(null);
                test.must(entry_expiry).be.equal(undefined);

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

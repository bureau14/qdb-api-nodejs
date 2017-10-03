var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.cluster_uri);


describe('Tag', function() {
    var dasTag = 'u96';
    var t = null;

    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function() {
        t = cluster.tag(dasTag);
    });

    it('should be of correct type', function() {
        test.object(t).isInstanceOf(qdb.Tag);
    });

    it("should have an 'alias' property", function() {
        test.object(t).hasProperty('alias');
        test.must(t.alias()).be.a.string();
        test.must(t.alias()).be.equal(dasTag);
    });

    describe('entries list test', function() {
        var b, i, q, s, ts;

        before('init', function() {
            b = cluster.blob('blob_tag_test');
            i = cluster.integer('int_tag_test');
            q = cluster.deque('deque_tag_test');
            s = cluster.set('set_tag_test');
            ts = cluster.ts('time_series_tag_test');
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

        it('should create the time series double columns for tag test', function(done)
        {
			ts.create([qdb.DoubleColumnInfo("col1")], function(err) {
				test.must(err).be.equal(null);

				done();
			});
        });

        it('should tag the blob successfully', function(done)
        {
            b.attachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should tag the integer successfully', function(done)
        {
            i.attachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should tag the deque successfully', function(done)
        {
            q.attachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should tag the set successfully', function(done)
        {
            s.attachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should tag the time series successfully', function(done)
        {
            ts.attachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return correct entry type', function (done) {
            t.getMetadata(function (err, meta) {
                test.must(err).be.equal(null);
                test.must(meta.type).be.a.number();
                test.must(meta.type).be.equal(qdb.ENTRY_TAG);

                done();
            });
        });

        it('should return the whole list of entries', function(done)
        {
            t.getEntries(function(err, entries)
            {
                test.must(err).be.equal(null);

                entries.must.have.length(5);

                test.must(entries.indexOf('blob_tag_test')).be.gte(0);
                test.must(entries.indexOf('int_tag_test')).be.gte(0);
                test.must(entries.indexOf('deque_tag_test')).be.gte(0);
                test.must(entries.indexOf('set_tag_test')).be.gte(0);
                test.must(entries.indexOf('time_series_tag_test')).be.gte(0);

                done();
            });
        });

        it('should untag the blob successfully', function(done)
        {
            b.detachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should untag the integer successfully', function(done)
        {
            i.detachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should untag the deque successfully', function(done)
        {
            q.detachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should untag the set successfully', function(done)
        {
            s.detachTag(dasTag, function(err)
            {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should untag the time series successfully', function(done)
        {
            ts.detachTag(dasTag, function(err)
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

        it('should remove the time series for tag test', function(done)
        {
            ts.remove(function(err)
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

        before('init', function() {
            b = cluster.blob('blob_tag_test');
            ts = tags.map(function(t) {
                return cluster.tag(t);
            });
        });

        it('should return an empty tag list', function() {
            ts.forEach(function (t)
            {
                t.getEntries(function (err, entries)
                {
                    test.must(err).be.equal(null);

                    entries.must.have.length(0);
                });
            });
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

        describe('attachTags', function ()
        {

            it('should add multiple tags successfully', function(done)
            {
                b.attachTags(tags, function(err)
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

        }); // attachTags

        describe('hasTags', function ()
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

        describe('detachTags', function ()
        {

            it('should remove multiple tags successfully', function(done)
            {
                b.detachTags(tags, function(err)
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

        }); // detachTags

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

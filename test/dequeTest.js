var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.cluster_uri);

describe('Deque', function() {
    var q;

    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function() {
        q = cluster.deque('q1');
    });

    it('should be of correct type', function() {
        test.object(q).isInstanceOf(qdb.Deque);
    });

    it("shoudld have an 'alias' property", function() {
        test.object(q).hasProperty('alias');
        test.must(q.alias()).be.a.string();
        test.must(q.alias()).be.equal('q1');
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
            q.getMetadata(function (err, meta) {
                test.must(err).be.equal(null);
                test.must(meta.type).be.a.number();
                test.must(meta.type).be.equal(qdb.ENTRY_DEQUE);

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

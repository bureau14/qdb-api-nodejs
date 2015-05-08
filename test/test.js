// test.js

var test = require('unit.js');

var qdb = require('../');

var c = new qdb.Cluster('qdb://127.0.0.1:2836');

var b = c.blob('bam');

test.must(b.alias()).be.a.string();
test.must(b.alias()).be.equal('bam');

// put/get/delete sequence
b.put(function(err, data)
{
    test.must(err).be.a.number();
    test.must(err).be.equal(0);

    b.get(function(err, data)
    {
        test.must(err).be.a.number();
        test.must(err).be.equal(0);

        test.must(data.toString()).be.equal('bam_content');

        b.remove(function(err, data)
        {
            test.must(err).be.a.number();
            test.must(err).be.equal(0);
        });
    });

}, new Buffer('bam_content'));

// integer sequence

var i = c.integer('int_test');

i.put(function(err, data)
{
    test.must(err).be.a.number();
    test.must(err).be.equal(0);

    i.add(function(err, data)
    {
        test.must(err).be.a.number();
        test.must(err).be.equal(0);

        test.must(data).be.a.number();
        test.must(data).be.equal(42);

        i.remove(function(err, data)
        {
            test.must(err).be.a.number();
            test.must(err).be.equal(0);
        });

    }, 42);

}, 0);

// some back push/pop
// note that at every moment you can have back() and front() (these functions do not pop)
var q = c.queue('q1');

q.pushBack(function(err, data)
{
    test.must(err).be.a.number();
    test.must(err).be.equal(0);

    q.popFront(function(err, data)
    {
        test.must(err).be.a.number();
        test.must(err).be.equal(0);

        test.must(data.toString()).be.equal('a');

        q.pushFront(function(err, data)
        {
            test.must(err).be.a.number();
            test.must(err).be.equal(0);

            q.popBack(function(err, data)
            {
                test.must(err).be.a.number();
                test.must(err).be.equal(0);

                test.must(data.toString()).be.equal('b');
            });

        },
        new Buffer('b'));
    });

}, new Buffer('a'));

// set test sequence
var s = c.set('s1');

s.contains(function(err)
{
    test.must(err).be.a.number();
    test.must(err).be.equal(8);

    s.insert(function(err)
    {
        test.must(err).be.a.number();
        test.must(err).be.equal(0);

        s.contains(function(err)
        {
            test.must(err).be.a.number();
            test.must(err).be.equal(0);

            s.erase(function(err)
            {
                test.must(err).be.a.number();
                test.must(err).be.equal(0);

                s.contains(function(err)
                {
                    test.must(err).be.a.number();
                    test.must(err).be.equal(8);
                }, new Buffer('da'));

            }, new Buffer('da'));

        }, new Buffer('da'));

    }, new Buffer('da'));

}, new Buffer('da'));

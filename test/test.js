// test.js
var spawn = require('child_process').spawn;
var execSync = require('child_process').execSync;
var qdb = require('../build/Release/qdb.node');
var test = require('unit.js');

var qdbd = null;

before('run quasardb daemon', function(done)
{
    this.timeout(5000);
    qdbd = spawn('./deps/qdb/bin/qdbd', ['--transient','--address=127.0.0.1:3030']);
    setTimeout(done, 2000);
});

after('terminate quasardb daemon', function(done)
{
    this.timeout(10000);
    qdbd.kill('SIGTERM');
    qdbd.on('exit', function(code)
    {
        done();
    });
});

describe('qdb', function()
{
    it('test suite', function()
    {
        describe('blob', function()
        {
            var c = new qdb.Cluster('qdb://127.0.0.1:3030');
            test.object(c).isInstanceOf(qdb.Cluster);
            var b = c.blob('bam');
            var tagName = 'myTag';

            test.object(b).isInstanceOf(qdb.Blob);

            test.object(b).hasProperty('alias');
            test.must(b.alias()).be.a.string();
            test.must(b.alias()).be.equal('bam');

            describe('update/get/delete/put/get/delete', function()
            {
                it('should update without error', function(done)
                {
                    b.update(new Buffer('bam_content'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    }); 
                });

                it('should get the previous value', function(done)
                {
                    b.get(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('bam_content');

                        done();
                    });
                });

                it('should remove without error', function(done)
                {
                    b.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should put again without an error', function(done)
                {
                    b.put(new Buffer('boom_content'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    }); 
                });

                 it('should get the new value', function(done)
                {
                    b.get(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('boom_content');

                        done();
                    });
                });

                it('should get an empty tag list', function(done)
                {
                    b.getTags(function(err, tags)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        tags.must.have.length(0);

                        done();
                    });

                });

                it('should not find the tag', function(done)
                {
                    b.hasTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(42);

                        done();
                    });

                });         

                it('should tag without an error', function(done)
                {
                    b.addTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should tag again with an error', function(done)
                {
                    b.addTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(41);
                        
                        done();
                    });

                });

                it('should find the tag', function(done)
                {
                    b.hasTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });

                });    

                it('should get a tag list with the tag', function(done)
                {
                    b.getTags(function(err, tags)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        tags.must.have.length(1);
                        tags[0].must.be.equal(tagName);

                        done();
                    });

                });


                it('should remove the tag without an error', function(done)
                {
                    b.removeTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should remove the tag again with an error', function(done)
                {
                    b.removeTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(42);
                        
                        done();
                    });

                });

                it('should not find the tag', function(done)
                {
                    b.hasTag(tagName, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(42);

                        done();
                    });

                });  

                it('should get an empty tag list again', function(done)
                {
                    b.getTags(function(err, tags)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        tags.must.have.length(0);

                        done();
                    });

                });

                it('should remove without error', function(done)
                {
                    b.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });     

            }); // update/get/delete

            // work on a different blob
            b = c.blob('expiry_bam');

            describe('expiry', function()
            {
                it('should update with an expiry in 2s without error', function(done)
                {
                    var test_exp = new Date();
                    test_exp.setSeconds(test_exp.getSeconds() + 2);

                    b.update(new Buffer('bam_content'), test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
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
                        test.must(err).be.a.number();
                        test.must(err).be.equal(8);

                        done();
                    });
                });

                it('should put with an expiry in 2030', function(done)
                {
                    var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                    b.put(new Buffer('bam_content'), test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should return the exact supplied expiry', function(done)
                {
                    var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                    b.getExpiry(function(err, entry_expiry)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                        done();
                    });
                });

                it('should set the expiry to 2040', function(done)
                {
                    var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                    b.expiresAt(test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should return the new supplied expiry', function(done)
                {
                    var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                    b.getExpiry(function(err, entry_expiry)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                        done();
                    });
                });

                it('should expire in 2 seconds from now', function(done)
                {
                    b.expiresFromNow(2, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
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
                        test.must(err).be.a.number();
                        test.must(err).be.equal(8);

                        done();
                    });
                });

            }); // expiry

        }); // blob

        describe('tag', function()
        {
            var dasTag = 'u96';
            var c = new qdb.Cluster('qdb://127.0.0.1:3030');
            test.object(c).isInstanceOf(qdb.Cluster);
            var t = c.tag(dasTag);

            test.object(t).isInstanceOf(qdb.Tag);

            test.object(t).hasProperty('alias');
            test.must(t.alias()).be.a.string();
            test.must(t.alias()).be.equal(dasTag);  

            describe('entries list test', function()
            {
                var b = c.blob('blob_tag_test');
                var i = c.integer('int_tag_test');
                var q = c.queue('queue_tag_test');
                var s = c.set('set_tag_test');

                it('should return an empty tag list', function(done)
                {
                    t.getEntries(function(err, entries)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        entries.must.have.length(0);

                        done();
                    });
                });

                it('should create the blob for tag test', function(done)
                {
                    b.put(new Buffer('untz'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should create the integer for tag test', function(done)
                {
                    i.update(0, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

                it('should create the queue for tag test', function(done)
                {
                    q.pushBack(new Buffer('mmm ok'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

                it('should create the set for tag test', function(done)
                {
                    s.insert(new Buffer('b00m'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

                it('should tag the blob successfully', function(done)
                {
                    b.addTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should tag the integer successfully', function(done)
                {
                    i.addTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should tag the queue successfully', function(done)
                {
                    q.addTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should tag the set successfully', function(done)
                {
                    s.addTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should return the whole list of entries', function(done)
                {
                    t.getEntries(function(err, entries)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        entries.must.have.length(4);

                        test.must(entries.indexOf('blob_tag_test')).be.gte(0);
                        test.must(entries.indexOf('int_tag_test')).be.gte(0);
                        test.must(entries.indexOf('queue_tag_test')).be.gte(0);
                        test.must(entries.indexOf('set_tag_test')).be.gte(0);

                        done();
                    });
                });

                it('should untag the blob successfully', function(done)
                {
                    b.removeTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should untag the integer successfully', function(done)
                {
                    i.removeTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should untag the queue successfully', function(done)
                {
                    q.removeTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should untag the set successfully', function(done)
                {
                    s.removeTag(dasTag, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();   
                    });
                });

                it('should return an empty tag list again', function(done)
                {
                    t.getEntries(function(err, entries)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        entries.must.have.length(0);

                        done();
                    });
                });

                it('should remove the blob for tag test', function(done)
                {
                    b.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should remove the integer for tag test', function(done)
                {
                    i.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

                it('should remove the queue for tag test', function(done)
                {
                    q.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

                it('should remove the set for tag test', function(done)
                {
                    s.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();                  
                    });
                });

            });  // entries list test

        }); // tag

        describe('integer', function()
        {
            var c = new qdb.Cluster('qdb://127.0.0.1:3030');
            test.object(c).isInstanceOf(qdb.Cluster);
            var i = c.integer('int_test');

            test.object(i).isInstanceOf(qdb.Integer);

            test.object(i).hasProperty('alias');
            test.must(i.alias()).be.a.string();
            test.must(i.alias()).be.equal('int_test');

            describe('update/add/add/remove', function()
            {
                it('should set the value to 0', function(done)
                {
                    i.update(0, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should after add be equal to 42', function(done)
                {
                    i.add(42, function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data).be.a.number();
                        test.must(data).be.equal(42);

                        done();
                    });
                });

                it('should after add be equal to 20', function(done)
                {
                    i.add(-22, function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data).be.a.number();
                        test.must(data).be.equal(20);

                        done();
                    });
                });

                it('should remove without an error', function(done)
                {
                    i.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should set the value to 10', function(done)
                {
                    i.put(10, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should get the value of 10', function(done)
                {
                    i.get(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data).be.a.number();
                        test.must(data).be.equal(10);

                        done();
                    });
                });

                it('should remove again without an error', function(done)
                {
                    i.remove(function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

            }); // update/add/add/remove

            // work on a different integer
            i = c.integer('expiry_int');

            describe('expiry', function()
            {
                it('should update with an expiry in 2s without error', function(done)
                {
                    var test_exp = new Date();
                    test_exp.setSeconds(test_exp.getSeconds() + 2);

                    i.update(10, test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
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
                        test.must(err).be.a.number();
                        test.must(err).be.equal(8);

                        done();
                    });
                });

                it('should put with an expiry in 2030', function(done)
                {
                    var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                    i.put(20, test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should return the exact supplied expiry', function(done)
                {
                    var test_exp = new Date(2030, 1, 1, 10, 50, 0, 0);

                    i.getExpiry(function(err, entry_expiry)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                        done();
                    });
                });

                it('should set the expiry to 2040', function(done)
                {
                    var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                    i.expiresAt(test_exp, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        done();
                    });

                });

                it('should return the new supplied expiry', function(done)
                {
                    var test_exp = new Date(2040, 1, 1, 10, 50, 0, 0);

                    i.getExpiry(function(err, entry_expiry)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
                        test.must(Math.trunc(entry_expiry.valueOf() / 1000)).be.equal(Math.trunc(test_exp.valueOf() / 1000));

                        done();
                    });
                });

                it('should expire in 2 seconds from now', function(done)
                {
                    i.expiresFromNow(2, function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);
                        
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
                        test.must(err).be.a.number();
                        test.must(err).be.equal(8);

                        done();
                    });
                });

            }); // expiry

        }); // integer

        describe('queue', function()
        {
            var c = new qdb.Cluster('qdb://127.0.0.1:3030');
            test.object(c).isInstanceOf(qdb.Cluster);
            // some back push/pop
            // note that at every moment you can have back() and front() (these functions do not pop)
            var q = c.queue('q1');

            test.object(q).isInstanceOf(qdb.Queue);

            test.object(q).hasProperty('alias');
            test.must(q.alias()).be.a.string();
            test.must(q.alias()).be.equal('q1');

            describe('push back/pop front/push front/pop back', function()
            {
                it('should push back without error', function(done)
                {
                    q.pushBack(new Buffer('a'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('back should be equal to the value we inserted', function(done)
                {
                    q.back(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('a');

                        done();
                    });
                });

                it('front should be equal to the value we inserted', function(done)
                {
                    q.front(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('a');

                        done();
                    });
                });

                it('should pop front the previous value', function(done)
                {
                    q.popFront(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('a');

                        done();
                    });
                });

                it('should push front without error', function(done)
                {
                    q.pushFront(new Buffer('b'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('back should be equal to the new value we inserted', function(done)
                {
                    q.back(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('b');

                        done();
                    });
                });

                it('front should be equal to the new value we inserted', function(done)
                {
                    q.front(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('b');

                        done();
                    });
                });

                it('should pop back the previous value', function(done)
                {
                    q.popBack(function(err, data)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        test.must(data.toString()).be.equal('b');

                        done();
                    });

                });

            });
        }); // queue

        describe('set', function()
        {
            var c = new qdb.Cluster('qdb://127.0.0.1:3030');
            test.object(c).isInstanceOf(qdb.Cluster);
            // some back push/pop
            // note that at every moment you can have back() and front() (these functions do not pop)
            var s = c.set('s1');

            test.object(s).isInstanceOf(qdb.Set);

            test.object(s).hasProperty('alias');
            test.must(s.alias()).be.a.string();
            test.must(s.alias()).be.equal('s1');

            describe('contains/insert/contains/erase/contains', function()
            {
                it('should not contain the entry', function(done)
                {
                    s.contains(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).not.be.equal(0);

                        done();
                    });
                });

                it('should insert without an error', function(done)
                {
                    s.insert(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should insert with an error', function(done)
                {
                    s.insert(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(38);

                        done();
                    });
                });

                it('should now contain the entry', function(done)
                {
                    s.contains(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should erase without an error', function(done)
                {
                    s.erase(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(0);

                        done();
                    });
                });

                it('should no longer contain the entry', function(done)
                {
                    s.contains(new Buffer('da'), function(err)
                    {
                        test.must(err).be.a.number();
                        test.must(err).be.equal(37);

                        done();
                    });
                });
            });
        }); // set

    }); // connect

}); // qdb
    


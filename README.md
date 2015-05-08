# quasardb node.js API

This API is still experimental and poorly documented. Use at your own risks!

quasardb 2.0 or later required. You need a C++ 11 compiler to compile this addon.

## Introduction

Using *quasardb* cluster from a PHP program is extremely straightforward, just create a Cluster and perform the operations.

```javascript
    var qdb = require('./qdb');

    var c = new qdb.Cluster('qdb://127.0.0.1:2836');
```

OK, now that we have a connection to the cluster, let's store some [binary data](doc/QdbBlob.md):

```javascript
    var b = c.blob('bam');

    b.put(function(err, data) { /* */  }, new Buffer("boom"));
    b.get(function(err, data) { /* */  });
```

Want a queue?

```javascript
    var q = c.queue('q2');

    q.pushBack(function(err, data) { /* */ }, new Buffer("boom"));
    q.popFront(function(err, data) { /* */ });
    q.pushFront(function(err, data) { /* */ }, new Buffer("bang"));
```

Want atomic integers now?

```javascript
    var i = c.integer('some_int');
    i.put(function(err, data){ /* */}, 3);
    i.add(function(err, data){ /* */}, 7);
```

What else? a set maybe?

```javascript
    var s = c.set('the_set');

    s.insert(function(err, data) { /* */ }, new Buffer("boom"));
```


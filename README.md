# quasardb node.js API

This API is still experimental and poorly documented. Use at your own risks!

quasardb 2.0 or later required. You need a C++ 11 compiler to compile this addon.

## Installation

Make sure the qdb API is installed on your machine and can be found by the compiler (On UNIXes typically /usr/lib or /usr/local/lib) and that you have [node-gyp](https://github.com/TooTallNate/node-gyp) installed.

In the directory run:

```
    node-gyp configure
    node-gyp build
```

You will then find a qdb.node file which is the quasardb addon.

## Introduction

Using *quasardb* starts with a Cluster:

```javascript
    var qdb = require('./qdb');

    var c = new qdb.Cluster('qdb://127.0.0.1:2836');
```

Now that we have a connection to the cluster, let's store some binary data:

```javascript
    var b = c.blob('bam');

    b.put(function(err, data) { /* */  }, new Buffer("boom"));
    b.get(function(err, data) { /* */  });
```

Want a queue? We have distributed queues.

```javascript
    var q = c.queue('q2');

    q.pushBack(function(err, data) { /* */ }, new Buffer("boom"));
    q.popFront(function(err, data) { /* */ });
    q.pushFront(function(err, data) { /* */ }, new Buffer("bang"));
```

quasardb comes out of the box with server-side atomic integers:

```javascript
    var i = c.integer('some_int');
    i.put(function(err, data){ /* */}, 3);
    i.add(function(err, data){ /* */}, 7);
```

We also provide distributed sets:

```javascript
    var s = c.set('the_set');

    s.insert(function(err, data) { /* */ }, new Buffer("boom"));
```


# quasardb node.js API

This API is still experimental and poorly documented. Use at your own risks!

quasardb 2.0 or later required. You need a C++ 11 compiler to compile this addon.

## Installation

## quasardb C API

To build the nodejs API, you will need the C API. It can either be installed on the machine (e.g. on unix in /usr/lib or /usr/local/lib) or you can unpack the C API archive in deps/qdb.

## Building the extension

You will need to have [node-gyp](https://github.com/TooTallNate/node-gyp) installed.

In the directory run:

```
    npm install
```

You will then find a qdb.node file which is the quasardb addon in `build/Release`.

## Introduction

Using *quasardb* starts with a Cluster:

```javascript
    var qdb = require('./qdb');

    var c = new qdb.Cluster('qdb://127.0.0.1:2836');
```

Now that we have a connection to the cluster, let's store some binary data:

```javascript
    var b = c.blob('bam');

    b.put(new Buffer("boom"), function(err, data) { /* */  });
    b.get(function(err, data) { /* */  });
```

Want a queue? We have distributed queues.

```javascript
    var q = c.queue('q2');

    q.pushBack(new Buffer("boom"), function(err, data) { /* */ });
    q.popFront(function(err, data) { /* */ });
    q.pushFront(new Buffer("bang"), function(err, data) { /* */ });
```

quasardb comes out of the box with server-side atomic integers:

```javascript
    var i = c.integer('some_int');
    i.put(3, function(err, data){ /* */});
    i.add(7, function(err, data){ /* */});
```

We also provide distributed sets (this is still an *experimental* feature):

```javascript
    var s = c.set('the_set');

    s.insert(new Buffer("boom"), function(err, data) { /* */ });
```


# quasardb node.js API

This is the official quasardb API, suitable for production. The full documentation is available [here](https://doc.quasardb.net/).

[quasardb](https://www.quasardb.net/) 2.0 or later required. You need a C++ 11 compiler to compile this addon.

## Installation

```
npm install quasardb --save
```

## Introduction

Using *quasardb* starts with a Cluster:

```javascript
var qdb = require('quasardb');

var c = new qdb.Cluster('qdb://127.0.0.1:2836');
```

And creating a connection:

```javascript
c.connect(function() {
        // successfully connected to the cluster
    },
    function(err) {
        // error
    });
```

 > The error callback you provide on the connect will also be called if you later get disconnected from the cluster or a fatal error preventing normal operation occurs.

Now that we have a connection to the cluster, let's store some binary data:

```javascript
var b = c.blob('bam');

b.put(new Buffer("boom"), function(err) { /* */  });
b.get(function(err, data) { /* */  });
```

Want a queue? We have distributed double-ended queues (aka deques).

```javascript
var q = c.deque('q2');

q.pushBack(new Buffer("boom"), function(err) { /* */ });
q.popFront(function(err, data) { /* */ });
q.pushFront(new Buffer("bang"), function(err) { /* */ });
q.size(function(err, s) { /* */ });
q.at(2, function(err, data) { /* */ });
```

quasardb comes out of the box with server-side atomic integers:

```javascript
var i = c.integer('some_int');
i.put(3, function(err){ /* */});
i.add(7, function(err, data){ /* */});
```

We also provide distributed sets:

```javascript
var s = c.set('the_set');

s.insert(new Buffer("boom"), function(err) { /* */ });
```

## Tags

[quasardb](https://www.quasardb.net/) is advanced key-value store with a powerful tagging feature. Tags lookup are fast, scalable and reliable.

Any entry can be tagged, including tags.

```javascript
var b = c.blob('bam');

b.addTag('dasTag', function(err) { /* */ });

var i = c.integer('bom');

i.addTag('dasTag', function(err) { /* */ });

var t = c.tag('dasTag');

t.getEntries(function(err, entries} { /* entries is the list of entries */ });
```

It is also possible to list the tags of an entry or test for the existence of a tag:

```javascript
b.hasTag('dasTag', function(err) { /* */ });

b.getTags(function(err, tags) { /* tags is the list of tags */ });
```

## Expiry

Integers and blob can be configured to automatically expire. The expiry can be specified with an absolute value at the entry creation or update, or can
later be set with expiresAt and expiresFromNow.

The expiry is second-precise.

```javascript
var b = c.blob('bam');

// value will be removed at the specified time
b.put(new Buffer("boom"), new Date("October 1st, 2016 11:13:00"), function(err) { /* */  });

// value expires now!
b.expiresAt(new Date());

// entry will be removed in 20 seconds relative to the current time
b.expiresFromNow(20);
```

Everytime you update a value without providing an expiry, the expiry time is set to "infinite" (i.e. never expires).

You can query the expiry time of an entry with getExpiry:

```javascript
// returns a Date object
var expiry = b.getExpiry();
```

What if you want to update but *do not want to change the expiry*?

There is a special value `qdb.PRESERVE_EXPIRATION` that can be used:

```javascript
var b = c.blob('bam');

// value will be removed at the specified time
b.put(new Buffer("boom"), new Date("October 1st, 2016 11:13:00"), function(err) { /* */  });

// do stuff

b.update(new Buffer("bang"), qdb.PRESERVE_EXPIRATION, function(err) { /* */ });
```

When using the special value `qdb.PRESERVE_EXPIRATION` updates will keep the previous expiration. If you want an entry to never expire you can either not specify an expiration or use `qdb.NEVER_EXPIRES`.

Tags, sets and queues do not expire (but can of course be manually removed).

## Timeout

You can configure the *client-side* timeout (the server-side timeout is a cluster configuration which cannot be remotely changed).

The client-side timeout is the time duration after which a client will consider a request to have timed out. The default value is one minute (60,000 ms). You may want to lower this value on low-latency networks.


```javascript
// sets the timeout to 10s
c.setTimeout(10000);
```

Ideally the timeout should be set before calling connect.

## Metadata

You may want to get some metainformation about an entry without actually acquiring the data itself. For this purpose, `getMetadata` method may be invoked on any entry.
It returns an object that describes the type of the entry, its creation, last modification and expiry times, as well as its unique identifier (reference).
For blob entries, the size of the content is also returned.

```javascript
var b = c.blob('bam');

b.getMetadata(function(err, meta)) {
    if (err) {
        // ...
    }

    // There is also meta.reference.

    if (meta.type == qdb.ENTRY_BLOB) {
        var content_size = meta.size;
    }

    var times = [meta.creation_time, meta.modification_time, meta.expiry_time];
}
```

## Errors

Quasardb callback return error objects. When the callback is successful, the error object is null. You can therefore safely write:

```javascript
var b = c.blob('bam');

b.put(new Buffer("boom"), function(err) {
    if (err) {
        // error management
        throw err.message;
    }

    // ...
});
```

You may not want to throw at every error. Some errors are *transient*. Meaning the underlying problem may (or may not) solve by itself. Transient errors are typically:

  * Transactions conflicts
  * Network timeouts
  * Cluster is currently stabilizing

Because you may want to try again before giving up, you can check if an error is transient with the transient() method:

```javascript
var b = c.blob('bam');

b.put(new Buffer("boom"), function(err) {
    if (err) {
        if (err.transient) {
            // let's try again
        }
    }

    // ...
});
```

You can also query if an error is *informational*. An informational error means that the query has been succesfully processed by the server and your parameters were valid but the result is either empty or unavailable. Typical informational errors are:

 * Non-existing entry
 * Empty collection
 * Index out of range
 * Integer overflow/underflow

```javascript
var b = c.blob('bam');

b.put(new Buffer("boom"), function(err) {
    if (err) {
        if (err.informational) {
            // let's do something different
        }
    }

    // ...
});
```

## Not supported yet

The quasardb nodejs addon is still a work in progress, the following quasardb features are not supported:

 * Advanced atomic operations such as compare and swap and conditional removal
 * Batches and transactions

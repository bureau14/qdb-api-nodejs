# quasardb node.js API

This is the official quasardb API, suitable for production. The full documentation is available [here](https://doc.quasardb.net/).

[quasardb](https://www.quasardb.net/) 2.0 or later required. You need a C++ 11 compiler to compile this addon.

## Installation

```
npm install quasardb --save
```

## Compilation

Alternatively if you want to compile from source:

1. Clone git repository
1. [Download quasardb server and C api](https://download.quasardb.net/quasardb/)
1. Extract both tarballs into `qdb-api-nodejs\qdb`

Then:

```
npm install --build-from-source
npm test
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

## Tags

[quasardb](https://www.quasardb.net/) is advanced key-value store with a powerful tagging feature. Tags lookup are fast, scalable and reliable.

Any entry can be tagged, including tags.

```javascript
var b = c.blob('bam');

b.attachTag('dasTag', function(err) { /* */ });

var i = c.integer('bom');

i.attachTag('dasTag', function(err) { /* */ });

var t = c.tag('dasTag');

t.getEntries(function(err, entries) { /* entries is the list of entries */ });
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
        throw err.message;
    }

    console.log(JSON.stringify(meta, null, ' '));
}
```

The result may be something like:

```json
{
 "reference": [
  7876229558754302000,
  -5412596186697675000,
  2684719845372276000,
  -4458640659107684400
 ],
 "type": 0,
 "size": 11,
 "modification_time": "2016-08-02T09:23:31.139Z"
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

## Time series

Creating time series:

```javascript
var ts = c.ts('temperature');

ts.create([qdb.DoubleColumnInfo('Temp')], function(err) {
    if (err) {
        // ...
    }

    // ...
});

// Or using `insert` function

ts.insert([qdb.DoubleColumnInfo('Temp'), qdb.BlobColumnInfo("Data")], function(err) {
    if (err) {
        // ...
    }

    // ...
});
```

Retrieving columns:


```javascript
var ts = c.ts('temperature');

// Populating ts ...


ts.columns(function(err, columns) {
    if (err) {
        // ...
    }

    // ...
});
```

Creating and populating columns:


```javascript
var ts = c.ts('temperature');

ts.create([qdb.DoubleColumnInfo("ColDouble"), qdb.BlobColumnInfo("ColBlob")], function(err, columns){
	if (err) {
		// ...
	}

	// columns is a array of `active column` instances
	var p1 = qdb.DoublePoint(new Date(2049, 10, 5), 100.0);
	var p2 = qdb.DoublePoint(new Date(2049, 10, 5, 4, 3), 110.0);

	columns[0].insert([p1, p2], function(err) {
		// ...
	});

	var p3 = qdb.BlobPoint(new Date(2049, 10, 5), new Buffer("Water must be hot now"));
	columns[1].insert([p3], function(err) {
		// ...
	});
});

```

Getting values from time series columns:


```javascript
var ts = c.ts('temperature');

ts.create([qdb.DoubleColumnInfo("ColDouble"), qdb.BlobColumnInfo("ColBlob")], function(err, columns){
	if (err) {
		// ...
	}

	// ... populating with values

	var range = qdb.TsRange(new Date(2021, 10, 10), new Date(2021, 11, 11));
	column[0].ranges([range], function(err, points){
		// ...
	});
});

```

Erasing values from time series columns:


```javascript
var ts = c.ts('temperature');

ts.create([qdb.DoubleColumnInfo("ColDouble"), qdb.BlobColumnInfo("ColBlob")], function(err, columns) {
	if (err) {
		// ...
	}

	// ... populating with values

	var ranges = [
	    qdb.TsRange(new Date(2021, 10, 10, 8, 0), new Date(2021, 10, 10, 12, 0)),
	    qdb.TsRange(new Date(2021, 11, 11, 0, 0), new Date(2021, 11, 11, 10, 0))
	];

	column[0].erase(ranges, function(err, erased) {
		// ...
	});
});

```


Aggregations on time series columns


```javascript
var ts = c.ts('temperature');

ts.create([qdb.DoubleColumnInfo("ColDouble"), qdb.BlobColumnInfo("ColBlob")], function(err, columns) {
	if (err) {
		// ...
	}

	// ... populating with values

	var range = qdb.TsRange(new Date(2021, 10, 10, 8, 0), new Date(2021, 10, 10, 12, 0));
	var aggFirst = qdb.Aggregation(dqb.AggFirst, range);
	var aggMax = qdb.Aggregation(dqb.AggMax, range);
	var aggSum = qdb.Aggregation(dqb.AggSum, range);

    // Aggregate over double column
    column[0].aggregate([aggFirst, aggMax, aggSum], function(err, aggrs) {
        if (err) {
            // ...
        }

        var firstPoint = aggrs[0].result;
        var maxPoint = aggrs[1].result;
        var sumValue = aggrs[2].result.value;
        var sumCount = aggrs[2].count;

        // Using aggr values
	});

    // Aggregate over blob column
    column[1].aggregate([aggFirst], function(err, aggrs) {
        if (err) {
            // ...
        }

        var firstBlob = aggrs[0].result;
	});

});

```

`aggregate` function return error (if any) and array of result for each aggregation which was passed as input.
Each result element consists from two properties:
 * `result` contains Blob or Double point;
 * `count` contains (if applicable) the number of datapoints on which aggregation has been computed.

## Not supported yet

The quasardb nodejs addon is still a work in progress, the following quasardb features are not supported:

 * Advanced atomic operations such as compare and swap and conditional removal
 * Batches and transactions

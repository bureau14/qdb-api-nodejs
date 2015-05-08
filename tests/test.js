// test.js
var qdb = require('../bin/qdb');

var c = new qdb.Cluster('qdb://127.0.0.1:2836');

// var b = c.blob('bam');

// console.log(b.alias());

// var mycontent = "boom";
// var buf = new Buffer(mycontent);

// b.put(function(err, data) { console.log(err); }, buf);
// b.get(function(err, data) { console.log(err); console.log(data.toString()); });

// var mycontent2 = "boom2";
// var buf2 = new Buffer(mycontent2);

// b.update(function(err, data) { console.log(err); console.log(data.toString()); }, buf2);
// b.get(function(err, data) { console.log(err); console.log(data.toString()); });

// b.remove(function(err, data) { console.log(err); });

// var i = c.integer('int1');

// i.put(function(err, value) { console.log('error = ' + err.toString()); }, 3);
// i.get(function(err, value) { console.log('error = ' + err.toString() + " value = " + value.toString());})
// i.add(function(err, value) { console.log('error = ' + err.toString() + " value = " + value.toString());}, 6)
// i.remove(function(err, value) { console.log('error = ' + err.toString()); });

/*var q = c.queue('q2');

var mycontent1 = "boom";
var buf1 = new Buffer(mycontent1);

var mycontent2 = "boom2";
var buf2 = new Buffer(mycontent2);

console.log(q.alias());

q.pushBack(function(err, data) { console.log("error = " + err.toString()); }, buf1);
q.front(function(err, data) 
{ 
    console.log("error = " + err.toString()); 
    console.log("content = " + data.toString()); 
});

q.pushFront(function(err, data) { console.log("error = " + err.toString()); }, buf2);
q.back(function(err, data) {console.log("error = " + err.toString()); console.log("content = " + data.toString()); } );*/


var s = c.set('s1');

//var mycontent1 = "boom";
var buf1 = new Buffer("bam");

s.contains(function(err) { console.log("first contains error = " + err.toString()); }, buf1);
s.insert(function(err) { console.log("insert error = " + err.toString()); }, buf1);
s.contains(function(err) { console.log("second contains error = " + err.toString()); }, buf1);
s.erase(function(err) { console.log("erase error = " + err.toString()); }, buf1);
s.contains(function(err) { console.log("third contains error = " + err.toString()); }, buf1);
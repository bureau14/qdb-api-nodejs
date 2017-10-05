var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.cluster_uri);


describe('TimeSeries', function() {
	var ts
    before('connect', function(done) {
        cluster.connect(done, done);
    });

    before('init', function() {
        ts = cluster.ts("time_series");
    });

    it('should be of correct type', function() {
        test.object(ts).isInstanceOf(qdb.TimeSeries);
    });

    it("should have an 'alias' property", function() {
        test.object(ts).hasProperty('alias');
        test.must(ts.alias()).be.a.string();
    });

    describe('column info', function() {
		const doubleColName  = 'doubleCol'
		const blobColName  = 'blobCol'
        var d, b

        before('init', function() {
			d = qdb.DoubleColumnInfo(doubleColName);
			b = qdb.BlobColumnInfo(blobColName);
        });

        it('should have name property', function() {
			test.object(d).hasProperty('name')
			test.must(d.name).be.a.string();
			test.must(d.name).be.equal(doubleColName);

			test.object(b).hasProperty('name')
			test.must(b.name).be.a.string();
			test.must(b.name).be.equal(blobColName);
        });

        it('should have type property', function() {
			test.object(d).hasProperty('type')
			test.must(d.type).be.a.number();
			test.must(d.type).be.equal(qdb.TS_COLUMN_DOUBLE);

			test.object(b).hasProperty('type')
			test.must(b.type).be.a.number();
			test.must(b.type).be.equal(qdb.TS_COLUMN_BLOB);
        });

    }); // column info test

	describe('creation', function() {
		var columnInfo
		var emptyTs
		before('init', function() {
			columnInfo = qdb.DoubleColumnInfo('Col2')
		});

		after('creation', function(done) {
			emptyTs.remove(function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should create with empty columns', function(done) {
			emptyTs = cluster.ts("empty-ts")
			emptyTs.create([], function(err) {
				test.must(err).be.equal(null);
				done();
			});
		});

		it('should create column', function(done) {
			ts.create([columnInfo], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should not create existing column', function(done) {
			ts.create([columnInfo], function(err) {
				test.must(err).not.be.equal(null);
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_ALIAS_ALREADY_EXISTS);
                test.must(err.severity).be.a.equal(qdb.E_SEVERITY_WARNING);
                test.must(err.origin).be.a.equal(qdb.E_ORIGIN_OPERATION);
                test.must(err.informational).be.false();

				done();
			});
		});

		it('should insert new columns', function(done) {
			ts.insert([qdb.DoubleColumnInfo("Colx"), qdb.BlobColumnInfo("Coly")], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should not insert existing columns', function(done) {
			ts.insert([qdb.DoubleColumnInfo("Colx"), qdb.BlobColumnInfo("ColZ")], function(err) {
				test.must(err).not.be.equal(null);
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.severity).be.a.equal(qdb.E_SEVERITY_ERROR);
                test.must(err.origin).be.a.equal(qdb.E_ORIGIN_INPUT);
                test.must(err.informational).be.false();

				done();
			});
		});

		it('should not insert empty columns', function(done) {
			ts.insert([], function(err) {
				test.must(err).not.be.equal(null);
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_INVALID_ARGUMENT);
                test.must(err.severity).be.a.equal(qdb.E_SEVERITY_ERROR);
                test.must(err.origin).be.a.equal(qdb.E_ORIGIN_INPUT);
                test.must(err.informational).be.false();

				done();
			});
		});

		it('should remove time series', function(done) {
			ts.remove(function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});


	}); // create test

	describe('columns', function() {
		var columnInfo
		var ts

		before('init', function(done) {
			columnInfo = [qdb.DoubleColumnInfo('Col1'), qdb.DoubleColumnInfo("Col2"), qdb.BlobColumnInfo("Col3")]
			ts = cluster.ts("list")

			ts.create([], function(err) {
				test.must(err).be.equal(null);
				done();
			});
		});

		it('should return empty', function(done) {
			ts.columns(function(err, columns) {
				test.must(err).be.equal(null);
				test.must(columns.length).be.equal(0);
				done();
			});
		});

		it('should insert columns', function(done) {
			ts.insert(columnInfo, function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should show columns', function(done) {
			ts.columns(function(err, columns) {
				test.must(err).be.equal(null);
				test.must(columns.length).be.equal(columnInfo.length);

				// FIXME: So far qdb returns columns in the same order as we
				// created them, but would be better if test compares using `indexof` or somthing like that
				for (var i = 0; i < columns.length; i++) {
					test.must(columns[i].name).be.equal(columnInfo[i].name);
					test.must(columns[i].type).be.equal(columnInfo[i].type);
				}

				done();
			});
		});

		it('should remove ts', function(done) {
			ts.remove(function(err){
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should return error', function(done) {
			ts.columns(function(err, columns) {
				test.must(err).not.be.equal(null);
                test.must(err.message).be.a.string();
                test.must(err.message).not.be.empty();
                test.must(err.code).be.equal(qdb.E_ALIAS_NOT_FOUND);
                test.must(err.severity).be.a.equal(qdb.E_SEVERITY_WARNING);
                test.must(err.origin).be.a.equal(qdb.E_ORIGIN_OPERATION);
                test.must(err.informational).be.false();

				done();
			});
		});

	}); // list

	describe('points insert', function() {
		var ts
		var columns

		before('init', function(done) {
			var columnInfo = [qdb.DoubleColumnInfo('Col1'), qdb.BlobColumnInfo("Col2")]
			ts = cluster.ts("points")

			ts.create([columnInfo], function(err, cols) {
				test.must(err).be.equal(null);

				columns = cols
				done();
			});
		});

		it('should create points', function() {
			var d1 = new Date(2049, 10, 5);
			var d2 = new Date(2049, 10, 5, 4);

			var v1 = 0.1
			var v2 = new Buffer("Hello there");

			var p1 = qdb.DoublePoint(d1, v1);
			var p2 = qdb.BlobPoint(d2, v2);

			test.object(p1).hasProperty('timestamp');
			test.object(p2).hasProperty('timestamp');

			test.must(p1.timestamp().getTime()).be.equal(d1.getTime());
			test.must(p2.timestamp().getTime()).be.equal(d2.getTime());

			test.object(p1).hasProperty('value')
			test.object(p2).hasProperty('value')

			test.must(p1.value()).be.equal(v1);
			test.must(p2.value().compare(v2)).be.equal(0);

		});

		it('should hold blob Buffer', function(done){
			var d = new Date(2049, 10, 5, 4);
			var v = new Buffer("Hello there");
			var p = qdb.BlobPoint(d, v);

			if (!global.gc) {
				console.log('Garbage collection unavailable.  Pass --expose-gc '
					+ 'when launching node to enable forced garbage collection.');
				done();
				return;
			}

			// Force GC
			v = null
			global.gc();
			this.timeout(3000);

			test.wait(2000, function() {
				var vc = new Buffer("Hello there")
				test.must(p.value().compare(vc)).be.equal(0)

				done();
			});

		});

		it('should insert double point', function(done) {
			var p = qdb.DoublePoint(new Date(2049, 10, 5), 0.4);
			columns[0].insert([p], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should insert multiple double points', function(done) {
			var points = [qdb.DoublePoint(new Date(2049, 10, 5, 1), 0.4),
				qdb.DoublePoint(new Date(2049, 10, 5, 2), 0.5),
				qdb.DoublePoint(new Date(2049, 10, 5, 3), 0.6)];

			columns[0].insert(points, function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should not insert double point into blob column', function(done) {
			var p = qdb.BlobPoint(new Date(2049, 10, 5, 4), new Buffer("Hello"));
			columns[1].insert([p], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should insert blob point', function(done) {
			var p = qdb.BlobPoint(new Date(2049, 10, 6, 4), new Buffer("Well "));
			columns[1].insert([p], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should insert multiple blob points', function(done) {
			var points = [qdb.BlobPoint(new Date(2049, 10, 6, 4), new Buffer("hello ")),
				qdb.BlobPoint(new Date(2049, 10, 6, 4), new Buffer("there ")),
				qdb.BlobPoint(new Date(2049, 10, 6, 4), new Buffer("Decard"))];

			columns[1].insert(points, function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

		it('should not insert blob point into double column', function(done) {
			var p = qdb.DoublePoint(new Date(2049, 10, 7, 4), 13.37);
			columns[1].insert([p], function(err) {
				test.must(err).be.equal(null);

				done();
			});
		});

	}); // points insert

}); // time series

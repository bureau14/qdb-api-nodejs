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

		it('should not insert epmty columns', function(done) {
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

}); // time series

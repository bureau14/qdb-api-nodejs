var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

describe('TimeSeries', function () {
    var ts
    before('connect', function (done) {
        insecureCluster.connect(done, done);
    });

    before('init', function () {
        ts = insecureCluster.ts("time_series");
    });

    it('should be of correct type', function () {
        test.object(ts).isInstanceOf(qdb.TimeSeries);
    });

    it("should have an 'alias' property", function () {
        test.object(ts).hasProperty('alias');
        test.must(ts.alias()).be.a.string();
    });

    describe('column info', function () {
        const doubleColName = 'doubleCol'
        const blobColName = 'blobCol'
        var d, b

        before('init', function () {
            d = qdb.DoubleColumnInfo(doubleColName);
            b = qdb.BlobColumnInfo(blobColName);
        });

        it('should have name property', function () {
            test.object(d).hasProperty('name')
            test.must(d.name).be.a.string();
            test.must(d.name).be.equal(doubleColName);

            test.object(b).hasProperty('name')
            test.must(b.name).be.a.string();
            test.must(b.name).be.equal(blobColName);
        });

        it('should have type property', function () {
            test.object(d).hasProperty('type')
            test.must(d.type).be.a.number();
            test.must(d.type).be.equal(qdb.TS_COLUMN_DOUBLE);

            test.object(b).hasProperty('type')
            test.must(b.type).be.a.number();
            test.must(b.type).be.equal(qdb.TS_COLUMN_BLOB);
        });

    }); // column info test

    describe('creation', function () {
        var columnInfo
        var emptyTs
        before('init', function () {
            columnInfo = qdb.DoubleColumnInfo('Col2')
        });

        after('creation', function (done) {
            emptyTs.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should create with empty columns', function (done) {
            emptyTs = insecureCluster.ts("empty-ts")
            emptyTs.create([], function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns).be.empty()

                done();
            });
        });

        it('should create column', function (done) {
            ts.create([columnInfo], function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns).not.be.empty()
                test.must(columns.length).be.equal(1)
                test.must(columns[0].alias()).be.equal("Col2");
                test.must(columns[0].type).be.equal(qdb.TS_COLUMN_DOUBLE);
                test.must(columns[0].timeseries).be.equal(ts.alias());

                done();
            });
        });

        it('should not create existing column', function (done) {
            ts.create([columnInfo], function (err, columns) {
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

        it('should insert new columns', function (done) {
            ts.insert([qdb.DoubleColumnInfo("Colx"), qdb.BlobColumnInfo("Coly")], function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns).not.be.empty()
                test.must(columns.length).be.equal(2)

                test.must(columns[0].alias()).be.equal("Colx");
                test.must(columns[0].type).be.equal(qdb.TS_COLUMN_DOUBLE);
                test.must(columns[0].timeseries).be.equal(ts.alias());

                test.must(columns[1].alias()).be.equal("Coly");
                test.must(columns[1].type).be.equal(qdb.TS_COLUMN_BLOB);
                test.must(columns[1].timeseries).be.equal(ts.alias());

                done();
            });
        });

        it('should not insert existing columns', function (done) {
            ts.insert([qdb.DoubleColumnInfo("Colx"), qdb.BlobColumnInfo("ColZ")], function (err, columns) {
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

        it('should not insert empty columns', function (done) {
            ts.insert([], function (err, columns) {
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

        it('should remove time series', function (done) {
            ts.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });
    });

    describe('columns', function () {
        var columnInfo
        var ts

        before('init', function (done) {
            columnInfo = [qdb.DoubleColumnInfo('Col1'), qdb.DoubleColumnInfo("Col2"), qdb.BlobColumnInfo("Col3")]
            ts = insecureCluster.ts("list")

            ts.create([], function (err, columns) {
                test.must(err).be.equal(null);
                done();
            });
        });

        it('should return empty', function (done) {
            ts.columns(function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns).be.empty();
                done();
            });
        });

        it('should insert columns', function (done) {
            ts.insert(columnInfo, function (err, columns) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should show columns', function (done) {
            ts.columns(function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns.length).be.equal(columnInfo.length);

                // Database guarantees to return columns in the same order as we created/appended them.
                for (var i = 0; i < columns.length; i++) {
                    test.must(columns[i].alias()).be.equal(columnInfo[i].name);
                    test.must(columns[i].type).be.equal(columnInfo[i].type);
                    test.must(columns[i].timeseries).be.equal(ts.alias());
                }

                done();
            });
        });

        it('should remove ts', function (done) {
            ts.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should return error', function (done) {
            ts.columns(function (err, columns) {
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

    });

    describe('points insert', function () {
        var ts = null
        var columns = null

        before('init', function (done) {
            var columnInfo = [qdb.DoubleColumnInfo('Col1'), qdb.BlobColumnInfo("Col2")]
            ts = insecureCluster.ts("points")

            ts.remove(function (err) {
                ts.create(columnInfo, function (err, cols) {
                    test.must(err).be.equal(null);
                    test.should(cols.length).eql(columnInfo.length);

                    columns = cols;
                    done();
                });
            })
        });

        it('should create points', function () {
            var d1 = new Date(2049, 10, 5);
            var d2 = new Date(2049, 10, 5, 4);

            var v1 = 0.1
            var v2 = Buffer.from("Hello there", 'utf8');

            var p1 = qdb.DoublePoint(qdb.Timestamp.fromDate(d1), v1);
            var p2 = qdb.BlobPoint(qdb.Timestamp.fromDate(d2), v2);

            test.object(p1).hasProperty('timestamp');
            test.object(p2).hasProperty('timestamp');

            test.must(p1.timestamp.toDate().getTime()).be.equal(d1.getTime());
            test.must(p2.timestamp.toDate().getTime()).be.equal(d2.getTime());

            test.object(p1).hasProperty('value')
            test.object(p2).hasProperty('value')

            test.must(p1.value).be.equal(v1);
            test.must(p2.value.compare(v2)).be.equal(0);

        });

        it('should hold blob Buffer', function () {
            var d = new Date(2049, 10, 5, 4);
            var v = new Buffer.from("Hello there", "utf8");
            var p = qdb.BlobPoint(qdb.Timestamp.fromDate(d), v);

            test.must(p.value.compare(v)).be.equal(0)
        });

        it('should insert double point', function (done) {
            var p = qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), 0.4);
            columns[0].insert([p], function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should insert multiple double points', function (done) {
            var points = [
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), 0.4),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), 0.5),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), 0.6)
            ];

            columns[0].insert(points, function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });


        it('should not insert double point into blob column', function (done) {
            var p = qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), Buffer.from("Hello", 'utf8'));
            columns[1].insert([p], function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should insert blob point', function (done) {
            var p = qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 6, 4)), Buffer.from("Well ", "utf8"));
            columns[1].insert([p], function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should insert multiple blob points', function (done) {
            var points = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 6, 4)), Buffer.from("hello ", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 6, 4)), Buffer.from("there ", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 6, 4)), Buffer.from("Decard", "utf8"))
            ];

            columns[1].insert(points, function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should not insert blob point into double column', function (done) {
            var p = qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 7, 4)), 13.37);
            columns[1].insert([p], function (err) {
                test.must(err).not.be.equal(null);

                done();
            });
        });
    });

    describe('ranges', function () {
        var ts = null
        var columns = null
        var doublePoints = null
        var blobPoints = null

        before('init', function (done) {
            var columnInfo = [qdb.DoubleColumnInfo('doubles'), qdb.BlobColumnInfo("blobs")]
            ts = insecureCluster.ts("ranges")

            ts.remove(function (err) {
                ts.create(columnInfo, function (err, cols) {
                    test.must(err).be.equal(null);
                    test.should(cols.length).eql(columnInfo.length);

                    columns = cols;
                    done();
                });
            });
        });

        it('should insert double points', function (done) {
            doublePoints = [
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 6)), 0.1),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 7)), 0.2),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 8)), 0.3),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), 0.4),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), 0.5),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), 0.6),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), 0.7),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), 0.8),
            ];

            columns[0].insert(doublePoints, function (err) {
                test.must(err).be.equal(null);
                done();
            });
        });

        it('should insert blob points', function (done) {
            blobPoints = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 6)), Buffer.from("#6", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 7)), Buffer.from("#7", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 8)), Buffer.from("#8", "utf8")),

                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from("#1", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from("#2", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from("#3", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), Buffer.from("#4", "utf8")),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), Buffer.from("#5", "utf8")),
            ];

            columns[1].insert(blobPoints, function (err) {
                test.must(err).be.equal(null);
                done();
            });
        });

        it('should create valid range', function () {
            var begin = new Date(2049, 10, 5, 1);
            var end = new Date(2049, 10, 5, 3);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            test.object(range).hasProperty('begin');
            test.object(range).hasProperty('end');

            test.must(range.begin.toDate().getTime()).be.equal(begin.getTime());
            test.must(range.end.toDate().getTime()).be.equal(end.getTime());
        });

        it('should retrieve nothing', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2020, 10, 5, 4);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[0].ranges([range], function (err, points) {
                test.must(err).be.equal(null);
                test.must(points.length).be.equal(0);

                done();
            });
        });

        it('should retrieve double points in range', function (done) {
            var begin = new Date(2049, 10, 5, 2);
            var end = new Date(2049, 10, 5, 4);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[0].ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = doublePoints.slice(4, 6);
                test.array(points).is(exp);
                done();
            });
        });

        it('should retrieve double points in all ranges', function (done) {
            var b1 = new Date(2049, 10, 5, 2);
            var e1 = new Date(2049, 10, 5, 4);
            var b2 = new Date(2030, 10, 5, 7);
            var e2 = new Date(2030, 10, 5, 8);
            var ranges = [
                qdb.TsRange(qdb.Timestamp.fromDate(b1), qdb.Timestamp.fromDate(e1)),
                qdb.TsRange(qdb.Timestamp.fromDate(b2), qdb.Timestamp.fromDate(e2))
            ];

            columns[0].ranges(ranges, function (err, points) {
                test.must(err).be.equal(null);

                exp = doublePoints.slice(1, 2).concat(doublePoints.slice(4, 6));
                test.array(points).is(exp);
                done();
            });
        });

        it('should retrieve all double points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[0].ranges([range], function (err, points) {
                test.must(err).be.equal(null);
                test.array(points).is(doublePoints);

                done();
            });
        });

        var blobsCheck =
            function (exp, act) {
                test.must(exp.length).be.equal(act.length);

                for (var i = 0; i < act.length; i++) {
                    e = exp[i];
                    a = act[i];
                    test.must(e.timestamp.toDate().getTime()).be.equal(a.timestamp.toDate().getTime());
                    test.must(e.value.compare(a.value)).be.equal(0);
                }
            }

        it('should retrieve blob points in range', function (done) {
            var begin = new Date(2049, 10, 5, 2);
            var end = new Date(2049, 10, 5, 4);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[1].ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = blobPoints.slice(4, 6);
                blobsCheck(exp, points);
                done();
            });
        });

        it('should retrieve blob points in all ranges', function (done) {
            var b1 = new Date(2049, 10, 5, 2);
            var e1 = new Date(2049, 10, 5, 4);
            var b2 = new Date(2030, 10, 5, 7);
            var e2 = new Date(2030, 10, 5, 8);
            var ranges = [
                qdb.TsRange(qdb.Timestamp.fromDate(b1), qdb.Timestamp.fromDate(e1)),
                qdb.TsRange(qdb.Timestamp.fromDate(b2), qdb.Timestamp.fromDate(e2))
            ];

            columns[1].ranges(ranges, function (err, points) {
                test.must(err).be.equal(null);

                exp = blobPoints.slice(1, 2).concat(blobPoints.slice(4, 6));
                blobsCheck(exp, points);
                done();
            });
        });


        it('should retrieve all blob points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[1].ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                blobsCheck(blobPoints, points);
                done();
            });
        });

        it('should erase ranges of double points', function (done) {
            var b1 = new Date(2030, 10, 5, 6);
            var e1 = new Date(2030, 10, 5, 10);
            var b2 = new Date(2049, 10, 5, 5);
            var e2 = new Date(2049, 10, 5, 6);
            var ranges = [
                qdb.TsRange(qdb.Timestamp.fromDate(b1), qdb.Timestamp.fromDate(e1)),
                qdb.TsRange(qdb.Timestamp.fromDate(b2), qdb.Timestamp.fromDate(e2))
            ];

            columns[0].erase(ranges, function (err, erased) {
                test.must(err).be.equal(null);
                test.must(erased).be.equal(4);

                done();
            });
        });

        it('should erase ranges of blob points', function (done) {
            var b1 = new Date(2030, 10, 5, 6);
            var e1 = new Date(2030, 10, 5, 10);
            var b2 = new Date(2049, 10, 5, 5);
            var e2 = new Date(2049, 10, 5, 6);
            var ranges = [
                qdb.TsRange(qdb.Timestamp.fromDate(b1), qdb.Timestamp.fromDate(e1)),
                qdb.TsRange(qdb.Timestamp.fromDate(b2), qdb.Timestamp.fromDate(e2))
            ];

            columns[1].erase(ranges, function (err, erased) {
                test.must(err).be.equal(null);
                test.must(erased).be.equal(4);

                done();
            });
        });

        it('should retrieve all remaining double points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[0].ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = doublePoints.slice(3, 7);
                test.array(points).is(exp);

                done();
            });
        });

        it('should retrieve all remaining blob points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            columns[1].ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = blobPoints.slice(3, 7);
                blobsCheck(exp, points);

                done();
            });
        });
    });

    describe('aggregations', function () {
        var ts = null
        var columns = null
        var doublePoints = null
        var blobPoints = null
        var range = null

        before('init', function (done) {
            ts = insecureCluster.ts("aggregations")
            range = qdb.TsRange(
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)),
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 12))
            );
            ts.remove(function (err) {
                ts.create([qdb.DoubleColumnInfo('doubles'), qdb.BlobColumnInfo("blobs")], function (err, cols) {
                    test.should(err).be.equal(null);
                    test.should(cols.length).eql(2);

                    columns = cols;
                    done();
                });
            })
        });

        it('should insert double points', function (done) {
            doublePoints = [
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), 0.1),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), 0.2),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), 0.3),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), 0.4),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), 0.5),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 6)), 0.1),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 7)), 0.2),
                qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 8)), 0.3),
            ];

            columns[0].insert(doublePoints, function (err) {
                test.should(err).be.equal(null);
                done();
            });
        });

        it('should insert blob points', function (done) {
            blobPoints = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from("#1", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from("#2", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from("#3", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), Buffer.from("#4", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), Buffer.from("#5", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 6)), Buffer.from("#6", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 7)), Buffer.from("#7", 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 8)), Buffer.from("#8", 'utf8')),
            ];

            columns[1].insert(blobPoints, function (err) {
                test.should(err).be.equal(null);
                done();
            });
        });

        it('should create valid aggregation', function () {
            var range = qdb.TsRange(
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)),
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 12))
            );
            var agg = qdb.Aggregation(qdb.AggFirst, range)

            test.object(agg).hasProperty('type');
            test.object(agg).hasProperty('range');

            test.should(agg.type).eql(qdb.AggFirst);
            test.should(agg.range).eql(range);
        });

        var checkAggrs = function (col, aggrs, exp, done) {
            col.aggregate(aggrs, function (err, results) {
                test.should(err).be.equal(null);
                test.should(results).not.be.equal(null);
                test.should(results.length).eql(exp.length);

                for (var i = 0; i < exp.length; i++) {
                    test.object(results[i]).hasProperty('result');
                    test.object(results[i]).hasProperty('count');

                    test.should(results[i].result).eql(exp[i]);
                    test.should(results[i].count).eql(1);
                }

                done();
            });
        };

        it('should find first and last double points', function (done) {
            var aggrs = [qdb.Aggregation(qdb.AggFirst, range), qdb.Aggregation(qdb.AggLast, range)];
            var exp = [doublePoints[0], doublePoints.slice(-1)[0]];

            checkAggrs(columns[0], aggrs, exp, done);
        });

        it('should find first and last blob points', function (done) {
            var aggrs = [qdb.Aggregation(qdb.AggFirst, range), qdb.Aggregation(qdb.AggLast, range)];
            var exp = [blobPoints[0], blobPoints.slice(-1)[0]];

            checkAggrs(columns[1], aggrs, exp, done);
        });

        it('should return sum of all double points', function (done) {
            var aggrs = [qdb.Aggregation(qdb.AggSum, range)];

            columns[0].aggregate(aggrs, function (err, results) {
                test.should(err).be.equal(null);
                test.should(results).not.be.equal(null);
                test.should(results.length).eql(1);

                test.should(results[0].count).eql(8);
                test.should(results[0].result.value).eql(2.1);

                done();
            });
        });
    });
});

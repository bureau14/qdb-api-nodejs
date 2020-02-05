var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

describe('Timeseries - Blob', function () {
    
    before('connect', function (done) {
        insecureCluster.connect(done, done);
    });

    describe('column info', function () {
        var col_info
        before('init', function () {
            col_info = qdb.BlobColumnInfo('col');
        });
        
        it('should have name property', function () {
            test.object(col_info).hasProperty('name')
            test.must(col_info.name).be.a.string();
            test.must(col_info.name).be.equal('col');
        });
        
        it('should have type property', function () {
            test.object(col_info).hasProperty('type')
            test.must(col_info.type).be.a.number();
            test.must(col_info.type).be.equal(qdb.TS_COLUMN_BLOB);
        });
    }); // column info

    
    describe('ts', function () {
        var columnInfo = null
        var ts = null
        before('init', function () {
            columnInfo = qdb.BlobColumnInfo('col')
            ts = insecureCluster.ts('ts')
        });
        
        it('should create blob column', function (done) {
            ts.remove(function (err) {
                ts.create([columnInfo], function (err, cols) {
                    test.must(err).be.equal(null);
                    test.must(cols).not.be.empty()
                    test.must(cols.length).be.equal(1)
                    test.must(cols[0].alias()).be.equal('col');
                    test.must(cols[0].type).be.equal(qdb.TS_COLUMN_BLOB);
                    test.must(cols[0].timeseries).be.equal(ts.alias());
                    done();
                });
            });
        });
        
        it('should not create existing blob column', function (done) {
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

        it('should remove timeseries with blob column', function (done) {
            ts.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });
    }); // ts
    
    describe('column', function () {
        var columnInfo
        var ts

        before('init', function (done) {
            columnInfo = [qdb.BlobColumnInfo('col')]
            ts = insecureCluster.ts('ts')

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
        
        it('should insert blob column', function (done) {
            ts.insert(columnInfo, function (err, columns) {
                test.must(err).be.equal(null);

                done();
            });
        });
        
        it('should show blob column', function (done) {
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

        it('should remove ts with blob column', function (done) {
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
    }); // column

    describe('insert', function () {
        var ts = null
        var column = null

        before('init', function (done) {
            var columnInfo = qdb.BlobColumnInfo('col')
            ts = insecureCluster.ts('ts')

            ts.remove(function (err) {
                ts.create([columnInfo], function (err, cols) {
                    test.must(err).be.equal(null);
                    test.should(cols.length).eql(1);

                    test.should(cols[0].type).eql(qdb.TS_COLUMN_BLOB);
                    column = cols[0];

                    done();
                });
            })
        });
        
        it('should create point', function () {
            var date = new Date(2049, 10, 5);
            var timestamp = qdb.Timestamp.fromDate(date);
            var value = Buffer.from('content', 'utf8');
            var point = qdb.BlobPoint(timestamp, value);
            
            test.object(point).hasProperty('timestamp');
            test.must(point.timestamp.toDate().getTime()).be.equal(date.getTime());
            
            test.object(point).hasProperty('value')
            
            test.must(point.value).be.equal(value);
        });
        

        it('should hold blob value', function () {
            var date = new Date(2049, 10, 5);
            var timestamp = qdb.Timestamp.fromDate(date);
            var value = Buffer.from('content', 'utf8');
            var point = qdb.BlobPoint(timestamp, value);
            test.must(point.value.compare(value)).be.equal(0)
        });
        
        it('should insert multiple blob points', function (done) {
            var points = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from('a', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from('b', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from('c', 'utf8'))
            ];

            column.insert(points, function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });
    }); // insert
    

    describe('ranges', function () {
        var ts = null
        var column = null
        var insertedPoints = null

        before('init', function (done) {
            var columnInfo = qdb.BlobColumnInfo('col')
            ts = insecureCluster.ts('ts')

            ts.remove(function (err) {
                ts.create([columnInfo], function (err, cols) {
                    test.must(err).be.equal(null);
                    test.should(cols.length).eql(1);

                    column = cols[0];
                    done();
                });
            });
        });
        
        it('should insert blob points', function (done) {
            insertedPoints = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 6)), Buffer.from('a', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 7)), Buffer.from('b', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 8)), Buffer.from('c', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from('d', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from('e', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from('f', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), Buffer.from('g', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), Buffer.from('h', 'utf8')),
            ];

            column.insert(insertedPoints, function (err) {
                test.must(err).be.equal(null);
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

            column.ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = insertedPoints.slice(4, 6);
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

            column.ranges(ranges, function (err, points) {
                test.must(err).be.equal(null);

                exp = insertedPoints.slice(1, 2).concat(insertedPoints.slice(4, 6));
                blobsCheck(exp, points);
                done();
            });
        });

        it('should retrieve all blob points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            column.ranges([range], function (err, points) {
                test.must(err).be.equal(null);
                blobsCheck(insertedPoints, points);

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

            column.erase(ranges, function (err, erased) {
                test.must(err).be.equal(null);
                test.must(erased).be.equal(4);

                done();
            });
        });

        it('should retrieve all remaining blob points', function (done) {
            var begin = new Date(2000, 10, 5, 2);
            var end = new Date(2049, 10, 5, 10);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            column.ranges([range], function (err, points) {
                test.must(err).be.equal(null);

                exp = insertedPoints.slice(3, 7);
                blobsCheck(exp, points);

                done();
            });
        });

    }); // ranges

    describe('aggregations', function () {
        var ts = null
        var column = null
        var insertedPoints = null
        
        before('init', function (done) {
            ts = insecureCluster.ts('ts')
            range = qdb.TsRange(
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)),
                qdb.Timestamp.fromDate(new Date(2049, 10, 5, 12))
            );
            ts.remove(function (err) {
                ts.create([qdb.BlobColumnInfo('col')], function (err, cols) {
                    test.should(err).be.equal(null);
                    test.should(cols.length).eql(1);

                    column = cols[0];
                    done();
                });
            })
        });
        
        it('should insert blob points', function (done) {
            insertedPoints = [
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 6)), Buffer.from('a', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 7)), Buffer.from('b', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2030, 10, 5, 8)), Buffer.from('c', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from('d', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from('e', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from('f', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 4)), Buffer.from('g', 'utf8')),
                qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 5)), Buffer.from('h', 'utf8')),
            ];

            column.insert(insertedPoints, function (err) {
                test.must(err).be.equal(null);
                done();
            });
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

        it('should find first and last blob points', function (done) {
            var aggrs = [qdb.Aggregation(qdb.AggFirst, range), qdb.Aggregation(qdb.AggLast, range)];
            var exp = [insertedPoints[0], insertedPoints.slice(-1)[0]];

            checkAggrs(column, aggrs, exp, done);
        });

    }); //aggregations
});
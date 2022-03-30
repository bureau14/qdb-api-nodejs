var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var insecureCluster = new qdb.Cluster(config.insecure_cluster_uri);

describe('Timeseries - General', function () {
    var ts = null

    var blobName = 'blob'
    var blobMakeInfo = (n) => qdb.BlobColumnInfo(n)
    var blobType = {
        name: blobName,
        colName: `${blobName}_col`,
        index: 0,
        makeInfo: (n) => blobMakeInfo(n),
        info: blobMakeInfo(blobName),
        makePoint: () => qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), Buffer.from('content', 'utf8')),
    }
    var doubleName = 'double'
    var doubleMakeInfo = (n) => qdb.DoubleColumnInfo(n)
    var doubleType = {
        name: doubleName,
        colName: `${doubleName}_col`,
        index: 1,
        makeInfo: (n) => doubleMakeInfo(n),
        info: doubleMakeInfo(doubleName),
        makePoint: () => qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), 1.0),
    }
    var int64Name ='int64'
    var int64MakeInfo = (n) => qdb.Int64ColumnInfo(n)
    var int64Type = {
        name: int64Name,
        colName: `${int64Name}_col`,
        index: 2,
        makeInfo: (n) => int64MakeInfo(n),
        info: int64MakeInfo(int64Name),
        makePoint: () => qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), 1),
    }
    var stringName = 'string'
    var stringMakeInfo = (n) => qdb.StringColumnInfo(n)
    var stringType = {
        name: stringName,
        colName: `${stringName}_col`,
        index: 3,
        makeInfo: (n) => stringMakeInfo(n),
        info: stringMakeInfo(stringName),
        makePoint: () => qdb.StringPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), Buffer.from('content', 'utf8')),
    }
    var timestampName = 'timestamp'
    var timestampMakeInfo = (n) => qdb.TimestampColumnInfo(n)
    var timestampType = {
        name: timestampName,
        colName: `${timestampName}_col`,
        index: 4,
        makeInfo: (n) => timestampMakeInfo(n),
        info: timestampMakeInfo(timestampName),
        makePoint: () => qdb.TimestampPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5)), qdb.Timestamp.fromDate(new Date(2049, 10, 5))),
    }

    var colTypes = [blobType,doubleType,int64Type,stringType,timestampType]

    var columnInfos = [blobType.info, doubleType.info, int64Type.info, stringType.info, timestampType.info]

    before('connect', function (done) {
        insecureCluster.connect(done, done);
        ts = insecureCluster.ts('ts');
    });

    it('should be of correct type', function () {
        test.object(ts).isInstanceOf(qdb.TimeSeries);
    });

    it("should have an 'alias' property", function () {
        test.object(ts).hasProperty('alias');
        test.must(ts.alias()).be.a.string();
    });

    after('Timeseries', function (done) {
        ts.remove(function (err) {
            test.must(err).be.equal(null);

            done();
        });
    });

    // Column Infos are tested in each type test file

    describe('creation', function () {
        var emptyTs

        after('creation', function (done) {
            emptyTs.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should create with empty columns', function (done) {
            emptyTs = insecureCluster.ts('empty-ts')
            emptyTs.remove(function (err) {
                emptyTs.create([], function (err, columns) {
                    test.must(err).be.equal(null);
                    test.must(columns).be.empty()
                    done();
                });
            });
        });

        it('should create multiple columns', function (done) {
            ts.remove(function (err) {
                ts.create(columnInfos, function (err, columns) {
                    test.must(err).be.equal(null);
                    test.must(columns).not.be.empty()
                    test.must(columns.length).be.equal(colTypes.length)
                    colTypes.forEach(function(col) {
                        test.must(columns[col.index].alias()).be.equal(col.name);
                        test.must(columns[col.index].type).be.equal(col.info.type);
                        test.must(columns[col.index].timeseries).be.equal(ts.alias());
                    });
                    done();
                });
            });
        });

        it('should not insert existing column', function (done) {
            ts.insert(columnInfos, function (err, columns) {
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

        it('should insert new columns', function (done) {
            var newColumns = colTypes.map(col => col.makeInfo(`col_${col.name}_${col.index}`))
            ts.insert(newColumns, function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns).not.be.empty()
                test.must(columns.length).be.equal(newColumns.length)

                colTypes.forEach(function(col) {
                    test.must(columns[col.index].alias()).be.equal(`col_${col.name}_${col.index}`);
                    test.must(columns[col.index].type).be.equal(col.info.type);
                    test.must(columns[col.index].timeseries).be.equal(ts.alias());
                });

                done();
            });
        });

        it('should not insert existing columns', function (done) {
            ts.insert(columnInfos, function (err, columns) {
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

        it('should remove timeseries', function (done) {
            ts.remove(function (err) {
                test.must(err).be.equal(null);

                done();
            });
        });
    });

    describe('columns', function () {
        before('init', function (done) {
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

        it('should insert columns', function (done) {
            ts.insert(columnInfos, function (err, columns) {
                test.must(err).be.equal(null);

                done();
            });
        });

        it('should show columns', function (done) {
            ts.columns(function (err, columns) {
                test.must(err).be.equal(null);
                test.must(columns.length).be.equal(columnInfos.length);

                // Database guarantees to return columns in the same order as we created/appended them.
                for (var i = 0; i < columns.length; i++) {
                    test.must(columns[i].alias()).be.equal(columnInfos[i].name);
                    test.must(columns[i].type).be.equal(columnInfos[i].type);
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
        createdColumns = []
        before('init', function (done) {
            ts = insecureCluster.ts("points")

            ts.remove(function (err) {
                ts.create(columnInfos, function (err, columns) {
                    test.must(err).be.equal(null);
                    test.should(columns.length).eql(colTypes.length);
                    colTypes.forEach(function(col) {
                        test.should(columns[col.index].type).eql(col.info.type);
                    });

                    createdColumns = columns

                    done();
                });
            })
        });

        colTypes.forEach(function(current) {
            colTypes.forEach(function(next) {
                if (current === next) return;

                // FIXME(Vianney): Currently we only check IsNumber, but we need to check if the point is a qdb.Int64Point.
                if ((current.name == 'int64' && next.name == 'double') || (current.name == 'double' && next.name == 'int64'))
                {
                    return;
                }
                // FIXME(vianney): Currently we can't check that blobs are not strings
                if ((current.name == 'blob' && next.name == 'string') || (current.name == 'string' && next.name == 'blob'))
                {
                    return;
                }
                it(`should not insert ${current.name} point into ${next.name} column`, function(done) {
                    var point = current.makePoint()
                    createdColumns[next.index].insert([point], function (err) {
                        test.must(err).not.be.equal(null);
                        done();
                    });
                });
            });
        });
    });

    describe('ranges', function () {
        it('should create valid range', function () {
            var begin = new Date(2049, 10, 5, 1);
            var end = new Date(2049, 10, 5, 3);
            var range = qdb.TsRange(qdb.Timestamp.fromDate(begin), qdb.Timestamp.fromDate(end));

            test.object(range).hasProperty('begin');
            test.object(range).hasProperty('end');

            test.must(range.begin.toDate().getTime()).be.equal(begin.getTime());
            test.must(range.end.toDate().getTime()).be.equal(end.getTime());
        });
    });

    describe('ranges from dates', function () {
        it('should create valid range', function () {
            var begin = new Date(2049, 10, 5, 1);
            var end = new Date(2049, 10, 5, 3);
            var range = qdb.TsRange(begin, end);

            test.object(range).hasProperty('begin');
            test.object(range).hasProperty('end');

            test.must(range.begin.toDate().getTime()).be.equal(begin.getTime());
            test.must(range.end.toDate().getTime()).be.equal(end.getTime());
        });
    });

    describe('aggregations', function () {
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
    });
});

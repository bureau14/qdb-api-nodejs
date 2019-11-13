var test = require('unit.js');
var qdb = require('..');
var config = require('./config')

var cluster = new qdb.Cluster(config.insecure_cluster_uri);

describe('Query', function () {
    var columnInfo
    var ts

    before('connect', function (done) {
        cluster.connect(done, done);
    });


    before('init', function (done) {
        columnInfo = [qdb.DoubleColumnInfo('double_col'), qdb.BlobColumnInfo('blob_col'), qdb.Int64ColumnInfo('int64_col'), qdb.TimestampColumnInfo('timestamp_col')]
        ts = cluster.ts("query_test")

        ts.remove(function (err) {
            ts.create(columnInfo, function (err, columns) {
                test.must(err).be.equal(null);
                var doublePoints = [
                    qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), 0.1),
                    qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), 0.2),
                    qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), 0.3)
                ];
                var blobPoints = [
                    qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), Buffer.from("a", "utf8")),
                    qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), Buffer.from("b", "utf8")),
                    qdb.BlobPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), Buffer.from("c", "utf8"))
                ];
                var int64Points = [
                    qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), 1),
                    qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), 2),
                    qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), 3)
                ];
                var timestampPoints = [
                    qdb.TimestampPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1)), qdb.Timestamp.fromDate(new Date(2049, 10, 5, 1))),
                    qdb.TimestampPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2)), qdb.Timestamp.fromDate(new Date(2049, 10, 5, 2))),
                    qdb.TimestampPoint(qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)), qdb.Timestamp.fromDate(new Date(2049, 10, 5, 3)))
                ];
                // encapsulate to make sure everything is inserted properly
                columns[0].insert(doublePoints, function (err) {
                    test.must(err).be.equal(null);
                    columns[1].insert(blobPoints, function (err) {
                        test.must(err).be.equal(null);
                        columns[2].insert(int64Points, function (err) {
                            test.must(err).be.equal(null);
                            columns[3].insert(timestampPoints, function (err) {
                                test.must(err).be.equal(null);
                                done();
                            });
                        });
                    });
                });
            });
        })
    });

    after('Query', function (done) {
        ts.remove(function (err) {
            test.must(err).be.equal(null);

            done();
        });
    });

    it('should retrieve all points', function (done) {
        cluster.query('select * from query_test').run(function (err, output) {
            test.must(err).be.equal(null);
            test.must(output.scanned_point_count).be.equal(12);
            test.must(output.error_message).be.empty();

            test.must(output.column_count).be.equal(6);
            test.must(output.column_names[0]).be.equal('$timestamp');
            test.must(output.column_names[1]).be.equal('$table');
            test.must(output.column_names[2]).be.equal('double_col');
            test.must(output.column_names[3]).be.equal('blob_col');
            test.must(output.column_names[4]).be.equal('int64_col');
            test.must(output.column_names[5]).be.equal('timestamp_col');

            test.must(output.row_count).be.equal(3);

            // first row
            test.must(output.rows[0][0].toDate().getTime()).be.equal((new Date(2049, 10, 5, 1)).getTime());
            test.must(output.rows[0][1]).be.equal('query_test');
            test.must(output.rows[0][2]).be.equal(0.1);
            test.must(output.rows[0][3]).be.equal('a');
            test.must(output.rows[0][4]).be.equal(1);
            test.must(output.rows[0][5].toDate().getTime()).be.equal((new Date(2049, 10, 5, 1)).getTime());

            // second row
            test.must(output.rows[1][0].toDate().getTime()).be.equal((new Date(2049, 10, 5, 2)).getTime());
            test.must(output.rows[0][1]).be.equal('query_test');
            test.must(output.rows[1][2]).be.equal(0.2);
            test.must(output.rows[1][3]).be.equal('b');
            test.must(output.rows[1][4]).be.equal(2);
            test.must(output.rows[1][5].toDate().getTime()).be.equal((new Date(2049, 10, 5, 2)).getTime());

            // third row
            test.must(output.rows[2][0].toDate().getTime()).be.equal((new Date(2049, 10, 5, 3)).getTime());
            test.must(output.rows[0][1]).be.equal('query_test');
            test.must(output.rows[2][2]).be.equal(0.3);
            test.must(output.rows[2][3]).be.equal('c');
            test.must(output.rows[2][4]).be.equal(3);
            test.must(output.rows[2][5].toDate().getTime()).be.equal((new Date(2049, 10, 5, 3)).getTime());

            done();
        });
    });

    it('should have a count query', function (done) {
        cluster.query('select count(int64_col) from query_test').run(function (err, output) {
            test.must(err).be.equal(null);
            // 3 rows so count should be 3
            expected_count = 3;
            // first row, ignore timestamp
            test.must(output.rows[0][0]).be.equal(expected_count);

            done();
        });
    });
});
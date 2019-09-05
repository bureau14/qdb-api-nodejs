var test = require('unit.js');
var qdb = require('..');

describe("Timestamp", function () {
  it("should support nanosecond precision", function () {
    var seconds = 1559084400
    var nanoseconds = 2430012
    var datetime = new qdb.Timestamp(seconds, nanoseconds)

    test.must(datetime.toString()).be.equal('2019-05-28T23:00:00.002430012Z')
    test.must(datetime.toDate().toISOString()).be.equal('2019-05-28T23:00:00.002Z')
  })
})
var test = require('unit.js');
var qdb = require('..');

describe("Timestamp", function () {
  it("should work", function () {
    console.log('@@@')
    var datetime = new qdb.Timestamp(0, 0)
    console.log('!!!')
    console.log(datetime)
    console.log(datetime.toDate())
  })
})
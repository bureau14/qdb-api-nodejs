var binary = require('@mapbox/node-pre-gyp');
var path = require('path')
var qdb_path = binary.find(path.resolve(path.join(__dirname, '/package.json')));
var quasardb = require(qdb_path);

// Api customisation
const DATE_LENGTH = 'YYYY-MM-DDTHH:MM:SS'.length

quasardb.Timestamp.prototype.toString = function () {
  const formattedDateTime = new Date(this.seconds * 1000).toISOString().substr(0, DATE_LENGTH)
  const formattedNanoseconds = `${this.nanoseconds}`.padStart(9, '0')
  return `${formattedDateTime}.${formattedNanoseconds}Z`
}

module.exports = exports = quasardb;
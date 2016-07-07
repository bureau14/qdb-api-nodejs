var binary = require('node-pre-gyp');
var path = require('path')
var qdb_path = binary.find(path.resolve(path.join(__dirname,'/package.json')));

var quasardb = require(qdb_path);

module.exports = exports = quasardb;
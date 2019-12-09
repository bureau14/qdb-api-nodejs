/** 
 * @overview The Offical QuasarDB Node.js API
 * 
 * @version 3.5.0
 * @copyright 2019
 * 
 */

var binary = require('node-pre-gyp');
var path = require('path')
var qdb_path = binary.find(path.resolve(path.join(__dirname, '/package.json')));
var qdb = require(qdb_path);

/**
 * The Cluster class represents a connection to a QuasarDB cluster.
 * 
 * For an insecure cluster only the cluster uri is required. For a secure cluster the cluster uri, cluster public key file path and user's config file are all required.
 * 
 * @constructor
 * @param {string} uri - The URI of the QuasarDB cluster to connect to
 * @param {string} [clusterPublicKeyPath] - File path to the QuasarDB cluster public key
 * @param {string} [usersConfigPath] - File path to the user's configuration file
 */
qdb.Cluster

/**
 * This callback will return either an error or a successfully connected qdb.Cluster
 * @callback qdb.Cluster~connectCallback
 * @param {error} error - Error containing information about connection failure
 * @param {qdb.Cluster} cluster - A connected qdb.Cluster object
 */

/**
 * Attempts to connect to a QuasarDB cluster
 * 
 * @method
 * @param {qdb.Cluster~connectCallback} cb - a callback returning either an error or successfully connected qdb.Cluster object
 */
qdb.Cluster.prototype.connect

/**
 * Constructs a QuasarDB timeseries object
 * 
 * @method
 * @param {string} name - The timeseries name or alias
 * 
 * @returns {qdb.Timeseries}
 */
qdb.Cluster.prototype.ts

/**
 * Constructs a QuasarDB query object
 * 
 * @method
 * @param {string} sql - The sql query
 * @see {@link https://doc.quasardb.net/master/queries/index.html}
 * 
 * @returns {qdb.Query}
 */
qdb.Cluster.prototype.query

/**
 * Represents a timeseries
 * 
 * @class
 * @hideconstructor
 * @see [qdb.Cluster#ts]{@link qdb.Cluster} for constructing a new qdb.Timeseries
 */
qdb.Timeseries

/**
 * This callback will return either an error or null if the timeseries was created successfully
 * @callback qdb.Timeseries~createCallback
 * @param {error} error - Error containing information about timeseries creation failure
 */

/**
 * Creates a timeseries
 * 
 * @method
 * @param {ColumnInfo} columnsInfo
 */
qdb.Timeseries.prototype.create

/**
 * Removes a timeseries
 * 
 * @method
 */
qdb.Timeseries.prototype.remove

/**
 * Inserts columns
 * 
 * @method
 */
qdb.Timeseries.prototype.insert

/**
 * Fetches a timeseries columns
 * 
 * @method
 */
qdb.Timeseries.prototype.columns

/**
 * Attach a tag to a table
 * 
 * @method
 */
qdb.Timeseries.prototype.attachTag


/**
 * Represents a query
 * 
 * @class
 * @hideconstructor
 * @see [qdb.Cluster#query]{@link qdb.Cluster} for constructing a new qdb.Query
 */
qdb.Query

/**
 * This callback will return either an error or the query results if the query was executed successfully
 * @callback qdb.Query~runCallback
 * @param {error} error - Error containing information about timeseries creation failure
 * @param {Object} results - The query results
 */

/**
 * Executes the query
 * 
 * @method
 * @param {qdb.Query~runCallback} cb - a callback returning either an error or query results
 */
qdb.Query.prototype.run

/**
 * An object representing meta data about a QuasarDB timeseries column
 * 
 * @typedef {Object} ColumnInfo
 * @property {string} name
 * @property {TS_COLUMN_DOUBLE | TS_COLUMN_INT64 | TS_COLUMN_BLOB | TS_COLUMN_TIMESTAMP} type
 */

/** 
 * @function DoubleColumnInfo
 * @param {string} name - The desired name of the column
 * 
 * @returns {ColumnInfo} - A ColumnInfo with the type set to TS_COLUMN_DOUBLE
 */
qdb.DoubleColumnInfo

/** 
 * @function Int64ColumnInfo
 * @param {string} name - The desired name of the column
 * 
 * @returns {ColumnInfo} - A ColumnInfo with the type set to TS_COLUMN_INT64
 */
qdb.Int64ColumnInfo

/** 
 * @function BlobColumnInfo
 * @param {string} name - The desired name of the column
 * 
 * @returns {ColumnInfo} - A ColumnInfo with the type set to TS_COLUMN_BLOB
 */
qdb.BlobColumnInfo

/** 
 * @function TimestampColumnInfo
 * @param {string} name - The desired name of the column
 * 
 * @returns {ColumnInfo} - A ColumnInfo with the type set to TS_COLUMN_TIMESTAMP
 */
qdb.TimestampColumnInfo

/**
 * @constant TS_COLUMN_DOUBLE
 */
qdb.TS_COLUMN_DOUBLE

/**
 * @constant TS_COLUMN_INT64
 */
qdb.TS_COLUMN_INT64

/**
 * @constant TS_COLUMN_BLOB
 */
qdb.TS_COLUMN_BLOB

/**
 * @constant TS_COLUMN_TIMESTAMP
 */
qdb.TS_COLUMN_TIMESTAMP

qdb.DoubleColumn
qdb.Int64Column
qdb.BlobColumn
qdb.TimestampColumn

qdb.DoublePoint
qdb.Int64Point
qdb.BlobPoint
qdb.TimestampPoint

qdb.TsRange

/**
 * A high resolution timestamp able to represent a DateTime to nanosecond precision
 * 
 * @constructor
 * @param {number} seconds - The number of seconds since Unix Epoch
 * @param {number} nanoseconds - The number of nanoseconds. Should be in the range 0 - 999999999
 */
qdb.Timestamp

/**
 * A convenience method to construct a qdb.Timestamp from a Javascript Date.
 * 
 * @static
 * @param {Date} date - A Javascript date object
 * @returns {qdb.Timestamp}
 */
qdb.Timestamp.fromDate

/**
 * A convenience method to convert a qdb.Timestamp to a Javascript Date. Warning: this conversion can result in a loss of precision.
 * 
 * @static
 * @returns {Date}
 */
qdb.Timestamp.toDate

// Api customisation
const DATE_LENGTH = 'YYYY-MM-DDTHH:MM:SS'.length

/**
 * Provides an ISO8601 string representation of the qdb.Timestamp object. This is similar to Date#toISOString but with nanosecond precision.
 * 
 * @method
 * @returns {string}
 */
qdb.Timestamp.prototype.toString = function () {
  const formattedDateTime = new Date(this.seconds * 1000).toISOString().substr(0, DATE_LENGTH)
  const formattedNanoseconds = `${this.nanoseconds}`.padStart(9, '0')
  return `${formattedDateTime}.${formattedNanoseconds}Z`
}

module.exports = exports = qdb;
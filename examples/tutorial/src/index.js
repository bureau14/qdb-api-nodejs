// import-start
var qdb = require('quasardb')
// import-end


function connect(callback) {
  // connect-start
  var cluster = new qdb.Cluster('qdb://127.0.0.1:2836')

  cluster.connect(function() {
    // Connected successfully
    return callback(null, cluster)
  }, function(err) { 
    // Connection error
    return callback(err, null)
  })
  // connect-end
}

function secureConnect(callback) {
  // secure-connect-start
  var secureCluster = new qdb.Cluster(
    'qdb://127.0.0.1:2836', 
    '/path/to/cluster_public.key', 
    '/path/to/user_private.key'
  )

  secureCluster.connect(function() {
    // Connected successfully
    return callback(null, cluster)
  }, function(err) { 
    // Connection error
    return callback(err, null)
  })
  // secure-connect-end
}

function createTable(cluster, callback) {
  // create-table-start
  var table = cluster.ts('stocks')
  table.create([
    qdb.DoubleColumnInfo('open'),
    qdb.DoubleColumnInfo('close'),
    qdb.Int64ColumnInfo('volume')
  ], function(err) {
    if (err) {
      // Failed to create table
      return callback(err)
    }
    // Successfully created table
    // create-table-end

    // tags-start
    table.attachTag("nasdaq", function (err) {
      if (err) {
        callback(err)
      }

      callback()
    })
    // tags-end
  })
}

function batchInsert(cluster, callback) {
  // not supported yet
}

function bulkRead(cluster, callback) {
  // not supported yet
}

function columnInsert(cluster, callback) {
  // column-insert-start
  var table = cluster.ts('stocks')
  table.columns(function(err, columns) {
    if (err) {
      return callback(err)
    }

    var open = columns[0]
    var close = columns[1]
    var volume = columns[2]

    open.insert([
      qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2000, 10, 5, 2)), 3.40),
      qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2020, 10, 5, 4)), 3.50)
    ], function(err) {
      if (err) {
        return callback(err)
      }

      close.insert([
        qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2000, 10, 5, 2)), 3.50),
        qdb.DoublePoint(qdb.Timestamp.fromDate(new Date(2020, 10, 5, 4)), 3.55)
      ], function(err) {
        if (err) {
          return callback(err)
        }
        volume.insert([
          qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2000, 10, 5, 2)), 10000),
          qdb.Int64Point(qdb.Timestamp.fromDate(new Date(2020, 10, 5, 4)), 7500)
        ], function(err) {
          if (err) {
            return callback(err)
          }
          return callback()
        })
      })
    })
  })
  // column-insert-end
}

function columnRead(cluster, callback) {
  // column-get-start
  var range = qdb.TsRange(
    qdb.Timestamp.fromDate(new Date(2000, 10, 5, 2)), 
    qdb.Timestamp.fromDate(new Date(2020, 10, 5, 5))
  )

  var table = cluster.ts('stocks')
  table.columns(function(err, columns) {
    if (err) {
      return callback(err)
    }

    var open = columns[0]
    var close = columns[1]
    var volume = columns[2]

    open.ranges([range], function(err, openPoints) {
      if (err) {
        return callback(err)
      }

      close.ranges([range], function(err, closePoints) {
        if (err) {
          return callback(err)
        }

        volume.ranges([range], function(err, volumePoints) {
          if (err) {
            return callback(err)
          }

          for (var i = 0; i < openPoints.length; i++) {
            timestamp = openPoints[i].timestamp
            open = openPoints[i].value
            close = closePoints[i].value
            volume = volumePoints[i].value
            console.log(`timestamp: ${timestamp}, open: ${open}, close: ${close}, volume: ${volume}`)
          }

          return callback()
        })
      })
    })
  })
  // column-get-end
}

function query(cluster, callback) {
  // query-start
  cluster.query("SELECT SUM(volume) FROM stocks").run(function(err, result) {
    if (err) {
      return callback(err)
    }

    table = result.tables[0]

    for (var i = 0 ; i < table.rows.length; i++) {
      console.log(`${table.rows[i][1]}`)
    }
    return callback()
  })
  // query-end
}

function dropTable(cluster, callback) {
  // drop-table-start
  table = cluster.ts("stocks")
  table.remove(function(err) {
    if (err) {
      callback(err)
    }

    callback()
  })
  // drop-table-end
}

connect(function(err, cluster) {
  if (err) {
    throw new Error("Failed to connect to cluster")
  }
  createTable(cluster, function (err) {
    if (err) {
      throw new Error("Failed to create table")
    }
    columnInsert(cluster, function(err) {
      if (err) {
        throw new Error("Failed to column insert")
      }

      columnRead(cluster, function(err) {
        if (err) {
          throw new Error("Failed to column read")
        }

        query(cluster, function(err) {
          if (err) {
            throw new Error("Failed to query table")
          }

          dropTable(cluster, function(err) {
            if (err) {
              throw new Error("Failed to drop table")
            }
          })
        })
      })
    })

  })
})
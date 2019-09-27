var qdb = require('quasardb')

function connect(callback) {
  console.log("connect")
  var cluster = new qdb.Cluster('qdb://127.0.0.1:2836')

  cluster.connect(function() {
    // connected successfully
    return callback(null, cluster)
  }, function(err) { 
    // connection error
    return callback(err, null)
  })
}

function secureConnect(callback) {

}

function createTable(cluster, callback) {
  var table = cluster.ts('stocks')
  table.create([
    qdb.DoubleColumnInfo('open'),
    qdb.DoubleColumnInfo('close'),
    qdb.Int64ColumnInfo('volume')
  ], function(err) {
    if (err) { 
      return callback(err)
    }

    callback()
  })
}

function batchInsert(cluster, callback) {
  // not supported yet
}

function bulkRead(cluster, callback) {
  // not supported yet
}

function columnInsert(cluster, callback) {
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
}

function columnRead(cluster, callback) {
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
}

function query(cluster, callback) {
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
}

function dropTable(cluster, callback) {
  table = cluster.ts("stocks")
  table.remove(callback)
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
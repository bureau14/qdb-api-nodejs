var qdb = require('..');

var c = new qdb.Cluster('qdb://127.0.0.1:2836');
c.connect(function() {
    var b = c.blob('bam');

    b.update(new Buffer('bam_content'), function (err) {
        if (err) throw err.message;

        b.getMetadata(function(err, meta) {
            if (err) throw err.message;

            console.log(JSON.stringify(meta, null, ' '));
        });
    });
}, function(err) {
    throw new Error("an error occurred in cluster launch: " + err.message);
});

/*var qdb = require('../build/Release/qdb.node');
var c = new qdb.Cluster('qdb://127.0.0.1:2836');

function getter(t, i)
{
    if (i < 100)
    {
        t.getEntries(function(err, entries)
        {
            if (err != 0)
            {
                console.log("could not get entries");
                return;
            }

            entries = null;

            getter(t, i + 1);
        });
    }
}

c.connect(function()
{
    console.log("connected successfully");

    // get tags en masse, should not leak
    var t = c.tag('dasTag');

    getter(t, 0);
},
function(err)
{   // error
    console.log("cannot connect: %s", err);
});
*/
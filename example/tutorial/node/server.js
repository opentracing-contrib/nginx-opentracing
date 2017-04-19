const express = require('express');
const program = require('commander');
const formidable = require('express-formidable');
const lightstep = require('lightstep-tracer');
const opentracing = require('opentracing');
const sqlite3 = require('sqlite3');
const fs = require('fs');
const uuid = require('uuid/v1');
const sharp = require('sharp');
const path = require('path');

const common = require('./common')
const tracing_middleware = require('./opentracing-express')

const thumbnailWidth = 320;
const thumbnailHeight = 200;

program
  .option('p, --port <n>', 'Port', parseInt)
  .option('r, --data_root <data_root>', 'Data Root')
  .option('a, --access_token <access_token>', 'Access Token')
  .parse(process.argv);

if (typeof program.port === 'undefined') {
  console.error('no port given!');
  process.exit(1);
}

if (typeof program.data_root === 'undefined') {
  console.error('no data_root given!');
  process.exit(1);
}

if (typeof program.access_token === 'undefined') {
  console.error('no access_token given!');
  process.exit(1);
}

const databasePath = path.join(program.data_root, common.databaseName);
const imageRoot = path.join(program.data_root, '/images/');
const accessToken = program.access_token;

tracer = new lightstep.Tracer({
  access_token: accessToken,
  component_name : 'virtual-zoo'
});

var db = new sqlite3.Database(databasePath);

function traceCallback(parentSpan, operationName, callback) {
  var span = tracer.startSpan(operationName, {childOf: parentSpan});
  return function () {
    span.finish();
    return callback.apply(this, arguments);
  };
}

var app = express();
app.use(tracing_middleware.middleware({tracer: tracer }));
app.use(formidable());
app.set('view engine', 'pug');
app.set('views', path.join(__dirname, '/views'));


app.get('/', function (req, res) {
  stmt = 'select uuid, name from animals order by name';
  db.all(stmt,
    traceCallback(req.span, stmt, function (err, rows) {
      console.log(err);
      // TODO: Check errors.
      var animals = rows.map(function (row) {
        return {
          name: row.name,
          profile: '/animal?id=' + row.uuid,
          thumbnail_pic: '/' + row.uuid + '_thumb.jpg'
        };
      });
      var table = [];
      while (animals[0]) {
        table.push(animals.splice(0, 3));
      }
      res.render('index', { animals: table });
    }));
});

app.get('/animal', function (req, res) {
  stmt = db.prepare(
      'select name, animal from animals where uuid = ?')
  stmt.get(req.query.id, function (err, row) {
    // TODO: Check for errors.
    res.render('animal', {
      title: row.name,
      name: row.name,
      animal: row.animal,
      profile_pic: '/' + req.query.id + '.jpg'
    });
  });
});

app.post('/upload/animal', (req, res) => {
  console.log(req.files.profile_pic.type);
  console.log(req.fields.name);
  console.log(req.files.profile_pic.path);

  id = uuid()

  profileFilename = imageRoot + id + '.jpg';
  thumbnailFilename = imageRoot + id + '_thumb.jpg';
  profilePic = sharp(req.files.profile_pic.path);
  profilePic.toFile(profileFilename,
      traceCallback(req.span, 'copyProfilePic', function (err) {}));

  profilePic.resize(thumbnailWidth, thumbnailHeight).toFile(thumbnailFilename,
      traceCallback(req.span, 'resizeProfilePic', function (err) {}));

  var stmtPattern = 'insert into animals values (?, ?, ?)';
  var stmt = db.prepare(stmtPattern);
  stmt.run(
      id,
      req.fields.animal,
      req.fields.name,
      traceCallback(req.span, stmtPattern, function (err) {
        // TODO: Check for errors.
        res.redirect(303, '/');
      }));
});

app.listen(program.port, function() {
  console.log('Listening on ' + program.port.toString());
});

const express = require('express');
const program = require('commander');
const lightstep = require('lightstep-tracer');
const opentracing = require('opentracing');
const sqlite3 = require('sqlite3');
const uuid = require('uuid/v1');
const sharp = require('sharp');
const path = require('path');

const common = require('./common');
const tracingMiddleware = require('./opentracing-express');

const thumbnailWidth = 320;
const thumbnailHeight = 200;

program.option('p, --port <n>', 'Port', parseInt)
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

const tracer = new lightstep.Tracer(
    { access_token: accessToken, component_name: 'virtual-zoo' });

const db = new sqlite3.Database(databasePath);
db.run('PRAGMA journal_mode = WAL');
db.configure('busyTimeout', 15000);

function onExit() {
  db.close();
  process.exit(0);
}
process.on('SIGINT', onExit);
process.on('SIGTERM', onExit);

function traceCallback(spanOptions, operationName, callback) {
  const span = tracer.startSpan(operationName, spanOptions);
  return (...args) => {
    span.finish();
    return callback(...args);
  };
}

const app = express();
app.use(tracingMiddleware.middleware({ tracer: tracer }));
app.set('view engine', 'pug');
app.set('views', path.join(__dirname, '/views'));

app.get('/', (req, res) => {
  const stmt = 'select uuid, name from animals order by name';
  db.all(stmt, traceCallback({ childOf: req.span }, stmt, (err, rows) => {
           if (err) {
             console.log(err);
             res.status(500).send('Failed to query animals!');
             return;
           }
           const animals = rows.map((row) => {
             return {
               name: row.name,
               profile: '/animal?id=' + row.uuid,
               thumbnail_pic: '/' + row.uuid + '_thumb.jpg'
             };
           });
           const table = [];
           while (animals[0]) {
             table.push(animals.splice(0, 3));
           }
           res.render('index', { animals: table },
                      traceCallback({ childOf: req.span }, 'render index',
                                    (err, html) => {
                                      if (err) {
                                        console.log(err);
                                        res.status(500).send(
                                            'Failed to render index!');
                                      } else {
                                        res.send(html);
                                      }
                                    }));
         }));
});

app.get('/animal', (req, res) => {
  const stmt = db.prepare('select name, animal from animals where uuid = ?')
  stmt.get(req.query.id, function(err, row) {
    // TODO: Check for errors.
    res.render(
        'animal', {
          title: row.name,
          name: row.name,
          animal: row.animal,
          profile_pic: '/' + req.query.id + '.jpg'
        },
        traceCallback({ childOf: req.span }, 'render animal', (err, html) => {
          if (err) {
            console.log(err);
            res.status(500).send('Failed to render animal');
          } else {
            res.send(html);
          }
        }));
  });
  stmt.finalize();
});

app.post('/upload/animal', (req, res) => {
  const id = uuid();

  const profileFilename = imageRoot + id + '.jpg';
  const thumbnailFilename = imageRoot + id + '_thumb.jpg';
  const profilePic = sharp(req.get('admit-profile-pic'));
  profilePic.toFile(
      profileFilename,
      traceCallback(
          { references: [opentracing.followsFrom(req.span.context())] },
          'copyProfilePic', (err) => {}));

  profilePic.resize(thumbnailWidth, thumbnailHeight)
      .toFile(thumbnailFilename,
              traceCallback(
                  { references: [opentracing.followsFrom(req.span.context())] },
                  'resizeProfilePic', (err) => {}));

  const stmtPattern = 'insert into animals values (?, ?, ?)';
  const stmt = db.prepare(stmtPattern);
  stmt.run(id, req.get('admit-animal'), req.get('admit-name'),
           traceCallback({ childOf: req.span }, stmtPattern, (err) => {
             if (err) {
               console.log(err);
               res.status(500).send('Failed to admit animal');
             } else {
               res.redirect(303, '/');
             }
           }));
  stmt.finalize();
});

app.listen(program.port,
           () => { console.log('Listening on ' + program.port.toString()); });

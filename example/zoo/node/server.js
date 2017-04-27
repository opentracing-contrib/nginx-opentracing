const express = require('express');
const program = require('commander');
const lightstep = require('lightstep-tracer');
const opentracing = require('opentracing');
const sqlite3 = require('sqlite3');
const uuid = require('uuid/v1');
const sharp = require('sharp');
const path = require('path');
const winston = require('winston');

const common = require('./common');
const tracingMiddleware = require('./opentracing-express');

const thumbnailWidth = 320;
const thumbnailHeight = 200;

program.option('p, --port <n>', 'Port', parseInt)
    .option('r, --data_root <data_root>', 'Data Root')
    .option('a, --access_token <access_token>', 'Access Token')
    .parse(process.argv);

if (typeof program.port === 'undefined') {
  winston.error('no port given!');
  process.exit(1);
}

if (typeof program.data_root === 'undefined') {
  winston.error('no data_root given!');
  process.exit(1);
}

if (typeof program.access_token === 'undefined') {
  winston.error('no access_token given!');
  process.exit(1);
}

const databasePath = path.join(program.data_root, common.databaseName);
const imageRoot = path.join(program.data_root, '/images/');
const accessToken = program.access_token;

const tracer = new lightstep.Tracer(
    { access_token: accessToken, component_name: 'zoo' });
opentracing.initGlobalTracer(tracer);

const db = new sqlite3.Database(databasePath);
db.run('PRAGMA journal_mode = WAL');
db.configure('busyTimeout', 15000);

function onExit() {
  db.close();
  process.exit(0);
}
process.on('SIGINT', onExit);
process.on('SIGTERM', onExit);

const app = express();
app.use(tracingMiddleware.middleware({ tracer }));
app.set('view engine', 'pug');
app.set('views', path.join(__dirname, '/views'));

function selectAnimals(req) {
  const stmt = 'select uuid, name from animals order by name';
  const span = tracer.startSpan(stmt, { childOf: req.span });
  span.setTag('component', 'SQLite');
  const executer = (resolve, reject) => {
    db.all(stmt, (err, rows) => {
      if (err) {
        winston.error('Unable to select!', { error: err });
        span.log({
          event: 'error',
          'error.object': err,
        });
        span.setTag('error', true);
        span.finish();
        reject(err);
      } else {
        span.finish();
        resolve(rows);
      }
    });
  };
  return new Promise(executer);
}

function selectAnimal(req) {
  const stmtPattern = 'select name, animal from animals where uuid = ?';
  const span = tracer.startSpan(stmtPattern, { childOf: req.span });
  span.setTag('component', 'SQLite');
  const stmt = db.prepare(stmtPattern);
  const executer = (resolve, reject) => {
    stmt.get(req.query.id, (err, row) => {
      if (err) {
        winston.error('Unable to select!', { error: err });
        span.log({
          event: 'error',
          'error.object': err,
        });
        span.setTag('error', true);
        span.finish();
        reject(err);
      } else {
        span.finish();
        resolve(row);
      }
    });
    stmt.finalize();
  };
  return new Promise(executer);
}

function insertAnimal(req, id) {
  const stmtPattern = 'insert into animals values (?, ?, ?)';
  const span = tracer.startSpan(stmtPattern, { childOf: req.span });
  span.setTag('component', 'SQLite');
  const stmt = db.prepare(stmtPattern);
  const executer = (resolve, reject) => {
    stmt.run(id, req.get('admit-animal'), req.get('admit-name'), (err) => {
      if (err) {
        winston.error('Unable to insert!', { error: err });
        span.log({
          event: 'error',
          'error.object': err,
        });
        span.setTag('error', true);
        span.finish();
        reject(err);
      } else {
        span.finish();
        resolve();
      }
    });
    stmt.finalize();
  };
  return new Promise(executer);
}

function resizeProfileImage(req, id) {
  const thumbnailFilename = `${imageRoot}${id}_thumb.jpg`;
  const span = tracer.startSpan(
      'resizeProfileImage',
      { references: [opentracing.followsFrom(req.span.context())] });
  span.setTag('component', 'sharp');
  const executer = (resolve, reject) => {
    sharp(req.get('admit-profile-pic'))
        .resize(thumbnailWidth, thumbnailHeight)
        .toFile(thumbnailFilename, (err) => {
          if (err) {
            winston.error('Unable to resize profile image!', { error: err });
            span.log({
              event: 'error',
              'error.object': err,
            });
            span.setTag('error', true);
            span.finish();
            reject(err);
          } else {
            span.finish();
            resolve();
          }
        });
  };
  return new Promise(executer);
}

function copyTransformProfileImage(req, id) {
  const profileFilename = `${imageRoot}${id}.jpg`;
  const span = tracer.startSpan(
      'copyTransformProfileImage',
      { references: [opentracing.followsFrom(req.span.context())] });
  span.setTag('component', 'sharp');
  const executer = (resolve, reject) => {
    sharp(req.get('admit-profile-pic')).toFile(profileFilename, (err) => {
      if (err) {
        winston.error('Unable to resize profile image!', { error: err });
        span.log({
          event: 'error',
          'error.object': err,
        });
        span.setTag('error', true);
        span.finish();
        reject(err);
      } else {
        span.finish();
        resolve();
      }
    });
  };
  return new Promise(executer);
}

app.get('/', (req, res) => {
  selectAnimals(req).then(
      (rows) => {
        const animals = rows.map(row => ({
                                   name: row.name,
                                   profile: `/animal?id=${row.uuid}`,
                                   thumbnail_pic: `/${row.uuid}_thumb.jpg`,
                                 }));
        const table = [];
        while (animals[0]) {
          table.push(animals.splice(0, 3));
        }
        res.render('index', { animals: table });
      },
      () => { res.status(500).send('Failed to view animals!'); });
});

app.get('/animal', (req, res) => {
  selectAnimal(req).then(
      (row) => {
        res.render('animal', {
          title: row.name,
          name: row.name,
          animal: row.animal,
          profile_pic: `/${req.query.id}.jpg`,
        });
      },
      () => { res.status(500).send('Failed to view animal!'); });
});

app.post('/upload/animal', (req, res) => {
  const id = uuid();
  resizeProfileImage(req, id);
  copyTransformProfileImage(req, id);
  insertAnimal(req, id).then(
      () => { res.redirect(303, '/'); },
      () => { res.status(500).send('Failed to upload animal'); });
});

app.listen(program.port,
           () => { winston.log('info', `Listening on ${program.port}`); });

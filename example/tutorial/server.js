const express = require('express');
const program = require('commander');
const formidable = require('express-formidable');
const sqlite3 = require('sqlite3');
const fs = require('fs');
const uuid = require('uuid/v1');
const sharp = require('sharp');

const common = require('./common')

const thumbnailWidth = 320;
const thumbnailHeight = 200;

var db = new sqlite3.Database(common.databasePath);
var app = express();

app.use(formidable());
app.set('view engine', 'pug');

program
  .option('p, --port <n>', 'Port', parseInt)
  .parse(process.argv);

if (typeof program.port === 'undefined') {
  console.error('no port given!');
  process.exit(1);
}

app.get('/', function (req, res) {
  // res.render('index', {title: 'Hey', message: 'Hello there!'});
  res.render('index', {title: 'Hey', message: 'Hello there!',
    name:'Pangy', animal:'The Pangolin', description:'Lives in Africa. Likes to eats ants.'});
});

app.post('/upload/animal', (req, res) => {
  console.log(req.files.profile_pic.type);
  console.log(req.fields.name);
  console.log(req.files.profile_pic.path);

  filename = common.imageRoot + uuid() + '.jpeg';
  thumbnailFilename = common.imageRoot + uuid() + '.jpeg';
  profilePic = sharp(req.files.profile_pic.path);
  profilePic.toFile(filename);
  profilePic.resize(thumbnailWidth, thumbnailHeight).toFile(thumbnailFilename);

  db.serialize(function() {
    var stmt = db.prepare('insert into animals values (?, ?, ?, ?, ?, ?)');
    stmt.run(
        req.fields.animal,
        req.fields.name,
        req.fields.habitat,
        req.fields.description,
        filename,
        thumbnailFilename
    );
    stmt.finalize();
  });

  res.send('Welcome!');
});

app.listen(program.port, function() {
  console.log('Listening on ' + program.port.toString());
});

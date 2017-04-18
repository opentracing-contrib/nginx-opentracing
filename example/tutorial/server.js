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
app.use('/images', express.static(common.imageRoot));
app.set('view engine', 'pug');

program
  .option('p, --port <n>', 'Port', parseInt)
  .parse(process.argv);

if (typeof program.port === 'undefined') {
  console.error('no port given!');
  process.exit(1);
}

app.get('/', function (req, res) {
  db.all('select uuid, name from animals order by name',
    function (err, rows) {
      console.log(err);
      console.log(rows);
      // TODO: Check errors.
      var animals = rows.map(function (row) {
        return {
          name: row.name,
          profile: 'http://localhost:3000/animal?id=' + row.uuid,
          thumbnail_pic: '/images/' + row.uuid + '_thumb.jpg'
        };
      });
      var table = [];
      while (animals[0]) {
        table.push(animals.splice(0, 3));
      }
      res.render('index', { animals: table });
    });
});

app.get('/animal', function (req, res) {
  stmt = db.prepare(
      'select name, animal, description from animals where uuid = ?')
  stmt.get(req.query.id, function (err, row) {
    // TODO: Check for errors.
    res.render('animal', {
      title: row.name,
      name: row.name,
      animal: row.animal,
      description: row.description,
      profile_pic: 'http://localhost:3000/images/' + req.query.id + '.jpg'
    });
  });
});

// app.post('/upload/animal', (req, res) => {
//   console.log(req.files.profile_pic.type);
//   console.log(req.fields.name);
//   console.log(req.files.profile_pic.path);

//   id = uuid()

//   profileFilename = common.imageRoot + id + '.jpg';
//   thumbnailFilename = common.imageRoot + id + '_thumb.jpg';
//   profilePic = sharp(req.files.profile_pic.path);
//   profilePic.toFile(profileFilename);
//   profilePic.resize(thumbnailWidth, thumbnailHeight).toFile(thumbnailFilename);

//   db.serialize(function() {
//     var stmt = db.prepare('insert into animals values (?, ?, ?, ?, ?)');
//     stmt.run(
//         id,
//         req.fields.animal,
//         req.fields.name,
//         req.fields.habitat,
//         req.fields.description
//     );
//     stmt.finalize();
//   });

//   res.redirect('/animal?id=' + id);
// });

app.listen(program.port, function() {
  console.log('Listening on ' + program.port.toString());
});

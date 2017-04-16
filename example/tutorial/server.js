const express = require('express');
const program = require('commander');
const formidable = require('express-formidable');
const sqlite3 = require('sqlite3');
const fs = require('fs');
const uuid = require('uuid/v1');
const sharp = require('sharp');

const dataRoot = './zoo-data';
var db = new sqlite3.Database('zoo.sqlite')
var app = express();

app.use(formidable());

program
  .option('p, --port <n>', 'Port', parseInt)
  .parse(process.argv);

if (typeof program.port === 'undefined') {
  console.error('no port given!');
  process.exit(1);
}

app.get('/', function (req, res) {
  res.send('Hello World!');
});

app.post('/upload/animal', (req, res) => {
  console.log(req.files.profile_pic.type);
  console.log(req.fields.name);
  console.log(req.files.profile_pic.path);
  filename = dataRoot + '/' + uuid() + '.jpeg';
  sharp(req.files.profile_pic.path)
    .toFile(filename);
  // fs.readFile(req.files.profile_pic.path, 'base64', function(err, data) {
  //   if (err) {
  //     return console.log(err);
  //   }
  //   db.serialize(function() {
  //     var stmt = db.prepare('insert into animals values (?, ?, ?, ?, ?)');
  //     stmt.run(
  //         req.fields.name,
  //         req.fields.habitat,
  //         req.fields.description,
  //         req.files.profile_pic.type,
  //         data
  //     );
  //     stmt.finalize();
  //   });
  // });
});

app.listen(program.port, function() {
  console.log('Listening on ' + program.port.toString());
});

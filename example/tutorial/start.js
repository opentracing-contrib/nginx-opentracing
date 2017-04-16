const sqlite3 = require('sqlite3');
const fs = require('fs');

const dataRoot = './zoo-data';
var db = new sqlite3.Database(dataRoot + '/zoo.sqlite')

if (!fs.existsSync(dataRoot)) {
  fs.mkdirSync(dataRoot);
}

db.serialize(function () {
  db.run(`create table if not exists animals (
    name text not null,
    habitat text not null,
    description text not null,
    image_type text,
    image blob
  )`);
});

db.close();

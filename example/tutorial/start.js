const sqlite3 = require('sqlite3');
const fs = require('fs');
const common = require('./common')

if (!fs.existsSync(common.dataRoot)) {
  fs.mkdirSync(common.dataRoot);
  fs.mkdirSync(common.imageRoot);
}

var db = new sqlite3.Database(common.databasePath);

db.serialize(function () {
  db.run(`create table if not exists animals (
    uuid character(36) primary key,
    animal text not null,
    name text not null,
    habitat text not null,
    description text not null
  )`);
});

db.close();

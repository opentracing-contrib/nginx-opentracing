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
    animal text not null,
    name text not null,
    habitat text not null,
    description text not null,
    thumbnail_img_path text not null,
    profile_img_path text not null
  )`);
});

db.close();

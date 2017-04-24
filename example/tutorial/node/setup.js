const sqlite3 = require('sqlite3');
const fs = require('fs');
const common = require('./common') const program = require('commander');
const path = require('path');

program.option('r, --data_root <data_root>', 'Data Root').parse(process.argv);

if (typeof program.data_root === 'undefined') {
  console.error('no daa_root given!');
  process.exit(1);
}

databasePath = path.join(program.data_root, common.databaseName);

var db = new sqlite3.Database(databasePath);

db.serialize(function() {
  db.run(`create table if not exists animals (
    uuid character(36) primary key,
    animal text not null,
    name text not null
  )`);
});

db.close();

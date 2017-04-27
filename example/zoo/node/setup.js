const sqlite3 = require('sqlite3');
const common = require('./common');
const program = require('commander');
const winston = require('winston');
const path = require('path');

program.option('r, --data_root <data_root>', 'Data Root').parse(process.argv);

if (typeof program.data_root === 'undefined') {
  winston.error('no data_root given!');
  process.exit(1);
}

const databasePath = path.join(program.data_root, common.databaseName);
const db = new sqlite3.Database(databasePath);

const stmt = `create table if not exists animals (
    uuid character(36) primary key,
    animal text not null,
    name text not null
  )`;
db.run(stmt, (err) => {
  db.close();
  if (err) {
    winston.error('Failed to create animal table!', { statement: stmt });
  }
});

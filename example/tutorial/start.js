var sqlite3 = require('sqlite3');
var db = new sqlite3.Database('zoo.sqlite')

db.serialize(function () {
  db.run('drop table if exists animals');
  db.run(`create table animals (
    name text not null,
    habitat text not null,
    description text not null,
    image blob
  )`);
});

db.close();

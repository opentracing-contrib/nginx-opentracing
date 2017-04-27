const request = require('request');
const fs = require('fs');
const path = require('path');
const program = require('commander');

program.option('r, --data_root <data_root>', 'Data Root')
    .option('n, --num_requests <num_requests>', 'Number of Requests', 1)
    .parse(process.argv);

if (typeof program.data_root === 'undefined') {
  console.error('no data_root given!');
  process.exit(1);
}

const files = fs.readdirSync(program.data_root);
const animals = files.map(function(file) {
  return {
    profile: path.join(program.data_root, file),
    animal: path.basename(file, path.extname(file))
  };
});

const names =
    ['Bella', 'Buddy', 'Gizmo', 'Coco', 'Dexter', 'Bandit', 'Romeo', 'Lexi'];

function randomInt(n) { return Math.floor(Math.random() * n); }

function admitAnimal() {
  var animal = animals[randomInt(animals.length)];
  var name = names[randomInt(names.length)];
  var options = {
    url: 'http://localhost:8080/upload/animal',
    headers: { 'admit-name': name, 'admit-animal': animal.animal }
  };
  req = request.post(options, function(err, resp, body) {
    if (err) {
      console.log(err);
    }
  });
  fs.createReadStream(animal.profile).pipe(req);
}

function queryAnimals() {
  request.get('http://localhost:8080/', function(err, resp, body) {
    if (err) {
      console.log(err);
    }
  });
}

for (i = 0; i < program.num_requests; i++) {
  if (randomInt(2)) {
    console.log('amit');
    admitAnimal();
  } else {
    console.log('query');
    queryAnimals();
  }
}

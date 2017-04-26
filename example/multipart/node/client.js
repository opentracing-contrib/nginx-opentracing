const restler = require('restler');
const program = require('commander');
const fs = require('fs');

program.option('f, --firstname <firstname>', 'First Name')
    .option('l, --lastname <lastname>', 'Last Name')
    .option('p, --profile <profile>', 'Profile')
    .parse(process.argv);

if (!program.firstname || !program.lastname || !program.profile) {
  console.error('missing options!');
  process.exit(1);
}

fs.stat(program.profile, function(err, stats) {
  if (err) {
    console.log(err);
    process.exit(1);
  }
  restler
      .post('http://localhost:8080/upload/profile', {
        multipart: true,
        username: 'abc',
        password: '123',
        data: {
          firstname: program.firstname,
          lastname: program.lastname,
          profilepic:
              restler.file(program.profile, null, stats.size, null, 'image/jpg')
        }
      })
      .on('error',
          function(err) {
            console.log('Failed to upload profile: ' + String(err));
            process.exit(1);
          })
      .on('success', function(data) { process.exit(0); });
});

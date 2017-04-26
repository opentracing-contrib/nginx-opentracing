const express = require('express');
const basicAuth = require('express-basic-auth');
const formidable = require('express-formidable');
const path = require('path');
const fs = require('fs');

var app = express();
app.use('/upload/profile', formidable());
app.use('/auth', basicAuth({
          users: { 'abc': '123' },
          challenge: true,
          realm: 'multipart-example'
        }));
app.set('view engine', 'pug');
app.set('views', path.join(__dirname, '/views'));

app.get('/auth', (req, res) => {
  console.log('auth');
  res.status(200).send();
});

app.post('/upload/profile', (req, res) => {
  console.log('upload');
  console.log(req.fields);
  fs.readFile(req.fields.profilepic_path, (err, data) => {
    if (err) {
      console.log(err);
      res.status(500).send('Failed to read file!');
      return;
    }
    imgEncoding = new Buffer(data).toString('base64');
    imgSrc =
        'data:' + req.fields.profilepic_content_type + ';base64,' + imgEncoding;
    res.render('profile', {
      firstname: req.fields.firstname,
      lastname: req.fields.lastname,
      profile_pic: imgSrc
    });
  });
});

app.listen(3000, function() { console.log('Listening on 3000'); });

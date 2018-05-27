from flask import Flask, request
app = Flask(__name__)

@app.route('/has-span-context')
def hello_world():
    assert 'x-ot-span-context' in request.headers
    return 'Flask Dockerized'

if __name__ == '__main__':
    app.run(debug=True,host='0.0.0.0')

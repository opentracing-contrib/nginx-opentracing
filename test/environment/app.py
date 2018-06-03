from flask import Flask, request
from flask.ext.api import status
app = Flask(__name__)

@app.route('/has-span-context')
def has_span_context():
    assert 'x-ot-span-context' in request.headers
    return 'Flask Dockerized'

@app.route('/has-span-context-redirect')
def has_span_context_redirect():
    assert 'x-ot-span-context' in request.headers
    return '', status.HTTP_301_MOVED_PERMANENTLY

if __name__ == '__main__':
    app.run(debug=True,host='0.0.0.0')

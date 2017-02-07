from flask import Flask, render_template, request
import json
import sys
import pyidam

pyidam.Client.server = 'localhost'
pyidam.Client.port = 56565

app = Flask(__name__)


@app.route("/", methods=['GET'])
def get():
    client = pyidam.Client()
    str = client.get('help::services()', '')
    return render_template('index.html', title="UDA server", text=str.str)


@app.route("/", methods=['POST'])
def post():
    try:
        signal = request.form.get('signal') or ''
        source = request.form.get('source') or ''
        server = request.form.get('server') or 'localhost'
        port = request.form.get('port') or '56565'
        pyidam.Client.server = server
        pyidam.Client.port = int(port)
        client = pyidam.Client()
        result = client.get(signal, source)
        return result.jsonify()
    except Exception as ex:
        return json.dumps(str(ex))

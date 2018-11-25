#!/usr/bin/env python

from datetime import datetime, timedelta
import json
import sqlite3

# Install flask with `apt install python-flask` or `pip install Flask`
from flask import abort, Flask, g, jsonify, render_template, request
from functools import wraps

app = Flask(__name__)
DATABASE = 'data/data.sqlite3'

def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
    return db

def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    if one:
        rv = cur.fetchone()
    else:
        rv = cur.fetchall()
    cur.close()
    return rv

# Requiring an key decorator
def require_appkey(view_function):
    @wraps(view_function)
    def decorated_function(*args, **kwargs):
        key = request.args.get('key')
        if key and query_db('SELECT id FROM apikeys WHERE key = ?', [key], True):
            return view_function(*args, **kwargs)
        else:
            abort(401)
    return decorated_function

@app.context_processor
def utility_processor():
    def temp_c_to_f(tempc):
        return float(tempc) * 1.8 + 32
    return dict(temp_c_to_f=temp_c_to_f)

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

@app.route('/')
def hello_world():
    return 'Hello, World!'

@app.route('/data')
def data_route():
    table = query_db('select timestamp, location, humidity, temp_c from temps')
    table.reverse()
    timestamps = [i[0] for i in table]
    humidities = [i[2] for i in table]
    ctemps = [i[3] for i in table]
    return render_template('data.html', table=table, humidities=json.dumps(humidities), timestamps=json.dumps(timestamps), ctemps=json.dumps(ctemps))

@app.route('/temps', methods=['GET'])
@require_appkey
def temps_route():
    hours_ago = request.args.get('hoursago')
    if hours_ago is not None:
        date = datetime.now() - timedelta(hours=int(hours_ago))
        temps_array = query_db('SELECT timestamp, location, humidity, temp_c FROM temps WHERE timestamp > ?',
                               [date])
    else:
        temps_array = query_db('select timestamp, location, humidity, temp_c from temps')

    temp_dicts = []
    for temp_array in temps_array:
        keys = ['timestamp', 'location', 'humidity', 'tempC']
        d = dict(zip(keys, temp_array))
        temp_dicts.append(d)

    return jsonify(temps=temp_dicts)

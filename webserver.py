#!/usr/bin/env python

import json
import sqlite3

# Install flask with `apt install python-flask` or `pip install Flask`
from flask import Flask
from flask import g
from flask import render_template

app = Flask(__name__)
DATABASE = 'data/data.sqlite3'

def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
    return db

def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv

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
    timestamps = [i[0] for i in table]
    humidities = [i[2] for i in table]
    ctemps = [i[3] for i in table]
    return render_template('data.html', table=table, humidities=json.dumps(humidities), timestamps=json.dumps(timestamps), ctemps=json.dumps(ctemps))
    # for temp_row in query_db('select location, temp_c from temps'):
        # print temp_row
        # print temp_row[0], ': ', temp_row[1], 'degrees C, '

#!/usr/bin/env python

# Install flask with `apt install python-flask` or `pip install Flask`
from flask import Flask
app = Flask(__name__)

import sqlite3

DATABASE = 'data/data.sqlite3'

def get_db():
    db = getattr(flask._app_ctx_stack.top , '_database', None)
    if db is None:
        db = flask._app_ctx_stack.top ._database = sqlite3.connect(DATABASE)
    return db

def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(flask._app_ctx_stack.top , '_database', None)
    if db is not None:
        db.close()

@app.route('/')
def hello_world():
    return 'Hello, World!'

@app.route('/data')
def data_route():
    for temp_row in query_db('select * from temps'):
        print temp_row['location'], ': ', temp_row['temp_c'], 'degrees C, ', temp_row['temp_f'], 'degrees F'

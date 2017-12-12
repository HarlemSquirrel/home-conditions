#!/usr/bin/env python

# Install flask with `apt install python-flask` or `pip install Flask`
from flask import Flask
app = Flask(__name__)

@app.route('/')
def hello_world():
    return 'Hello, World!'

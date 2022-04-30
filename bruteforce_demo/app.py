# Cleaned up a lot from https://pythonspot.com/login-authentication-with-flask/
from flask import Flask, flash, render_template, request, session
import os

app = Flask(__name__)


@app.route('/')
def home():
    if not session.get('logged_in'):
        return render_template('login.html')
    else:
        return "Hello Boss! <a href=\"/logout\">Logout</a>"


@app.route('/login', methods=['POST'])
def do_admin_login():
    if request.form['username'] == 'administrator' and request.form['password'] == 'racing':
        session['logged_in'] = True
        return home(), 200
    else:
        flash('wrong password!')
        return home(), 401



@app.route("/logout")
def logout():
    session['logged_in'] = False
    return home()


if __name__ == "__main__":
    app.secret_key = os.urandom(12)
    app.run(debug=True, host='0.0.0.0', port=4000)

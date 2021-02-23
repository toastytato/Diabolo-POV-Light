from flask import *

app = Flask(__name__)

package = "Hello"
state = False

correction = 1.0


@app.route('/')
def home_page():
    return render_template('main_page.html')


@app.route('/button')
def button():
    print("Clicked")

    global package
    global state

    if state:
        package = "on"
    else:
        package = "off"

    state = not state
    return redirect('/')


@app.route('/success/<name>')
def success(name):
    return 'Welcome %s' % name


@app.route('/submit', methods=['POST', 'GET'])
def login():
    if request.method == 'POST':
        value = request.form['name1']
        return redirect('/')
    elif request.method == 'GET':
        value = request.args.get('correction')
        global correction
        correction = value
        # print(correction.type())
        return redirect('/')

@app.route('/esp_post', methods=['POST'])
def postHandler():
    if request.method == 'POST':
        print(float(request.data))

    return str(correction)


if __name__ == "__main__":
    app.run(host='0.0.0.0', port='8266')

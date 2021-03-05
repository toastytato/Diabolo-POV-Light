from flask import *
from celery import Celery
import threading
import socket, select, queue
import numpy as np
from sys import getsizeof


def run_udp(threadname):
    while True:
        # Receive BUFFER_SIZE bytes data
        # data is a list with 2 elements
        # first is data
        # second is client address
        data = s.recvfrom(BUFFER_SIZE)
        if data:
            # print received data
            # print("Client to Server: ", data)
            # Convert to upper case and send back to Client
            bitstream = package_display_data(display_matrix)
            # print(bitstream)

            s.sendto(bitstream, data[1])


def package_display_data(data):
    update_packet = np.array(
        [
            0,
            curr_sector,
            data.shape[0],
            data.shape[1],
            data.shape[2],
        ],
        dtype=np.uint8,
    )
    # print(display_matrix.shape, display_matrix.flatten(), update_packet)
    update_packet = np.concatenate((update_packet, display_matrix.flatten())).astype(
        "uint8"
    )
    return bytes(update_packet)


app = Flask(__name__)

# bind all IP
UDP_IP = "0.0.0.0"
# Listen on Port
UDP_PORT = 8266
# Size of receive buffer
BUFFER_SIZE = 1024
# Create a TCP/IP socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Bind the socket to the UDP_IP and port
s.bind((UDP_IP, UDP_PORT))

correction = 1.0
curr_sector = 0
display_matrix = np.ones((1, 1, 1))

udp_thread = threading.Thread(target=run_udp, args=("UDP Thread",))
udp_thread.start()


@app.route("/")
def home_page():
    return render_template("main_page.html")


@app.route("/send_img_btn")
def button():
    global curr_sector
    print(display_matrix.shape[0])
    curr_sector = (curr_sector + 1) % display_matrix.shape[0]
    return redirect("/")


@app.route("/array", methods=["POST"])
def process_array():
    data = request.form.getlist("canvas_data")[0]
    # print(data)
    array = np.array(json.loads(data), dtype=np.uint8)
    global display_matrix
    display_matrix = array
    print(array)
    return "data"


@app.route("/submit", methods=["POST", "GET"])
def login():
    if request.method == "POST":
        value = request.form["name1"]
        return redirect("/")
    elif request.method == "GET":
        value = request.args.get("correction")
        global correction
        correction = value
        # print(correction.type())
        return redirect("/")


# receives data from ESP then transmits data from server
@app.route("/esp_request", methods=["POST"])
def postHandler():
    if request.method == "POST":
        print(float(request.data))

    return str(correction)


if __name__ == "__main__":
    # app.run(debug=True)
    app.run(host="0.0.0.0", port="8266")

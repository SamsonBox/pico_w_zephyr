from flask import Flask, jsonify, send_from_directory, request
import time
import random

app = Flask(__name__)
start_time = time.time()

# Gerätezustand (simuliert)
state = {
    "relay": False,
    "start_time": "08:00",
    "end_time": "17:00",
    "switch_temp": "15.0",
}

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

@app.route('/main.js')
def script():
    return send_from_directory('.', 'main.js')

@app.route('/data')
def data():
    uptime_ms = int((time.time() - start_time) * 1000)
    temp = round(random.uniform(20.0, 30.0), 2)
    print(f'data: {state}')
    return jsonify({
        'uptime': uptime_ms,
        'temp': temp,
        **state
    })

@app.route('/update', methods=['POST'])
def update_state():
    try:
        updates = request.get_json()

        # einfache Validierung serverseitig
        if "switch_temp" in updates:
            temp = float(updates["switch_temp"])
            if not (0 <= temp <= 100):
                return jsonify({"error": "Schaltetemperatur ungültig"}), 400

        for key, value in updates.items():
            if key in state:
                state[key] = value

        uptime_ms = int((time.time() - start_time) * 1000)
        temp = round(random.uniform(20.0, 30.0), 2)
        print(f'update: {state}')
        return jsonify({
            'uptime': uptime_ms,
            'temp': temp,
            **state
        })
    except Exception as e:
        return jsonify({"error": str(e)}), 400

if __name__ == '__main__':
    app.run(debug=True)

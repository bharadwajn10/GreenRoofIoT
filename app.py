import sqlite3
from flask import Flask, request, jsonify, g
from flask_cors import CORS
from datetime import datetime, timezone # FIX 1: IMPORT the necessary datetime modules

# --- Application Setup ---
app = Flask(__name__)
CORS(app) 
DATABASE = 'sensor_data.db'

# --- Database Connection Management ---
def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
        db.row_factory = sqlite3.Row
    return db

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

def init_db():
    with app.app_context():
        db = get_db()
        cursor = db.cursor()
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS sensor_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                temperature REAL,
                humidity REAL,
                pressure REAL,
                soilMoisture REAL,
                ph REAL,
                nutrientIndex REAL,
                -- FIX 2: REMOVE 'DEFAULT CURRENT_TIMESTAMP'. The app will provide the timestamp.
                recorded_at TEXT NOT NULL 
            )
        ''')
        db.commit()

# --- API Endpoints ---
@app.route('/insert_data', methods=['POST'])
def insert_data():
    if not request.is_json:
        return jsonify({"success": False, "error": "Invalid data format (Expected JSON)"}), 400

    data = request.get_json()
    
    temperature = data.get('temperature')
    humidity = data.get('humidity')
    pressure = data.get('pressure')
    soilMoisture = data.get('soilMoisture')
    ph = data.get('ph')
    nutrientIndex = data.get('nutrientIndex')

    if None in [temperature, humidity, pressure, soilMoisture, ph, nutrientIndex]:
        return jsonify({"success": False, "error": "One or more sensor values are missing"}), 400

    try:
        db = get_db()
        cursor = db.cursor()
        
        # FIX 3: GENERATE a timezone-aware UTC timestamp in ISO 8601 format
        now_utc = datetime.now(timezone.utc)
        timestamp_for_db = now_utc.strftime('%Y-%m-%dT%H:%M:%SZ')

        # FIX 4: ADD the 'recorded_at' column and the new timestamp to the SQL query
        cursor.execute(
            "INSERT INTO sensor_data (temperature, humidity, pressure, soilMoisture, ph, nutrientIndex, recorded_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
            (temperature, humidity, pressure, soilMoisture, ph, nutrientIndex, timestamp_for_db)
        )
        db.commit()
        return jsonify({"success": True, "message": "Data inserted successfully"}), 200
    except Exception as e:
        db.rollback() 
        return jsonify({"success": False, "error": str(e)}), 500

@app.route('/get_data', methods=['GET'])
def get_data():
    try:
        db = get_db()
        cursor = db.cursor()
        
        # This query remains the same and will work perfectly
        cursor.execute("SELECT * FROM sensor_data ORDER BY id DESC")
        rows = cursor.fetchall()
        
        data_list = [dict(row) for row in rows]

        return jsonify({"success": True, "data": data_list}), 200
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

if __name__ == '__main__':
    init_db()
    print("Database Initialized. Running Flask Server...")
    app.run(host='0.0.0.0', port=5000)
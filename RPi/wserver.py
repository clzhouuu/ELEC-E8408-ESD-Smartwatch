from flask import Flask
from flask import render_template
from flask import jsonify
from flask import Response
from flask import request
import outbound_package
from datetime import date, timedelta, datetime

import db
import hike
from receiver import hubbt

app = Flask(__name__)
hdb = db.HubDatabase()

metrics = hdb.get_metrics()
if metrics:
    hike.USER_WEIGHT = metrics[0]
    hike.USER_HEIGHT = metrics[1] 

@app.route('/')
def get_home():
    sessions = hdb.get_sessions() 
    bt = hubbt.connected
    return render_template('home.html', sessions=sessions)

@app.route('/session-page')
@app.route('/session-page/<id>')
def get_session(id='latest'):
    sessions = hdb.get_sessions()
    if id == 'latest':
        session = sessions[-1] if sessions else None
    else:
        session = hdb.get_session(int(id))
    return render_template('session.html', session=session)

@app.route('/history-page')
def get_history():
    sessions = hdb.get_sessions() 
    if sessions:
        start_of_week = date.today() - timedelta(days=date.today().weekday())
        weekly_sessions = [s for s in sessions if (datetime.strptime(s.date, "%Y-%m-%d").date()) >= start_of_week]
        total = {
            'km': round(sum(s.km for s in weekly_sessions), 1),
            'steps': sum(s.steps for s in weekly_sessions),
            'kcal': int(sum(s.kcal for s in weekly_sessions))
        } if weekly_sessions else {'km': 0, 'steps': 0, 'kcal': 0}
        best = max(sessions, key=lambda s: s.km)
        avg = {
            'km': round(sum(s.km for s in sessions) / len(sessions), 1),
            'steps': int(sum(s.steps for s in sessions) / len(sessions)),
            'kcal': int(sum(s.kcal for s in sessions) / len(sessions))
        }
    return render_template('history.html', sessions=sessions, total=total, best=best, avg=avg)
@app.route('/sessions')
def get_sessions():
    sessions = hdb.get_sessions() 
    sessions = list(map(lambda s: hike.to_list(s), sessions))
    print(sessions)
    return jsonify(sessions)

@app.route('/sessions/<id>')
def get_session_by_id(id):
    session = hdb.get_session(id)
    return jsonify(hike.to_list(session))

@app.route('/sessions/<id>/delete')
def delete_session(id):
    hdb.delete(id)
    print(f'DELETED SESSION WITH ID: {id}')
    return Response(status=202)

@app.route('/settings')
def get_settings():
    return jsonify({
        "height": hike.USER_HEIGHT,
        "weight": hike.USER_WEIGHT
    })

@app.route('/save-settings', methods=['POST'])
def save_settings():
    data = request.get_json()
    height = int(data['height'])
    weight = int(data['weight'])

    hike.USER_HEIGHT = height
    hike.USER_WEIGHT = weight

    hdb.save_metrics(weight, height)    
    hubbt.send_package(outbound_package.load_metrics(hdb))

    return jsonify({"height": height, "weight": weight})
    
@app.route('/bt-status')
def bt_status():
    return jsonify({"connected": hubbt.connected})

if __name__ == "__main__":
    app.run('0.0.0.0', debug=True)

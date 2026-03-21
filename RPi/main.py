import threading
import receiver
import wserver

thread = threading.Thread(target=receiver.main, daemon=True)
thread.start()
wserver.app.run('0.0.0.0', debug=True)

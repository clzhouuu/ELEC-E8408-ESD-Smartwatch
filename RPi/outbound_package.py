import time

class SendPackage:
    weight = 0
    height = 0

def to_list(s: SendPackage) -> list:
    return [s.weight, s.height]

def from_list(l: list) -> SendPackage:
    s = SendPackage()
    s.weight = l[0]
    s.height = l[1]
    return s

def load_metrics(db) -> SendPackage:
    s = SendPackage()
    metrics = db.get_metrics() 
    s.weight = metrics[0]
    s.height = metrics[1]
    return s

def send_config(bt_serial, db):
    s = load_metrics(db)
    msg = f"CONFIG,{s.weight},{s.height}\n"
    bt_serial.write(msg.encode())
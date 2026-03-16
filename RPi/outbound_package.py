import time

class SendPackage:
    RTC = 0
    weight = 0
    height = 0

def to_list(s: SendPackage) -> list:
    return [s.RTC, s.weight, s.height]

def from_list(l: list) -> SendPackage:
    s = SendPackage()
    s.RTC = l[0]
    s.weight = l[1]
    s.height = l[2]
    return s

def load_metrics(db) -> SendPackage:
    s = SendPackage()
    s.RTC = int(time.mktime(time.localtime()))
    metrics = db.get_metrics() 
    s.weight = metrics[0]
    s.height = metrics[1]
    return s
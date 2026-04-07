from db import HubDatabase
import hike

def load_coords(path="coords_with_alt.txt"):
    coords = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            lat, lon = line.split(",")
            coords.append((float(lat), float(lon)))
    return coords

coords = load_coords("coords_with_alt.txt")

# Adjust these values if you want
km = 11.53
steps = 15840
kcal = 684
duration = "02:22:09" # seconds = 2h 22m
start_time = "10:18:36"   # HHMMSS
date = "26-03-2026"   # YYYYMMDD

# IMPORTANT:
# Change this constructor line if your hike.py uses a different argument order/name.
session = hike.HikeSession()

session.id = 0
session.km = km
session.steps = steps
session.kcal = kcal
session.duration = duration
session.start_time = start_time
session.date = date
session.coords = coords

db = HubDatabase()
db.save(session)

print("Dummy session saved successfully.")
print(f"Session summary: {km} km, {steps} steps, {kcal} kcal, {len(coords)} GPS points")

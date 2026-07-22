"""Vision system - zorne kuzele + LOS raycasting s vyskovou mapou."""
import math
from .settings import VISION_RANGE, PRIMARY_CONE, PERIPHERAL_CONE


def line_of_sight(tilemap, ox, oy, tx, ty, eye_height=1):
    """Bresenham medzi dvoma dlazdicami; blokuje dlazdica vyssia ako oko."""
    x0, y0, x1, y1 = ox, oy, tx, ty
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx - dy
    while True:
        if (x0, y0) != (ox, oy) and (x0, y0) != (tx, ty):
            if tilemap.height(x0, y0) > eye_height:
                return False
        if (x0, y0) == (x1, y1):
            return True
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x0 += sx
        if e2 < dx:
            err += dx
            y0 += sy


def angle_diff(a, b):
    d = math.degrees(a - b)
    while d > 180:
        d -= 360
    while d < -180:
        d += 360
    return abs(d)


def detect(tilemap, gx, gy, facing, tx, ty, crouch):
    """Vrati ('primary'|'peripheral'|None) podla zorneho pola straze."""
    dx = tx - gx
    dy = ty - gy
    dist = math.hypot(dx, dy)
    if dist > VISION_RANGE or dist < 1e-6:
        return None
    target_angle = math.atan2(dy, dx)
    diff = angle_diff(target_angle, facing)
    if diff > PERIPHERAL_CONE:
        return None
    if not line_of_sight(tilemap, int(round(gx)), int(round(gy)),
                         int(round(tx)), int(round(ty))):
        return None
    if diff <= PRIMARY_CONE:
        return "primary"     # cerveny kuzel - vidi aj krciaceho sa
    # perifery: krciaci sa hrac je neviditelny
    if crouch:
        return None
    return "peripheral"

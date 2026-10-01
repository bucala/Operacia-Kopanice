"""Konecny stavovy automat nepriatela (Patrol / Suspicious / Alert)."""
import math
import random

from .vision import detect, line_of_sight
from .pathfinding import astar
from .settings import (
    ALERT_RATE, ALERT_DECAY, SUSPICIOUS_TIMEOUT, ALERT_LOST_TIMEOUT,
    GUARD_SHOOT_RANGE, GUARD_SHOOT_DMG, GUARD_SHOOT_CD, NOISE_SHOT, VISION_RANGE,
)


def _repath(world, guard, goal):
    start = (guard.move.tx, guard.move.ty)
    if start == goal:
        guard.move.path = []
        return
    path = astar(world.tilemap, start, goal)
    if path:
        guard.move.set_path(path)


def _perceive(world, guard):
    """Vrati slovnik s najlepsim podnetom pre straz."""
    gx, gy = guard.move.fx, guard.move.fy
    facing = guard.move.angle
    best = {"kind": None, "unit": None, "pos": None}

    # videnie hracov (zamaskovaneho spiona straze ignoruju)
    for u in world.alive_units():
        if u.disguised:
            continue
        d = detect(world.tilemap, gx, gy, facing, u.move.fx, u.move.fy, u.crouch)
        if d == "primary":
            return {"kind": "primary", "unit": u, "pos": (u.move.tx, u.move.ty)}
        if d == "peripheral" and best["kind"] is None:
            best = {"kind": "peripheral", "unit": u, "pos": (u.move.tx, u.move.ty)}

    # mrtve telo v dohlade -> Alert
    for (bx, by) in world.bodies:
        dist = math.hypot(bx - gx, by - gy)
        if dist <= VISION_RANGE and line_of_sight(
            world.tilemap, int(round(gx)), int(round(gy)), bx, by
        ):
            ang = math.degrees(math.atan2(by - gy, bx - gx))
            diff = abs(((ang - math.degrees(facing) + 180) % 360) - 180)
            if diff <= 45:
                return {"kind": "body", "unit": None, "pos": (bx, by)}

    # zvukove podnety
    for n in world.noises:
        dist = math.hypot(n.x - gx, n.y - gy)
        if dist <= n.radius:
            if best["kind"] is None:
                best = {"kind": "noise", "unit": None, "pos": (n.x, n.y)}
    return best


def update(world, dt):
    for g in world.guards:
        if not g.alive:
            continue
        _update_guard(world, g, dt)


def _update_guard(world, g, dt):
    p = _perceive(world, g)
    kind = p["kind"]

    # --- akumulacia podozrenia ---
    stimulus = False
    if kind == "primary":
        g.alert = 100.0
        g.last_known = p["pos"]
        g.target_unit = p["unit"]
        stimulus = True
    elif kind == "body":
        g.alert = 100.0
        g.last_known = p["pos"]
        stimulus = True
    elif kind == "peripheral":
        g.alert = min(100.0, g.alert + ALERT_RATE * dt)
        g.last_known = p["pos"]
        g.target_unit = p["unit"]
        stimulus = True
    elif kind == "noise":
        g.alert = max(g.alert, 55.0)
        g.last_known = p["pos"]
        stimulus = True
    else:
        g.alert = max(0.0, g.alert - ALERT_DECAY * dt)

    # --- prechody stavov ---
    if g.state == "PATROL":
        if g.alert >= 100.0:
            _enter_alert(g)
        elif g.alert > 0.0:
            g.state = "SUSPICIOUS"
            g.suspicious_timer = 0.0
        _patrol(world, g)

    elif g.state == "SUSPICIOUS":
        if g.alert >= 100.0:
            _enter_alert(g)
        else:
            if stimulus:
                g.suspicious_timer = 0.0
            else:
                g.suspicious_timer += dt
            if g.suspicious_timer >= SUSPICIOUS_TIMEOUT and g.alert <= 0.0:
                g.state = "PATROL"
                g.last_known = None
            _investigate(world, g, speed_mod=0.7)

    elif g.state == "ALERT":
        if stimulus and kind in ("primary", "peripheral"):
            g.alert_lost_timer = 0.0
        else:
            g.alert_lost_timer += dt
        if g.alert_lost_timer >= ALERT_LOST_TIMEOUT:
            g.state = "SUSPICIOUS"
            g.suspicious_timer = 0.0
            g.target_unit = None
        _combat(world, g, dt)
        _investigate(world, g, speed_mod=1.9)

    # pohyb
    sm = 1.9 if g.state == "ALERT" else (0.7 if g.state == "SUSPICIOUS" else 1.0)
    g.move.update(dt, world.tilemap, speed_mod=sm)


def _enter_alert(g):
    if g.state != "ALERT":
        g.state = "ALERT"
        g.alert_lost_timer = 0.0


def _patrol(world, g):
    if not g.waypoints:
        return
    if not g.move.path:
        target = g.waypoints[g.wp_idx]
        if (g.move.tx, g.move.ty) == target:
            g.wp_idx = (g.wp_idx + 1) % len(g.waypoints)
            target = g.waypoints[g.wp_idx]
        _repath(world, g, target)


def _investigate(world, g, speed_mod=1.0):
    if g.last_known is None:
        return
    if (g.move.tx, g.move.ty) == g.last_known and not g.move.path:
        # prehladaj okolie nahodne
        gx, gy = g.move.tx, g.move.ty
        for _ in range(8):
            nx = gx + random.randint(-3, 3)
            ny = gy + random.randint(-3, 3)
            if world.tilemap.walkable(nx, ny):
                _repath(world, g, (nx, ny))
                break
    elif not g.move.path:
        _repath(world, g, g.last_known)


def _combat(world, g, dt):
    g.shoot_cd = max(0.0, g.shoot_cd - dt)
    target = None
    best_d = 1e9
    for u in world.alive_units():
        if u.disguised:
            continue
        d = math.hypot(u.move.fx - g.move.fx, u.move.fy - g.move.fy)
        if d <= GUARD_SHOOT_RANGE and line_of_sight(
            world.tilemap, g.move.tx, g.move.ty, u.move.tx, u.move.ty
        ):
            if d < best_d:
                best_d = d
                target = u
    if target and g.shoot_cd <= 0.0:
        target.hp -= GUARD_SHOOT_DMG
        g.shoot_cd = GUARD_SHOOT_CD
        world.add_noise(g.move.tx, g.move.ty, NOISE_SHOT)
        if target.hp <= 0:
            target.hp = 0
            target.alive = False

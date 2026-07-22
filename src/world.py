"""Entity, komponenty a herny svet (light ECS)."""
import math
from .settings import (
    TERRAIN, PLAYER_HP, FOOTPRINT_MUD_TTL, FOOTPRINT_SNOW_TTL,
)


def tile_of(f):
    return int(round(f))


class Movement:
    """Pohybovy komponent: pohyb po dlazdicach pozdlz cesty."""
    def __init__(self, fx, fy, base_speed):
        self.fx = float(fx)
        self.fy = float(fy)
        self.base_speed = base_speed
        self.path = []          # zoznam dlaznic (x,y)
        self.angle = 0.0        # smer pohladu (rad), +x = 0
        self.moving = False

    @property
    def tx(self):
        return tile_of(self.fx)

    @property
    def ty(self):
        return tile_of(self.fy)

    def set_path(self, path):
        # zahod prvy uzol ak je to aktualna dlazdica
        if path and path[0] == (self.tx, self.ty):
            path = path[1:]
        self.path = list(path)
        self.moving = bool(self.path)

    def update(self, dt, tilemap, speed_mod=1.0):
        if not self.path:
            self.moving = False
            return None
        tx, ty = self.path[0]
        terr = tilemap.info(self.tx, self.ty)
        speed = self.base_speed * terr["speed"] * speed_mod
        dx = tx - self.fx
        dy = ty - self.fy
        dist = math.hypot(dx, dy)
        if dist > 1e-4:
            self.angle = math.atan2(dy, dx)
        step = speed * dt
        stepped_tile = None
        if dist <= step or dist < 1e-4:
            prev_tile = (self.tx, self.ty)
            self.fx, self.fy = float(tx), float(ty)
            self.path.pop(0)
            if (tx, ty) != prev_tile:
                stepped_tile = (tx, ty)
        else:
            self.fx += dx / dist * step
            self.fy += dy / dist * step
        self.moving = bool(self.path)
        return stepped_tile  # dlazdica, na ktoru sa prave vstupilo (pre stopy)


class Unit:
    """Hratelna postava."""
    def __init__(self, uid, name, role, fx, fy, base_speed):
        self.id = uid
        self.name = name
        self.role = role           # "leader" | "sapper"
        self.move = Movement(fx, fy, base_speed)
        self.hp = PLAYER_HP
        self.alive = True
        self.crouch = False
        self.tnt = 0               # nesene naloze (sapper)
        self.has_uniform = False   # nemecka dostojnicka uniforma (spy)
        self.disguised = False     # aktivne maskovanie (spy)
        self.ammo = 0              # naboje odstrelovaca
        self.selected = False

    @property
    def speed_mod(self):
        return 0.55 if self.crouch else 1.0


class Guard:
    """Nepriatel so stavovym automatom."""
    def __init__(self, gid, fx, fy, waypoints, facing=0.0):
        self.id = gid
        self.move = Movement(fx, fy, 1.8)
        self.move.angle = facing
        self.waypoints = waypoints      # zoznam (x,y)
        self.wp_idx = 0
        self.state = "PATROL"           # PATROL | SUSPICIOUS | ALERT
        self.alert = 0.0                # 0..100
        self.alive = True
        self.last_known = None          # (x,y) posledna znama pozicia hraca
        self.suspicious_timer = 0.0
        self.alert_lost_timer = 0.0
        self.shoot_cd = 0.0
        self.target_unit = None
        self.search_target = None


class Footprint:
    def __init__(self, x, y, kind):
        self.x = x
        self.y = y
        self.kind = kind
        self.ttl = FOOTPRINT_SNOW_TTL if kind == "snow" else FOOTPRINT_MUD_TTL
        self.age = 0.0


class Noise:
    """Akusticky podnet (transientny, spracuje ho AI v jednom snimku)."""
    def __init__(self, x, y, radius):
        self.x = x
        self.y = y
        self.radius = radius


class World:
    def __init__(self, tilemap):
        self.tilemap = tilemap
        self.units = []
        self.guards = []
        self.bodies = []        # (x,y) mrtve/omracene telá
        self.footprints = []
        self.noises = []        # zoznam Noise pre aktualny snimok

    def add_noise(self, x, y, radius):
        self.noises.append(Noise(x, y, radius))

    def alive_units(self):
        return [u for u in self.units if u.alive]

    def blocked_tiles(self, exclude=None):
        s = set()
        for u in self.units:
            if u.alive and u is not exclude:
                s.add((u.move.tx, u.move.ty))
        for g in self.guards:
            if g.alive and g is not exclude:
                s.add((g.move.tx, g.move.ty))
        return s

    def spawn_footprint(self, x, y):
        terr = self.tilemap.info(x, y)
        kind = terr["foot"]
        if kind:
            self.footprints.append(Footprint(x, y, kind))

    def update_footprints(self, dt):
        for fp in self.footprints:
            fp.age += dt
        self.footprints = [fp for fp in self.footprints if fp.age < fp.ttl]

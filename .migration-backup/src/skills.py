"""Systom schopnosti (Skill hooks) - validate/execute podla ISkillHook."""
import math
import random

from .pathfinding import astar
from .vision import line_of_sight
from .settings import NOISE_SHOT, FALSE_ORDER_RANGE, SNIPE_RANGE, SNIPE_NOISE


def _adjacent(ax, ay, bx, by, include_self=True):
    dx, dy = abs(ax - bx), abs(ay - by)
    if dx == 0 and dy == 0:
        return include_self
    return dx <= 1 and dy <= 1


class Takedown:
    """Vodca: ticha eliminacia priliahleho nepriatela."""
    key = "Takedown (Vodca)"

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "leader":
            return False, "Iba Vodca dokaze ticho zlikvidovat straz."
        g = Takedown._target(world, unit)
        if not g:
            return False, "Ziadna straz v dosahu."
        if g.state == "ALERT":
            return False, "Straz v poplachu sa neda ticho zneskodnit."
        return True, ""

    @staticmethod
    def _target(world, unit):
        for g in world.guards:
            if g.alive and _adjacent(unit.move.tx, unit.move.ty, g.move.tx, g.move.ty,
                                     include_self=False):
                return g
        return None

    @staticmethod
    def execute(world, mission, unit):
        g = Takedown._target(world, unit)
        if not g:
            return False, "Ziadna straz v dosahu."
        g.alive = False
        world.bodies.append((g.move.tx, g.move.ty))
        return True, "Straz ticho zneskodnena."


class PickTnt:
    """Zenista: ukradni debny s TNT zo skladu."""
    key = "Vziat TNT (Zenista)"

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "sapper":
            return False, "Iba Zenista manipuluje s vybusninami."
        if mission.tnt_available <= 0:
            return False, "Sklad TNT je prazdny."
        if not _adjacent(unit.move.tx, unit.move.ty, *mission.tnt_crate):
            return False, "Musis stat pri sklade TNT."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        take = mission.tnt_available
        unit.tnt += take
        mission.tnt_available = 0
        return True, f"Ukradnutych {take}x TNT."


class PlantTnt:
    """Zenista: poloz naloz na nosny pilier viaduktu."""
    key = "Polozit naloz (Zenista)"

    @staticmethod
    def _target(mission, unit):
        for pil in mission.pillars:
            if not pil["planted"] and _adjacent(unit.move.tx, unit.move.ty, *pil["pos"]):
                return pil
        return None

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "sapper":
            return False, "Iba Zenista vie zalozit naloz."
        if unit.tnt <= 0:
            return False, "Nemas ziadne TNT."
        if not PlantTnt._target(mission, unit):
            return False, "Stoj pri neoznacenom pilieri."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        pil = PlantTnt._target(mission, unit)
        if not pil:
            return False, "Ziadny pilier v dosahu."
        pil["planted"] = True
        unit.tnt -= 1
        return True, "Naloz zalozena a rozbuska prepojena."


class Disguise:
    """Spion: nasad / zloz nemecku dostojnicku uniformu."""
    key = "Maskovanie (Spion)"

    @staticmethod
    def _alert_zone(world, unit):
        for g in world.guards:
            if g.alive and g.state == "ALERT":
                d = math.hypot(g.move.fx - unit.move.fx, g.move.fy - unit.move.fy)
                if d <= 6.0:
                    return True
        return False

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "spy":
            return False, "Iba Spion vie pouzit maskovanie."
        if unit.disguised:
            return True, ""  # zlozenie uniformy je vzdy povolene
        if not unit.has_uniform:
            return False, "Nemas nemecku dostojnicku uniformu."
        if Disguise._alert_zone(world, unit):
            return False, "Nemozno sa maskovat v zone poplachu."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        unit.disguised = not unit.disguised
        if unit.disguised:
            return True, "Maskovanie aktivne - straze ta povazuju za svojho."
        return True, "Uniforma zlozena."


class FalseOrder:
    """Spion: zadaj falosny rozkaz - odlakaj najblizsiu straz."""
    key = "Falosny rozkaz (Spion)"

    @staticmethod
    def _target(world, unit):
        best, best_d = None, 1e9
        for g in world.guards:
            if not g.alive or g.state == "ALERT":
                continue
            d = math.hypot(g.move.fx - unit.move.fx, g.move.fy - unit.move.fy)
            if d <= FALSE_ORDER_RANGE and d < best_d:
                best, best_d = g, d
        return best

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "spy":
            return False, "Iba Spion zadava rozkazy."
        if not unit.disguised:
            return False, "Najprv sa zamaskuj do uniformy."
        if not FalseOrder._target(world, unit):
            return False, "Ziadna straz v dosahu rozkazu."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        g = FalseOrder._target(world, unit)
        if not g:
            return False, "Ziadna straz v dosahu."
        decoy = FalseOrder._decoy_tile(world, unit, g)
        if decoy:
            g.last_known = decoy
            g.state = "SUSPICIOUS"
            g.suspicious_timer = 0.0
            path = astar(world.tilemap, (g.move.tx, g.move.ty), decoy)
            if path:
                g.move.set_path(path)
        return True, "Straz odoslana na falosnu obhliadku."

    @staticmethod
    def _decoy_tile(world, unit, g):
        # tile daleko od hracov, do ktoreho straz posleme
        ux, uy = unit.move.tx, unit.move.ty
        cands = []
        for _ in range(40):
            nx = g.move.tx + random.randint(-7, 7)
            ny = g.move.ty + random.randint(-7, 7)
            if world.tilemap.walkable(nx, ny):
                dist_player = math.hypot(nx - ux, ny - uy)
                cands.append((dist_player, (nx, ny)))
        if not cands:
            return None
        cands.sort(reverse=True)
        return cands[0][1]


class Snipe:
    """Odstrelovac: eliminacia na velku vzdialenost (hlasny vystrel, malo nabojov)."""
    key = "Odstrel (R)"

    @staticmethod
    def _target(world, unit):
        best, best_d = None, 1e9
        for g in world.guards:
            if not g.alive:
                continue
            d = math.hypot(g.move.fx - unit.move.fx, g.move.fy - unit.move.fy)
            if d <= SNIPE_RANGE and d < best_d and line_of_sight(
                world.tilemap, unit.move.tx, unit.move.ty, g.move.tx, g.move.ty
            ):
                best, best_d = g, d
        return best

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "sniper":
            return False, "Iba Odstrelovac vie odstrelit ciel."
        if unit.ammo <= 0:
            return False, "Ziadne naboje."
        if not Snipe._target(world, unit):
            return False, "Ziadny ciel v dostrele a priamej viditelnosti."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        g = Snipe._target(world, unit)
        if not g:
            return False, "Ziadny ciel."
        g.alive = False
        world.bodies.append((g.move.tx, g.move.ty))
        unit.ammo -= 1
        world.add_noise(unit.move.tx, unit.move.ty, SNIPE_NOISE)
        return True, f"Ciel zlikvidovany. Naboje: {unit.ammo}"


class Climb:
    """Zved: preliezanie cez vertikalnu prekazku (stenu/zabradlie)."""
    key = "Preliezt (V)"

    DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1)]

    @staticmethod
    def _target(world, unit):
        x, y = unit.move.tx, unit.move.ty
        for dx, dy in Climb.DIRS:
            wall = (x + dx, y + dy)
            land = (x + 2 * dx, y + 2 * dy)
            if (world.tilemap.in_bounds(*wall)
                    and world.tilemap.height(*wall) > 0
                    and world.tilemap.walkable(*land)
                    and land not in world.blocked_tiles(exclude=unit)):
                return land
        return None

    @staticmethod
    def validate(world, mission, unit):
        if unit.role != "thief":
            return False, "Iba Zved dokaze preliezat prekazky."
        if not Climb._target(world, unit):
            return False, "Ziadna prekazka na preliezanie v okoli."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        land = Climb._target(world, unit)
        if not land:
            return False, "Nie je kam preliezt."
        unit.move.path = []
        unit.move.fx, unit.move.fy = float(land[0]), float(land[1])
        return True, "Zved preliezol prekazku."


class Detonate:
    """Odpalenie viaduktu - vsetky piliere musia byt zaminovane."""
    key = "Odpalit (X)"

    @staticmethod
    def validate(world, mission, unit):
        if not mission.pillars:
            return False, "Ziadne ciele."
        if not all(p["planted"] for p in mission.pillars):
            left = sum(1 for p in mission.pillars if not p["planted"])
            return False, f"Este chyba {left} naloz na pilieroch."
        return True, ""

    @staticmethod
    def execute(world, mission, unit):
        mission.detonated = True
        for p in mission.pillars:
            world.add_noise(p["pos"][0], p["pos"][1], NOISE_SHOT * 2)
        return True, "VIADUKT ZNICENY! Misia splnena."

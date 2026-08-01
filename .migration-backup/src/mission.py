"""Misia 1: Zelezna hrdelnica - mapa, ciele a vyhodnotenie."""
from .tilemap import TileMap
from .world import World, Unit, Guard


def _rect(grid, x0, y0, x1, y1, terr):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            grid[y][x] = terr


def _line(grid, x0, y0, x1, y1, terr):
    if y0 == y1:
        for x in range(min(x0, x1), max(x0, x1) + 1):
            grid[y0][x] = terr
    else:
        for y in range(min(y0, y1), max(y0, y1) + 1):
            grid[y][x0] = terr


def build_mission():
    W, H = 26, 20
    grid = [["grass" for _ in range(W)] for _ in range(H)]

    # hlavna cesta a prepojenia
    _line(grid, 1, 10, 24, 10, "road")
    _line(grid, 2, 10, 2, 17, "road")
    _line(grid, 24, 2, 24, 10, "road")
    _line(grid, 11, 5, 24, 5, "rail")     # zeleznicna trat na viadukte

    # jesenne blato (spomaluje, stopy)
    _rect(grid, 6, 12, 12, 15, "mud")
    _rect(grid, 15, 13, 19, 16, "mud")

    # staničná budova so skladom (steny + podlaha + dvere)
    _rect(grid, 4, 3, 9, 7, "wall")
    _rect(grid, 5, 4, 8, 6, "floor")
    grid[7][6] = "floor"                   # dvere na juh

    # nosne piliere viaduktu (neprejazdne, vysoke)
    pillar_pos = [(14, 5), (17, 5), (20, 5)]
    for (px, py) in pillar_pos:
        grid[py][px] = "wall"

    tilemap = TileMap(grid)
    world = World(tilemap)

    # hratelne postavy
    leader = Unit("leader", "Vodca", "leader", 2, 17, base_speed=3.0)
    sapper = Unit("sapper", "Zenista", "sapper", 3, 17, base_speed=2.8)
    spy = Unit("spy", "Spion", "spy", 2, 16, base_speed=3.0)
    spy.has_uniform = True
    sniper = Unit("sniper", "Odstrelovac", "sniper", 4, 17, base_speed=2.8)
    sniper.ammo = 5
    thief = Unit("thief", "Zved", "thief", 3, 16, base_speed=3.8)
    world.units = [leader, sapper, spy, sniper, thief]

    # straze a patroly
    world.guards = [
        Guard("g1", 11, 7, [(11, 7), (22, 7), (22, 8), (11, 8)]),
        Guard("g2", 10, 9, [(10, 9), (10, 4), (3, 9), (10, 9)]),
        Guard("g3", 6, 11, [(6, 11), (20, 11), (20, 12), (6, 12)]),
        Guard("g4", 7, 8, [(7, 8), (12, 8), (12, 6), (7, 8)]),
    ]

    mission = Mission(world)
    mission.tnt_crate = (6, 5)
    mission.tnt_available = 3
    mission.pillars = [{"pos": p, "planted": False} for p in pillar_pos]
    mission.exit = (24, 2)
    return world, mission


class Mission:
    def __init__(self, world):
        self.world = world
        self.tnt_crate = None
        self.tnt_available = 3
        self.pillars = []
        self.exit = None
        self.detonated = False

    @property
    def planted_count(self):
        return sum(1 for p in self.pillars if p["planted"])

    def status(self):
        if any(not u.alive for u in self.world.units):
            return "lose"
        if self.detonated:
            return "win"
        return "playing"

    def objectives(self):
        got_tnt = self.tnt_available == 0
        o = []
        o.append(("[x]" if got_tnt else "[ ]") + " Ukradni TNT zo skladu")
        o.append(f"[{self.planted_count}/{len(self.pillars)}] Zaloz naloze na piliere")
        o.append(("[x]" if self.detonated else "[ ]") + " Odpal viadukt (X)")
        return o

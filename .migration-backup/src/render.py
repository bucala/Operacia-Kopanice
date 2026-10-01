"""Vykreslovanie izometrickeho sveta a HUD."""
import math
import os
import pygame

from .iso import grid_to_screen
from .settings import (
    HALF_W, HALF_H, LAYER_H, TERRAIN, VISION_RANGE, PRIMARY_CONE, PERIPHERAL_CONE,
    COL_BG, COL_PLAYER, COL_PLAYER_SEL, COL_GUARD, COL_GUARD_ALERT, COL_BODY,
    COL_TNT, COL_PILLAR, COL_EXIT, COL_TEXT, COL_PANEL, CONE_PRIMARY, CONE_PERIPH,
    COL_SPY, COL_SPY_DISGUISED, COL_SNIPER, COL_THIEF,
)

ASSET_DIR = os.path.join(os.path.dirname(__file__), "..", "assets")
SPRITE_HEIGHT = 70  # cielova vyska sprite v px


def _shade(color, f):
    return tuple(max(0, min(255, int(c * f))) for c in color)


class Renderer:
    def __init__(self, screen):
        self.screen = screen
        pygame.font.init()
        self.font = pygame.font.SysFont("consolas", 16)
        self.font_big = pygame.font.SysFont("consolas", 40, bold=True)
        self.font_mid = pygame.font.SysFont("consolas", 22, bold=True)
        self.overlay = pygame.Surface(screen.get_size(), pygame.SRCALPHA)
        self.sprites = self._load_sprites()

    def _load_sprites(self):
        """Nacita assets/<key>.png ak existuju (spy, spy_disguised, leader, sapper, guard)."""
        sprites = {}
        keys = ["leader", "sapper", "spy", "spy_disguised", "sniper", "thief", "guard"]
        for key in keys:
            path = os.path.join(ASSET_DIR, key + ".png")
            if os.path.isfile(path):
                try:
                    img = pygame.image.load(path).convert_alpha()
                    scale = SPRITE_HEIGHT / img.get_height()
                    size = (int(img.get_width() * scale), SPRITE_HEIGHT)
                    sprites[key] = pygame.transform.smoothscale(img, size)
                except pygame.error:
                    pass
        return sprites

    def _blit_sprite(self, sprite, cx, cy):
        r = sprite.get_rect()
        r.midbottom = (int(cx), int(cy + HALF_H // 2))
        self.screen.blit(sprite, r)

    def world_to_px(self, gx, gy, cam, z=0):
        sx, sy = grid_to_screen(gx, gy, z)
        return sx + cam[0], sy + cam[1]

    # ---------- dlazdice ----------
    def _diamond(self, cx, cy):
        return [(cx, cy - HALF_H), (cx + HALF_W, cy), (cx, cy + HALF_H), (cx - HALF_W, cy)]

    def draw_floor(self, world, cam):
        tm = world.tilemap
        for s in range(tm.w + tm.h):
            for x in range(tm.w):
                y = s - x
                if y < 0 or y >= tm.h:
                    continue
                terr = tm.info(x, y)
                if terr["height"] > 0:
                    continue
                cx, cy = self.world_to_px(x, y, cam)
                pts = self._diamond(cx, cy)
                pygame.draw.polygon(self.screen, terr["color"], pts)
                pygame.draw.polygon(self.screen, _shade(terr["color"], 0.7), pts, 1)

    def _draw_block(self, x, y, color, cam, h=2):
        top_cx, top_cy = self.world_to_px(x, y, cam, z=h)
        base_cx, base_cy = self.world_to_px(x, y, cam, z=0)
        top = self._diamond(top_cx, top_cy)
        # bocne steny
        left = [top[3], top[2], (base_cx, base_cy + HALF_H), (base_cx - HALF_W, base_cy)]
        right = [top[2], top[1], (base_cx + HALF_W, base_cy), (base_cx, base_cy + HALF_H)]
        pygame.draw.polygon(self.screen, _shade(color, 0.55), left)
        pygame.draw.polygon(self.screen, _shade(color, 0.4), right)
        pygame.draw.polygon(self.screen, color, top)
        pygame.draw.polygon(self.screen, _shade(color, 0.7), top, 1)

    # ---------- footprints ----------
    def draw_footprints(self, world, cam):
        for fp in world.footprints:
            cx, cy = self.world_to_px(fp.x, fp.y, cam)
            alpha = max(40, int(180 * (1 - fp.age / fp.ttl)))
            col = (235, 235, 245) if fp.kind == "snow" else (40, 30, 20)
            s = pygame.Surface((8, 6), pygame.SRCALPHA)
            s.fill((*col, alpha))
            self.screen.blit(s, (cx - 4, cy - 3))

    # ---------- vision cones ----------
    def _cone_points(self, tm, gx, gy, facing, half_deg):
        pts = []
        steps = 14
        for i in range(steps + 1):
            ang = facing + math.radians(-half_deg + 2 * half_deg * i / steps)
            dx, dy = math.cos(ang), math.sin(ang)
            d = 0.0
            last = (gx, gy)
            while d < VISION_RANGE:
                d += 0.3
                px, py = gx + dx * d, gy + dy * d
                ix, iy = int(round(px)), int(round(py))
                if not tm.in_bounds(ix, iy):
                    break
                if tm.height(ix, iy) > 1 and (ix, iy) != (int(round(gx)), int(round(gy))):
                    break
                last = (px, py)
            pts.append(last)
        return pts

    def draw_cones(self, world, cam):
        self.overlay.fill((0, 0, 0, 0))
        tm = world.tilemap
        for g in world.guards:
            if not g.alive:
                continue
            gx, gy = g.move.fx, g.move.fy
            base = self.world_to_px(gx, gy, cam)
            for half, col in ((PERIPHERAL_CONE, CONE_PERIPH), (PRIMARY_CONE, CONE_PRIMARY)):
                fan = self._cone_points(tm, gx, gy, g.move.angle, half)
                poly = [base] + [self.world_to_px(px, py, cam) for (px, py) in fan]
                if len(poly) >= 3:
                    c = col
                    if g.state == "ALERT":
                        c = (255, 40, 30, col[3] + 30)
                    pygame.draw.polygon(self.overlay, c, poly)
        self.screen.blit(self.overlay, (0, 0))

    # ---------- entity ----------
    def _depth(self, e):
        return e[0] + e[1]

    def draw_entities(self, world, mission, cam):
        drawables = []
        tm = world.tilemap
        # steny ako bloky
        for y in range(tm.h):
            for x in range(tm.w):
                if tm.info(x, y)["height"] > 0:
                    drawables.append((x, y, "wall", None))
        for (bx, by) in world.bodies:
            drawables.append((bx, by, "body", None))
        # crate, exit
        if mission.tnt_available > 0:
            drawables.append((mission.tnt_crate[0], mission.tnt_crate[1], "crate", None))
        drawables.append((mission.exit[0], mission.exit[1], "exit", None))
        for pil in mission.pillars:
            drawables.append((pil["pos"][0], pil["pos"][1], "marker", pil))
        for g in world.guards:
            if g.alive:
                drawables.append((g.move.fx, g.move.fy, "guard", g))
        for u in world.units:
            if u.alive:
                drawables.append((u.move.fx, u.move.fy, "unit", u))

        drawables.sort(key=self._depth)
        for d in drawables:
            x, y, kind, obj = d
            if kind == "wall":
                color = COL_PILLAR if (x, y) in [p["pos"] for p in mission.pillars] else TERRAIN["wall"]["color"]
                self._draw_block(x, y, color, cam, h=2)
            elif kind == "body":
                cx, cy = self.world_to_px(x, y, cam)
                pygame.draw.ellipse(self.screen, COL_BODY, (cx - 12, cy - 4, 24, 12))
            elif kind == "crate":
                self._draw_crate(x, y, cam)
            elif kind == "exit":
                self._draw_exit(x, y, cam)
            elif kind == "marker":
                self._draw_pillar_marker(x, y, cam, obj)
            elif kind == "guard":
                self._draw_guard(obj, cam)
            elif kind == "unit":
                self._draw_unit(obj, cam)

    def _draw_crate(self, x, y, cam):
        cx, cy = self.world_to_px(x, y, cam)
        rect = pygame.Rect(cx - 12, cy - 18, 24, 20)
        pygame.draw.rect(self.screen, COL_TNT, rect)
        pygame.draw.rect(self.screen, _shade(COL_TNT, 0.6), rect, 2)
        self.screen.blit(self.font.render("TNT", True, (40, 30, 0)), (cx - 13, cy - 16))

    def _draw_exit(self, x, y, cam):
        cx, cy = self.world_to_px(x, y, cam)
        pts = self._diamond(cx, cy)
        pygame.draw.polygon(self.screen, COL_EXIT, pts, 3)
        self.screen.blit(self.font.render("UNIK", True, COL_EXIT), (cx - 16, cy - 8))

    def _draw_pillar_marker(self, x, y, cam, pil):
        cx, cy = self.world_to_px(x, y, cam, z=2)
        col = (90, 220, 120) if pil["planted"] else (240, 200, 60)
        txt = "TNT" if pil["planted"] else "PILIER"
        pygame.draw.circle(self.screen, col, (int(cx), int(cy - 30)), 6)
        self.screen.blit(self.font.render(txt, True, col), (cx - 18, cy - 56))

    def _facing_tip(self, cx, cy, angle, length=18):
        # smer v mriezke -> obrazovka (priblizne cez iso transformaciu smeru)
        dx, dy = math.cos(angle), math.sin(angle)
        sx = (dx - dy) * HALF_W
        sy = (dx + dy) * HALF_H
        n = math.hypot(sx, sy) or 1
        return cx + sx / n * length, cy + sy / n * length

    def _unit_color(self, u):
        if u.role == "spy":
            return COL_SPY_DISGUISED if u.disguised else COL_SPY
        if u.role == "sniper":
            return COL_SNIPER
        if u.role == "thief":
            return COL_THIEF
        return COL_PLAYER_SEL if u.selected else COL_PLAYER

    def _sprite_key(self, u):
        if u.role == "spy" and u.disguised and "spy_disguised" in self.sprites:
            return "spy_disguised"
        return u.role

    def _draw_unit(self, u, cam):
        cx, cy = self.world_to_px(u.move.fx, u.move.fy, cam)
        col = self._unit_color(u)
        if u.selected:
            pygame.draw.ellipse(self.screen, (240, 240, 180), (cx - 16, cy - 6, 32, 14), 2)
        pygame.draw.ellipse(self.screen, (10, 12, 10), (cx - 12, cy - 2, 24, 9))

        key = self._sprite_key(u)
        sprite = self.sprites.get(key)
        if sprite is not None:
            self._blit_sprite(sprite, cx, cy)
            top_y = cy - SPRITE_HEIGHT
        elif u.role == "spy":
            self._draw_spy(cx, cy, col, u)
            top_y = cy - 34
        else:
            h = 16 if not u.crouch else 10
            pygame.draw.rect(self.screen, col, (cx - 5, cy - h, 10, h))
            pygame.draw.circle(self.screen, col, (int(cx), int(cy - h - 5)), 5)
            if u.role == "sniper":
                # puska na chrbte (diagonalna)
                pygame.draw.line(self.screen, (60, 44, 30),
                                 (cx - 8, cy - 2), (cx + 8, cy - h - 8), 3)
            tip = self._facing_tip(cx, cy - h // 2, u.move.angle, 14)
            pygame.draw.line(self.screen, (250, 250, 200), (cx, cy - h // 2), tip, 2)
            top_y = cy - h - 12

        label = u.name + (" (krc)" if u.crouch else "")
        if u.role == "spy" and u.disguised:
            label += " [maska]"
        self.screen.blit(self.font.render(label, True, COL_TEXT), (cx - 26, top_y - 16))
        self._bar(cx - 16, top_y - 2, 32, 4, u.hp / 100, (80, 200, 90))
        if u.role == "sapper" and u.tnt > 0:
            self.screen.blit(self.font.render(f"TNT x{u.tnt}", True, COL_TNT),
                             (cx - 20, cy + 6))
        if u.role == "sniper":
            self.screen.blit(self.font.render(f"Naboje x{u.ammo}", True, (220, 200, 120)),
                             (cx - 26, cy + 6))

    def _draw_spy(self, cx, cy, col, u):
        """Procedurálna silueta dôstojníka: dlhý kožený plášť + štítová čiapka."""
        coat = [(cx - 9, cy - 22), (cx + 9, cy - 22), (cx + 11, cy), (cx - 11, cy)]
        pygame.draw.polygon(self.screen, col, coat)
        pygame.draw.polygon(self.screen, _shade(col, 0.6), coat, 1)
        pygame.draw.line(self.screen, _shade(col, 0.4), (cx, cy - 22), (cx, cy), 1)
        # hlava
        pygame.draw.circle(self.screen, (196, 168, 142), (int(cx), int(cy - 28)), 5)
        # štítová čiapka
        pygame.draw.rect(self.screen, _shade(col, 0.7), (cx - 6, cy - 34, 12, 5))
        pygame.draw.rect(self.screen, (20, 20, 20), (cx - 7, cy - 30, 14, 2))
        tip = self._facing_tip(cx, cy - 12, u.move.angle, 14)
        pygame.draw.line(self.screen, (230, 220, 200), (cx, cy - 12), tip, 2)

    def _draw_guard(self, g, cam):
        cx, cy = self.world_to_px(g.move.fx, g.move.fy, cam)
        col = COL_GUARD_ALERT if g.state == "ALERT" else COL_GUARD
        pygame.draw.ellipse(self.screen, _shade(col, 0.4), (cx - 10, cy - 2, 20, 8))
        pygame.draw.rect(self.screen, col, (cx - 5, cy - 16, 10, 16))
        pygame.draw.circle(self.screen, col, (int(cx), int(cy - 21)), 5)
        tip = self._facing_tip(cx, cy - 8, g.move.angle, 16)
        pygame.draw.line(self.screen, (255, 230, 200), (cx, cy - 8), tip, 2)
        sym = {"PATROL": "", "SUSPICIOUS": "?", "ALERT": "!"}[g.state]
        if sym:
            scol = (255, 220, 60) if g.state == "SUSPICIOUS" else (255, 60, 40)
            self.screen.blit(self.font_mid.render(sym, True, scol), (cx - 5, cy - 46))
        if 0 < g.alert < 100:
            self._bar(cx - 16, cy - 30, 32, 4, g.alert / 100, (240, 200, 60))

    def _bar(self, x, y, w, h, frac, color):
        pygame.draw.rect(self.screen, (30, 30, 30), (x, y, w, h))
        pygame.draw.rect(self.screen, color, (x, y, int(w * max(0, min(1, frac))), h))

    # ---------- HUD ----------
    def draw_hud(self, mission, world, selected, message):
        sw, sh = self.screen.get_size()
        panel = pygame.Surface((300, 150), pygame.SRCALPHA)
        panel.fill((*COL_PANEL, 210))
        self.screen.blit(panel, (8, 8))
        y = 14
        self.screen.blit(self.font_mid.render("MISIA 1: Zelezna hrdelnica", True, COL_TEXT), (16, y))
        y += 30
        for o in mission.objectives():
            self.screen.blit(self.font.render(o, True, COL_TEXT), (16, y))
            y += 20

        # ovladanie
        help_lines = [
            "1-5 vyber  |  L-klik: pohyb  |  C: krcat",
            "F: likvidacia (Vodca)  R: odstrel (Odstrelovac)",
            "G: maskovanie  T: falosny rozkaz (Spion)  V: preliezt (Zved)",
            "E: vziat TNT  Q: polozit naloz  X: odpalit",
            "Sipky/WASD: kamera",
        ]
        hy = sh - 22 * len(help_lines) - 10
        hp = pygame.Surface((430, 22 * len(help_lines) + 6), pygame.SRCALPHA)
        hp.fill((*COL_PANEL, 200))
        self.screen.blit(hp, (8, hy - 4))
        for ln in help_lines:
            self.screen.blit(self.font.render(ln, True, COL_TEXT), (16, hy))
            hy += 22

        if selected:
            info = f"Vybrany: {selected.name}  HP {selected.hp}"
            self.screen.blit(self.font.render(info, True, COL_PLAYER_SEL), (sw - 280, 14))

        if message:
            mw = self.font_mid.size(message[0])[0]
            self.screen.blit(self.font_mid.render(message[0], True, message[1]),
                             (sw // 2 - mw // 2, 16))

    def draw_end(self, status):
        sw, sh = self.screen.get_size()
        ov = pygame.Surface((sw, sh), pygame.SRCALPHA)
        ov.fill((0, 0, 0, 180))
        self.screen.blit(ov, (0, 0))
        if status == "win":
            txt, col = "MISIA SPLNENA - Viadukt zniceny!", (120, 230, 140)
        else:
            txt, col = "MISIA ZLYHALA", (240, 90, 70)
        w = self.font_big.size(txt)[0]
        self.screen.blit(self.font_big.render(txt, True, col), (sw // 2 - w // 2, sh // 2 - 40))
        sub = "R - restart    ESC - koniec"
        w2 = self.font_mid.size(sub)[0]
        self.screen.blit(self.font_mid.render(sub, True, COL_TEXT), (sw // 2 - w2 // 2, sh // 2 + 20))

"""Herna slucka, vstup a kamera."""
import pygame

from . import ai
from .iso import grid_to_screen, screen_to_grid
from .mission import build_mission
from .render import Renderer
from .settings import (
    SCREEN_W, SCREEN_H, FPS, TITLE, COL_BG, NOISE_STEP, TERRAIN,
)
from .skills import (
    Takedown, PickTnt, PlantTnt, Detonate, Disguise, FalseOrder, Snipe, Climb,
)


class Game:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((SCREEN_W, SCREEN_H))
        pygame.display.set_caption(TITLE)
        self.clock = pygame.time.Clock()
        self.renderer = Renderer(self.screen)
        self.reset()

    def reset(self):
        self.world, self.mission = build_mission()
        self.selected = self.world.units[0]
        self.selected.selected = True
        self.message = None
        self.msg_ttl = 0.0
        self.state = "playing"
        self._center_camera()

    def _center_camera(self):
        tm = self.world.tilemap
        cx, cy = grid_to_screen(tm.w / 2, tm.h / 2)
        self.cam = [SCREEN_W / 2 - cx, SCREEN_H / 2 - cy - 60]

    def set_message(self, text, color=(230, 228, 220)):
        self.message = (text, color)
        self.msg_ttl = 2.5

    # ---------------- vstup ----------------
    def select_index(self, i):
        units = self.world.alive_units()
        for u in self.world.units:
            u.selected = False
        if 0 <= i < len(self.world.units) and self.world.units[i].alive:
            self.selected = self.world.units[i]
            self.selected.selected = True

    def mouse_to_tile(self, mx, my):
        gx, gy = screen_to_grid(mx - self.cam[0], my - self.cam[1])
        return int(round(gx)), int(round(gy)), gx, gy

    def on_click(self, mx, my):
        tx, ty, gx, gy = self.mouse_to_tile(mx, my)
        # vyber jednotky
        for u in self.world.alive_units():
            if abs(u.move.fx - gx) < 0.8 and abs(u.move.fy - gy) < 0.8:
                for o in self.world.units:
                    o.selected = False
                self.selected = u
                u.selected = True
                return
        # pohyb
        if self.selected and self.selected.alive and self.world.tilemap.walkable(tx, ty):
            from .pathfinding import astar
            path = astar(self.world.tilemap, (self.selected.move.tx, self.selected.move.ty),
                         (tx, ty), blocked=self.world.blocked_tiles(exclude=self.selected))
            if path:
                self.selected.move.set_path(path)
            else:
                self.set_message("Cesta nedostupna.", (240, 120, 90))

    def try_skill(self, skill):
        if not self.selected or not self.selected.alive:
            return
        ok, msg = skill.validate(self.world, self.mission, self.selected)
        if not ok:
            self.set_message(msg, (240, 160, 80))
            return
        ok, msg = skill.execute(self.world, self.mission, self.selected)
        self.set_message(msg, (140, 230, 150) if ok else (240, 120, 90))

    def handle_event(self, e):
        if e.type == pygame.QUIT:
            self.running = False
        elif e.type == pygame.KEYDOWN:
            if e.key == pygame.K_ESCAPE:
                self.running = False
            elif self.state != "playing":
                if e.key == pygame.K_r:
                    self.reset()
                return
            elif e.key == pygame.K_1:
                self.select_index(0)
            elif e.key == pygame.K_2:
                self.select_index(1)
            elif e.key == pygame.K_3:
                self.select_index(2)
            elif e.key == pygame.K_4:
                self.select_index(3)
            elif e.key == pygame.K_5:
                self.select_index(4)
            elif e.key == pygame.K_c and self.selected:
                self.selected.crouch = not self.selected.crouch
            elif e.key == pygame.K_f:
                self.try_skill(Takedown)
            elif e.key == pygame.K_g:
                self.try_skill(Disguise)
            elif e.key == pygame.K_t:
                self.try_skill(FalseOrder)
            elif e.key == pygame.K_r:
                self.try_skill(Snipe)
            elif e.key == pygame.K_v:
                self.try_skill(Climb)
            elif e.key == pygame.K_e:
                self.try_skill(PickTnt)
            elif e.key == pygame.K_q:
                self.try_skill(PlantTnt)
            elif e.key == pygame.K_x:
                self.try_skill(Detonate)
        elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1 and self.state == "playing":
            self.on_click(*e.pos)

    def pan_camera(self, dt):
        keys = pygame.key.get_pressed()
        sp = 500 * dt
        if keys[pygame.K_LEFT] or keys[pygame.K_a]:
            self.cam[0] += sp
        if keys[pygame.K_RIGHT] or keys[pygame.K_d]:
            self.cam[0] -= sp
        if keys[pygame.K_UP] or keys[pygame.K_w]:
            self.cam[1] += sp
        if keys[pygame.K_DOWN] or keys[pygame.K_s]:
            self.cam[1] -= sp

    # ---------------- update ----------------
    def update(self, dt):
        self.pan_camera(dt)
        if self.msg_ttl > 0:
            self.msg_ttl -= dt
            if self.msg_ttl <= 0:
                self.message = None
        if self.state != "playing":
            return

        self.world.noises = []
        tm = self.world.tilemap
        for u in self.world.alive_units():
            stepped = u.move.update(dt, tm, u.speed_mod)
            if stepped:
                self.world.spawn_footprint(*stepped)
            if u.move.moving and not u.crouch:
                terr = tm.terrain(u.move.tx, u.move.ty)
                if TERRAIN[terr]["cost"] > 1.0:
                    self.world.add_noise(u.move.tx, u.move.ty, NOISE_STEP)

        ai.update(self.world, dt)
        self.world.update_footprints(dt)

        st = self.mission.status()
        if st != "playing":
            self.state = st

    def draw(self):
        self.screen.fill(COL_BG)
        self.renderer.draw_floor(self.world, self.cam)
        self.renderer.draw_footprints(self.world, self.cam)
        self.renderer.draw_cones(self.world, self.cam)
        self.renderer.draw_entities(self.world, self.mission, self.cam)
        self.renderer.draw_hud(self.mission, self.world, self.selected, self.message)
        if self.state != "playing":
            self.renderer.draw_end(self.state)
        pygame.display.flip()

    def run(self):
        self.running = True
        while self.running:
            dt = self.clock.tick(FPS) / 1000.0
            dt = min(dt, 0.05)
            for e in pygame.event.get():
                self.handle_event(e)
            self.update(dt)
            self.draw()
        pygame.quit()

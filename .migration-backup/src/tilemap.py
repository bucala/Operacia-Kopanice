"""Dlazdicova mapa s terenom, vyskami a nahliadnutim (LOS)."""
from .settings import TERRAIN


class TileMap:
    def __init__(self, grid):
        """grid: zoznam riadkov (stringov / zoznamov) terrain id."""
        self.tiles = grid
        self.h = len(grid)
        self.w = len(grid[0]) if grid else 0

    def in_bounds(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h

    def terrain(self, x, y):
        return self.tiles[y][x]

    def info(self, x, y):
        return TERRAIN[self.tiles[y][x]]

    def cost(self, x, y):
        if not self.in_bounds(x, y):
            return float("inf")
        return TERRAIN[self.tiles[y][x]]["cost"]

    def walkable(self, x, y):
        return self.in_bounds(x, y) and self.cost(x, y) != float("inf")

    def height(self, x, y):
        if not self.in_bounds(x, y):
            return 99
        return TERRAIN[self.tiles[y][x]]["height"]

    def set(self, x, y, terrain_id):
        self.tiles[y][x] = terrain_id

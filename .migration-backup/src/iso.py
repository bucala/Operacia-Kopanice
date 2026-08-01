"""Izometricka transformacia a depth sorting.

screenX = (x - y) * W_t
screenY = (x + y) * H_t - z * H_l
RenderOrder = x + y + z
"""
from .settings import HALF_W, HALF_H, LAYER_H


def grid_to_screen(x, y, z=0):
    sx = (x - y) * HALF_W
    sy = (x + y) * HALF_H - z * LAYER_H
    return sx, sy


def screen_to_grid(sx, sy):
    """Inverzia (z=0). Vracia float mriezkove suradnice."""
    gx = (sx / HALF_W + sy / HALF_H) / 2.0
    gy = (sy / HALF_H - sx / HALF_W) / 2.0
    return gx, gy


def render_order(x, y, z=0):
    return x + y + z

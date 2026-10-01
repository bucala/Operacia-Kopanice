"""A* pathfinding s vahou terenu.

f(n) = g(n) + h(n), kde h je Manhattanska heuristika.
Cena kroku do suseda je nasobena koeficientom terenu (g_m).
"""
import heapq

# 4-smerove susedstvo (izometricke kardinalne smery)
NEIGHBORS = [(1, 0), (-1, 0), (0, 1), (0, -1)]


def manhattan(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1])


def astar(tilemap, start, goal, blocked=None):
    """Vrati zoznam dlaznic [start..goal] alebo None.

    blocked: mnozina (x,y) docasne neprejazdnych poli (napr. ine entity).
    """
    if blocked is None:
        blocked = set()
    if not tilemap.walkable(*goal) or goal in blocked:
        return None

    open_heap = [(0.0, start)]
    came_from = {}
    g_score = {start: 0.0}
    closed = set()

    while open_heap:
        _, current = heapq.heappop(open_heap)
        if current == goal:
            return _reconstruct(came_from, current)
        if current in closed:
            continue
        closed.add(current)

        cx, cy = current
        for dx, dy in NEIGHBORS:
            nx, ny = cx + dx, cy + dy
            nxt = (nx, ny)
            if not tilemap.walkable(nx, ny) or nxt in blocked or nxt in closed:
                continue
            step = tilemap.cost(nx, ny)
            tentative = g_score[current] + step
            if tentative < g_score.get(nxt, float("inf")):
                came_from[nxt] = current
                g_score[nxt] = tentative
                f = tentative + manhattan(nxt, goal)
                heapq.heappush(open_heap, (f, nxt))
    return None


def _reconstruct(came_from, current):
    path = [current]
    while current in came_from:
        current = came_from[current]
        path.append(current)
    path.reverse()
    return path

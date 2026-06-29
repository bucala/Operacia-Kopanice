import { BinaryHeap } from '@/core/util/BinaryHeap';
import type { Vec2 } from '@/core/math/Vec2';
import type { TileMap } from './TileMap';

interface Node {
  x: number;
  y: number;
  g: number; // cost from start
  f: number; // g + heuristic
  parent: Node | null;
}

const SQRT2 = Math.SQRT2;

const keyOf = (x: number, y: number): number => y * 100000 + x;

/**
 * Weighted A* over the tile grid. Entering a tile costs that tile's `moveCost`
 * (so snow/mud are expensive and roads cheap); diagonal steps are scaled by √2.
 * The octile-distance heuristic stays admissible by multiplying by the cheapest
 * possible tile cost, guaranteeing optimal paths.
 *
 * @returns the list of grid steps from (excluding) `start` to (including)
 *          `goal`, or an empty array if unreachable.
 */
export function findPath(
  map: TileMap,
  start: Vec2,
  goal: Vec2,
  options: { minTileCost?: number } = {},
): Vec2[] {
  const sx = Math.round(start.x);
  const sy = Math.round(start.y);
  const gx = Math.round(goal.x);
  const gy = Math.round(goal.y);

  if (!map.isWalkable(gx, gy)) return [];
  if (sx === gx && sy === gy) return [];

  const minCost = options.minTileCost ?? 1;
  const heuristic = (x: number, y: number): number => {
    const dx = Math.abs(x - gx);
    const dy = Math.abs(y - gy);
    // Octile distance scaled by the cheapest tile cost (admissible).
    return (Math.max(dx, dy) + (SQRT2 - 1) * Math.min(dx, dy)) * minCost;
  };

  const open = new BinaryHeap<Node>((n) => n.f);
  const bestG = new Map<number, number>();
  const closed = new Set<number>();

  const startNode: Node = { x: sx, y: sy, g: 0, f: heuristic(sx, sy), parent: null };
  open.push(startNode);
  bestG.set(keyOf(sx, sy), 0);

  while (open.size > 0) {
    const current = open.pop()!;
    const ck = keyOf(current.x, current.y);
    if (current.x === gx && current.y === gy) {
      return reconstruct(current);
    }
    if (closed.has(ck)) continue;
    closed.add(ck);

    for (const n of map.neighbors(current.x, current.y)) {
      const nk = keyOf(n.x, n.y);
      if (closed.has(nk)) continue;
      const diagonal = n.x !== current.x && n.y !== current.y;
      const stepCost = map.moveCost(n.x, n.y) * (diagonal ? SQRT2 : 1);
      const g = current.g + stepCost;
      if (g < (bestG.get(nk) ?? Infinity)) {
        bestG.set(nk, g);
        open.push({ x: n.x, y: n.y, g, f: g + heuristic(n.x, n.y), parent: current });
      }
    }
  }
  return [];
}

function reconstruct(node: Node): Vec2[] {
  const path: Vec2[] = [];
  let cur: Node | null = node;
  while (cur && cur.parent) {
    path.push({ x: cur.x, y: cur.y });
    cur = cur.parent;
  }
  path.reverse();
  return path;
}

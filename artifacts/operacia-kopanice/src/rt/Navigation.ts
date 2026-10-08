import PF from "pathfinding";
import { type Mission, type Point, finite, distance } from "./types";

export class Navigation {
  readonly resolution = 0.3;
  private grid!: PF.Grid;
  bridgeDestroyed = false;
  constructor(readonly mission: Mission) {
    this.rebuild();
  }
  riverX(y: number): number {
    return (
      (this.mission.river?.x ?? 0) +
      0.32 * Math.sin((y - (this.mission.river?.bridgeY ?? 0)) * 0.4)
    );
  }
  blocked(p: Point, radius = 0.3, sight = false): boolean {
    const m = this.mission;
    if (
      !finite(p) ||
      p.x < radius ||
      p.y < radius ||
      p.x > m.width - radius ||
      p.y > m.height - radius
    )
      return true;
    if (
      !sight &&
      m.river &&
      Math.abs(p.x - this.riverX(p.y)) < m.river.width / 2 + radius
    ) {
      if (
        this.bridgeDestroyed ||
        Math.abs(p.y - m.river.bridgeY) > m.river.bridgeWidth / 2 - radius
      )
        return true;
    }
    return m.props.some(
      (o) =>
        o.kind !== "shrub" &&
        Math.abs(p.x - o.x) < o.width / 2 + radius &&
        Math.abs(p.y - o.y) < o.height / 2 + radius,
    );
  }
  clear(a: Point, b: Point, sight = false): boolean {
    if (!finite(a) || !finite(b)) return false;
    const steps = Math.max(1, Math.ceil(distance(a, b) / 0.12));
    for (let i = 0; i <= steps; i++) {
      const t = i / steps;
      if (
        this.blocked(
          { x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t },
          sight ? 0 : 0.3,
          sight,
        )
      )
        return false;
    }
    return true;
  }
  rebuild(): void {
    const r = this.resolution;
    this.grid = new PF.Grid(
      Math.ceil(this.mission.width / r) + 1,
      Math.ceil(this.mission.height / r) + 1,
    );
    for (let y = 0; y < this.grid.height; y++)
      for (let x = 0; x < this.grid.width; x++)
        this.grid.setWalkableAt(x, y, !this.blocked({ x: x * r, y: y * r }));
  }
  path(from: Point, to: Point): Point[] | null {
    if (this.blocked(from) || this.blocked(to)) return null;
    if (this.clear(from, to)) return [{ ...to }];
    const r = this.resolution,
      sx = Math.round(from.x / r),
      sy = Math.round(from.y / r),
      tx = Math.round(to.x / r),
      ty = Math.round(to.y / r);
    if (!this.grid.isWalkableAt(sx, sy) || !this.grid.isWalkableAt(tx, ty))
      return null;
    const finder = new PF.AStarFinder({
      allowDiagonal: true,
      dontCrossCorners: true,
      heuristic: PF.Heuristic.octile,
    });
    const raw = finder.findPath(sx, sy, tx, ty, this.grid.clone());
    if (!raw.length) return null;
    const points = PF.Util.compressPath(raw).map(([x, y]) => ({
      x: x * r,
      y: y * r,
    }));
    points.push({ ...to });
    let previous = from;
    for (const p of points) {
      if (!this.clear(previous, p)) return null;
      previous = p;
    }
    return points;
  }
}

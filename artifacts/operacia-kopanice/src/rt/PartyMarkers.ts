import { type Point, type Unit } from "./types";

export interface ScreenRect {
  left: number;
  top: number;
  right: number;
  bottom: number;
}
export interface PartyMarker {
  unit: Unit;
  number: number;
  anchor: Point;
  center: Point;
  bounds: ScreenRect;
}
const SIZE = 40;
export const intersects = (a: ScreenRect, b: ScreenRect) =>
  a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
export const contains = (r: ScreenRect, p: Point) =>
  p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom;
export const bodyHeight = (u: Unit, scale: number) =>
  scale * (u.stance === "prone" ? 0.7 : u.stance === "crouch" ? 1.35 : 1.85);

export function layoutPartyMarkers(
  party: readonly Unit[],
  project: (p: Point) => Point,
  scale: number,
  width: number,
  height: number,
  exclusions: readonly ScreenRect[] = [],
): PartyMarker[] {
  const markers: PartyMarker[] = [];
  // Stable party order keeps neighbours from swapping sides as their depth changes.
  party.forEach((unit, index) => {
    if (unit.enemy || unit.health <= 0 || unit.hidden || unit.carriedBy) return;
    const foot = project(unit),
      anchor = { x: foot.x, y: foot.y - bodyHeight(unit, scale) };
    if (foot.x < 0 || foot.x > width || foot.y < 0 || anchor.y > height) return;
    const ideal = { x: anchor.x, y: anchor.y - SIZE / 2 - 5 };
    const offsets = [
      [0, 0],
      [-44, 0],
      [44, 0],
      [0, -44],
      [-44, -44],
      [44, -44],
      [-88, 0],
      [88, 0],
      [0, 44],
    ];
    for (const [dx, dy] of offsets) {
      const center = {
        x: Math.max(24, Math.min(width - 24, ideal.x + dx)),
        y: Math.max(24, Math.min(height - 24, ideal.y + dy)),
      };
      const bounds = {
        left: center.x - SIZE / 2,
        right: center.x + SIZE / 2,
        top: center.y - SIZE / 2,
        bottom: center.y + SIZE / 2,
      };
      if (
        width < 48 ||
        height < 48 ||
        exclusions.some((r) => intersects(bounds, r)) ||
        markers.some((m) =>
          intersects(bounds, {
            left: m.bounds.left - 4,
            right: m.bounds.right + 4,
            top: m.bounds.top - 4,
            bottom: m.bounds.bottom + 4,
          }),
        )
      )
        continue;
      markers.push({ unit, number: index + 1, anchor, center, bounds });
      break;
    }
  });
  return markers;
}

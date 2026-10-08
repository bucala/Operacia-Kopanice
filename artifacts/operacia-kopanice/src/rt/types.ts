export interface Point {
  x: number;
  y: number;
}
export type Stance = "walk" | "run" | "crouch" | "prone";
export type Alert = "patrol" | "investigate" | "combat";
export type Action = "move" | "interact" | "takedown" | "carry" | "distract";
export interface Order {
  kind: Action;
  position: Point;
  target?: string;
  run?: boolean;
}
export interface Stop extends Point {
  facing: number;
  wait: number;
}
export interface Prop extends Point {
  kind: "house1" | "house2" | "tree" | "rock" | "crate" | "vehicle" | "shrub";
  width: number;
  height: number;
}
export interface Mission {
  id: number;
  name: string;
  briefing: string;
  width: number;
  height: number;
  start: Point;
  supply: Point;
  target: Point;
  exit: Point;
  hide: Point;
  goal: "bridge" | "documents" | "command";
  river?: { x: number; width: number; bridgeY: number; bridgeWidth: number };
  props: Prop[];
  patrols: Stop[][];
}
export interface Unit extends Point {
  id: string;
  name: string;
  enemy: boolean;
  facing: number;
  health: number;
  stance: Stance;
  selected: boolean;
  orders: Order[];
  path: Point[];
  started: boolean;
  moving: boolean;
  cooldown: number;
  charges: number;
  alert: Alert;
  suspicion: number;
  patrol: Stop[];
  stop: number;
  wait: number;
  lastKnown?: Point;
  search: number;
  shot: number;
  repath: number;
  carrying?: string;
  carriedBy?: string;
  hidden: boolean;
}
export const distance = (a: Point, b: Point) =>
  Math.hypot(a.x - b.x, a.y - b.y);
export const angleTo = (a: Point, b: Point) => Math.atan2(b.y - a.y, b.x - a.x);
export const angleDelta = (a: number, b: number) =>
  Math.atan2(Math.sin(b - a), Math.cos(b - a));
export const finite = (p: Point) =>
  Number.isFinite(p.x) && Number.isFinite(p.y);

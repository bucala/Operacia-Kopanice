import { Renderer } from "./Renderer";
import { type Action, type Point, distance } from "./types";

export class Pointer {
  action: Action = "move";
  shift = false;
  private points = new Map<number, Point>();
  private down?: {
    p: Point;
    last: Point;
    button: number;
    touch: boolean;
    shift: boolean;
    dragged: boolean;
  };
  private gesture = false;
  private pinch = 0;
  private lastCommand?: { time: number; point: Point };
  private listeners: (() => void)[] = [];
  constructor(
    readonly renderer: Renderer,
    readonly changed: () => void,
  ) {
    const c = renderer.canvas;
    const on = <K extends keyof HTMLElementEventMap>(
      name: K,
      fn: (e: HTMLElementEventMap[K]) => void,
    ) => {
      c.addEventListener(name, fn);
      this.listeners.push(() => c.removeEventListener(name, fn));
    };
    on("contextmenu", (e) => e.preventDefault());
    on("pointerdown", (e) => this.start(e));
    on("pointermove", (e) => this.move(e));
    on("pointerup", (e) => this.end(e));
    on("pointercancel", () => this.reset());
    on("lostpointercapture", () => {
      if (!this.points.size) this.reset();
    });
    const wheel = (e: WheelEvent) => {
      e.preventDefault();
      renderer.zoom(
        Math.exp(-Math.max(-120, Math.min(120, e.deltaY)) * 0.002),
        this.position(e),
      );
    };
    c.addEventListener("wheel", wheel, { passive: false });
    this.listeners.push(() => c.removeEventListener("wheel", wheel));
  }
  private position(e: { clientX: number; clientY: number }): Point {
    const r = this.renderer.canvas.getBoundingClientRect();
    return { x: e.clientX - r.left, y: e.clientY - r.top };
  }
  reset(): void {
    this.points.clear();
    this.down = undefined;
    this.gesture = false;
    this.pinch = 0;
    this.renderer.selection = undefined;
  }
  destroy(): void {
    this.listeners.forEach((off) => off());
    this.reset();
  }
  private start(e: PointerEvent): void {
    if (this.renderer.sim.menu || this.renderer.sim.outcome) return;
    e.preventDefault();
    this.renderer.canvas.setPointerCapture(e.pointerId);
    const p = this.position(e);
    this.points.set(e.pointerId, p);
    if (this.points.size > 1) {
      this.gesture = true;
      this.down = undefined;
      this.renderer.selection = undefined;
      const [a, b] = [...this.points.values()];
      this.pinch = distance(a, b);
      return;
    }
    this.gesture = false;
    this.down = {
      p,
      last: p,
      button: e.button,
      touch: e.pointerType === "touch",
      shift: e.shiftKey,
      dragged: false,
    };
  }
  private move(e: PointerEvent): void {
    const p = this.position(e);
    this.shift = e.shiftKey;
    this.renderer.hover = this.renderer.unproject(p);
    if (this.points.has(e.pointerId)) this.points.set(e.pointerId, p);
    if (this.gesture) {
      if (this.points.size >= 2) {
        const [a, b] = [...this.points.values()],
          span = distance(a, b);
        if (this.pinch > 0)
          this.renderer.zoom(span / this.pinch, {
            x: (a.x + b.x) / 2,
            y: (a.y + b.y) / 2,
          });
        this.pinch = span;
      }
      return;
    }
    const d = this.down;
    if (d) {
      d.dragged ||= distance(p, d.p) > 7;
      if (d.dragged) {
        if (d.touch || d.button === 1)
          this.renderer.pan(p.x - d.last.x, p.y - d.last.y);
        else if (d.button === 0) this.renderer.selection = { from: d.p, to: p };
      }
      d.last = p;
    }
    this.renderer.preview =
      this.action === "move" && this.renderer.paths && !d
        ? this.renderer.sim.preview(this.renderer.hover, e.shiftKey)
        : null;
  }
  private end(e: PointerEvent): void {
    const p = this.position(e),
      d = this.down;
    this.points.delete(e.pointerId);
    if (this.gesture) {
      if (!this.points.size) this.reset();
      return;
    }
    if (!d) return;
    if (!this.renderer.sim.menu && !this.renderer.sim.outcome) {
      if (d.dragged && d.button === 0 && !d.touch) {
        const sim = this.renderer.sim;
        if (!d.shift) sim.party.forEach((u) => (u.selected = false));
        sim.party.forEach((u, i) => {
          const q = this.renderer.project(u);
          if (
            q.x >= Math.min(p.x, d.p.x) &&
            q.x <= Math.max(p.x, d.p.x) &&
            q.y >= Math.min(p.y, d.p.y) &&
            q.y <= Math.max(p.y, d.p.y)
          )
            sim.select(i, true);
        });
      } else if (!d.dragged && d.button !== 1)
        this.click(p, d.button, d.touch, d.shift);
    }
    this.down = undefined;
    this.renderer.selection = undefined;
    this.changed();
  }
  private click(
    screen: Point,
    button: number,
    touch: boolean,
    append: boolean,
  ): void {
    const sim = this.renderer.sim,
      u = this.renderer.pick(screen),
      p = this.renderer.unproject(screen);
    if (button === 2 && this.action !== "move") {
      this.action = "move";
      this.renderer.preview = null;
      return;
    }
    if (button === 0 && u && !u.enemy && this.action === "move") {
      sim.select(sim.party.indexOf(u), append);
      return;
    }
    if (button === 0 && !touch && this.action === "move") return;
    let kind = this.action,
      target: string | undefined,
      position = p;
    if (kind === "move" && u?.enemy) {
      kind = u.health > 0 ? "takedown" : "carry";
      target = u.id;
      position = u;
    } else if (kind === "takedown" || kind === "carry") {
      target = u?.id;
      position = u ?? p;
    } else if (kind === "move" || kind === "interact") {
      const point = [
        sim.mission.supply,
        sim.mission.target,
        sim.mission.hide,
        sim.mission.exit,
      ].find((q) => distance(p, q) < 1.1);
      if (point && point !== sim.mission.exit) {
        kind = "interact";
        position = point;
      }
    }
    const now = performance.now(),
      run =
        kind === "move" &&
        !!this.lastCommand &&
        now - this.lastCommand.time < 350 &&
        distance(p, this.lastCommand.point) < 0.5;
    // Upgrade the current move without appending a duplicate in tactical pause.
    let accepted = false;
    if (run)
      sim.party
        .filter((u) => u.selected)
        .forEach((u) => {
          const last = u.orders.at(-1);
          if (last?.kind === "move" && distance(last.position, p) < 0.65) {
            last.run = true;
            accepted = true;
            if (!sim.paused && u.started && last === u.orders[0])
              u.stance = "run";
          }
        });
    else accepted = sim.command(kind, position, target, append, run);
    this.lastCommand =
      kind === "move" && accepted ? { time: now, point: p } : undefined;
    this.action = "move";
    this.renderer.preview = null;
  }
}

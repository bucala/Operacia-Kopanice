import { Navigation } from "./Navigation";
import {
  type Action,
  type Mission,
  type Order,
  type Point,
  type Stance,
  type Stop,
  type Unit,
  angleDelta,
  angleTo,
  distance,
  finite,
} from "./types";

const SPEED: Record<Stance, number> = {
  walk: 1.9,
  run: 3.8,
  crouch: 0.9,
  prone: 0.5,
};
const VISIBILITY: Record<Stance, number> = {
  walk: 1,
  run: 1,
  crouch: 0.35,
  prone: 0.15,
};
export const SIGHT = {
  near: 3.3,
  far: 10,
  nearAngle: (35 * Math.PI) / 180,
  farAngle: Math.PI / 4,
};
function unit(
  id: string,
  name: string,
  p: Point,
  enemy = false,
  patrol: Stop[] = [],
): Unit {
  return {
    ...p,
    id,
    name,
    enemy,
    patrol,
    facing: patrol[0]?.facing ?? -Math.PI / 2,
    health: 100,
    stance: "walk",
    selected: !enemy && id === "partisan",
    orders: [],
    path: [],
    started: false,
    moving: false,
    cooldown: 0,
    charges: 5,
    alert: "patrol",
    suspicion: 0,
    stop: 0,
    wait: 0,
    search: 0,
    shot: 0.65,
    repath: 0,
    hidden: false,
  };
}
export class Simulation {
  readonly nav: Navigation;
  readonly party: Unit[];
  readonly guards: Unit[];
  active = 0;
  paused = false;
  menu = false;
  elapsed = 0;
  outcome: "won" | "lost" | null = null;
  hasSupply = false;
  objectiveDone = false;
  message = "Infiltracia zacala.";
  feedback?: {
    position: Point;
    accepted: number;
    requested: number;
    issued: number;
  };
  readonly noises: { position: Point; radius: number; age: number }[] = [];
  constructor(readonly mission: Mission) {
    this.nav = new Navigation(mission);
    this.party = [
      unit("partisan", "Partizan", mission.start),
      unit("officer", "Dostojnik", {
        x: mission.start.x + 1.8,
        y: mission.start.y,
      }),
    ];
    this.guards = mission.patrols.map((route, i) =>
      unit("guard-" + i, "Hliadka " + (i + 1), route[0], true, route),
    );
  }
  get units(): Unit[] {
    return [...this.party, ...this.guards];
  }
  get objective(): string {
    if (this.outcome)
      return this.outcome === "won" ? "Misia splnena" : "Misia zlyhala";
    if (this.objectiveDone) return "Dosiahnite vychod s oboma clenmi";
    if (this.mission.goal === "documents") return "Ziskajte dokumenty z tabora";
    if (!this.hasSupply) return "Ziskajte TNT";
    if (this.mission.goal === "bridge")
      return this.party.every((p) => this.eastBank(p))
        ? "Aktivujte detonator"
        : "Presunte oboch clenov cez most";
    return "Vyradte velitelstvo";
  }
  select(index: number, append = false): void {
    if (this.menu || this.outcome || !this.party[index]?.health) return;
    if (!append) this.party.forEach((p) => (p.selected = false));
    this.party[index].selected = true;
    this.active = index;
  }
  selectAll(): void {
    if (!this.menu && !this.outcome)
      this.party.forEach((p) => (p.selected = p.health > 0));
  }
  next(): void {
    const selected = this.party
      .map((p, i) => (p.selected ? i : -1))
      .filter((i) => i >= 0);
    if (selected.length > 1)
      this.select(
        selected[(selected.indexOf(this.active) + 1) % selected.length],
        true,
      );
    else this.select(1 - this.active);
  }
  stance(stance: Stance): void {
    if (!this.menu && !this.outcome)
      this.party.filter((p) => p.selected).forEach((p) => (p.stance = stance));
  }
  togglePause(): void {
    if (!this.menu && !this.outcome) this.paused = !this.paused;
  }
  stop(): void {
    if (this.menu || this.outcome) return;
    this.party
      .filter((p) => p.selected)
      .forEach((p) => {
        p.orders = [];
        p.path = [];
        p.started = false;
        p.moving = false;
      });
  }
  undo(): boolean {
    const p = this.party[this.active];
    if (
      !this.paused ||
      this.menu ||
      this.outcome ||
      !p.selected ||
      p.orders.length <= (p.started ? 1 : 0)
    )
      return false;
    p.orders.pop();
    return true;
  }
  cover(p: Unit): boolean {
    return (
      (p.stance === "crouch" || p.stance === "prone") &&
      this.mission.props.some(
        (o) =>
          (o.kind === "shrub" || o.kind === "crate" || o.kind === "rock") &&
          distance(p, o) < Math.max(o.width, o.height) / 2 + 0.75,
      )
    );
  }
  visible(guard: Unit, p: Point): "near" | "far" | null {
    const d = distance(guard, p),
      a = Math.abs(angleDelta(guard.facing, angleTo(guard, p)));
    if (d > SIGHT.far || a > SIGHT.farAngle || !this.nav.clear(guard, p, true))
      return null;
    return d < SIGHT.near && a < SIGHT.nearAngle ? "near" : "far";
  }
  preview(position: Point, append = false): Point[] | null {
    const p = this.party[this.active];
    if (this.menu || this.outcome || !p.selected || p.health <= 0) return null;
    const selected = this.party.filter((u) => u.selected && u.health > 0);
    const dest = { ...position };
    if (selected.length > 1) dest.y += selected.indexOf(p) === 0 ? -0.45 : 0.45;
    const from =
      (append || this.paused) && p.orders.length
        ? p.orders.at(-1)!.position
        : p;
    const path = this.nav.path(from, dest);
    return path ? [{ x: from.x, y: from.y }, ...path] : null;
  }
  command(
    kind: Action,
    position: Point,
    target?: string,
    append = false,
    run = false,
  ): boolean {
    if (this.menu || this.outcome || !finite(position)) return false;
    const selected =
      kind === "move"
        ? this.party.filter((p) => p.selected && p.health > 0)
        : [this.party[this.active]].filter((p) => p.selected && p.health > 0);
    let accepted = 0,
      reason = "";
    for (const p of selected) {
      const dest = { ...position };
      if (kind === "move" && selected.length > 1)
        dest.y += selected.indexOf(p) === 0 ? -0.45 : 0.45;
      const order: Order = { kind, position: dest, target, run };
      const failure = this.validate(p, order, append || this.paused);
      if (failure) {
        reason ||= p.name + ": " + failure;
        continue;
      }
      if (!append && !this.paused) {
        p.orders = [];
        p.path = [];
        p.started = false;
      }
      p.orders.push(order);
      accepted++;
    }
    this.feedback = {
      position: { ...position },
      accepted,
      requested: selected.length,
      issued: performance.now(),
    };
    this.message =
      accepted === selected.length && accepted > 0
        ? "Rozkaz prijaty: " + accepted + "/" + selected.length
        : accepted
          ? "Prijate " + accepted + "/" + selected.length + ". " + reason
          : reason || "Vyberte postavu.";
    return accepted > 0;
  }
  private validate(p: Unit, o: Order, append: boolean): string | null {
    if (append && p.orders.length >= 32) return "Front rozkazov je plny.";
    const target = this.units.find((u) => u.id === o.target);
    if (o.kind === "takedown" && (!target?.enemy || target.health <= 0))
      return "Vyberte zivu hliadku.";
    if (
      o.kind === "carry" &&
      (!target?.enemy ||
        target.health > 0 ||
        target.carriedBy ||
        target.hidden ||
        p.carrying)
    )
      return "Telo nie je dostupne.";
    if (o.kind === "distract") {
      const reserved = append
        ? p.orders.filter((q) => q.kind === "distract").length
        : 0;
      if (p.charges - reserved <= 0) return "Ziadne kamene.";
      const from = append ? (p.orders.at(-1)?.position ?? p) : p;
      if (
        distance(from, o.position) > 8 ||
        !this.nav.clear(from, o.position, true)
      )
        return "Hod je prilis daleko alebo blokovany.";
    } else if (
      !this.nav.path(
        append ? (p.orders.at(-1)?.position ?? p) : p,
        target ?? o.position,
      )
    )
      return "Ciel nema pristupnu cestu. Plan zostal zachovany.";
    return null;
  }
  emit(position: Point, radius: number): void {
    this.noises.push({ position: { ...position }, radius, age: 0 });
    for (const g of this.guards)
      if (
        g.health > 0 &&
        g.alert !== "combat" &&
        distance(g, position) < radius &&
        (this.nav.clear(g, position, true) ||
          distance(g, position) < Math.min(radius, 5))
      ) {
        g.alert = "investigate";
        g.lastKnown = { ...position };
        g.search = 0;
        g.path = [];
        g.repath = 0;
        g.wait = 0;
      }
  }
  drop(): void {
    const p = this.party[this.active],
      body = this.guards.find((g) => g.id === p.carrying);
    if (this.menu || this.outcome || !body) return;
    body.x = p.x;
    body.y = p.y;
    body.carriedBy = undefined;
    body.hidden = distance(p, this.mission.hide) < 2;
    p.carrying = undefined;
    this.message = body.hidden ? "Telo ukryte." : "Telo polozene.";
  }
  tick(dt: number): void {
    if (
      this.paused ||
      this.menu ||
      this.outcome ||
      !Number.isFinite(dt) ||
      dt <= 0
    )
      return;
    // Bound resumption after a background tab; never run a hidden catch-up turn.
    let remaining = Math.min(dt, 0.25);
    while (remaining > 1e-8 && !this.outcome) {
      const step = Math.min(1 / 60, remaining);
      this.step(step);
      remaining -= step;
    }
  }
  private step(dt: number): void {
    this.elapsed += dt;
    for (const p of this.party) {
      p.cooldown = Math.max(0, p.cooldown - dt);
      p.moving = false;
      this.execute(p, dt);
      const body = this.guards.find((g) => g.id === p.carrying);
      if (body) {
        body.x = p.x;
        body.y = p.y;
      }
    }
    for (const g of this.guards) if (g.health > 0) this.enemy(g, dt);
    this.noises.forEach((n) => (n.age += dt));
    for (let i = this.noises.length - 1; i >= 0; i--)
      if (this.noises[i].age > 1.5) this.noises.splice(i, 1);
    if (this.party.some((p) => p.health <= 0)) this.outcome = "lost";
    else if (
      this.objectiveDone &&
      this.party.every((p) => distance(p, this.mission.exit) < 1.6)
    )
      this.outcome = "won";
    if (this.outcome)
      this.party.forEach((p) => {
        p.orders = [];
        p.path = [];
        p.moving = false;
        p.started = false;
      });
  }
  private move(p: Unit, dt: number, speed: number): boolean {
    p.moving = false;
    let left = dt * speed;
    while (p.path.length && left > 0) {
      const to = p.path[0],
        d = distance(p, to),
        step = Math.min(left, d);
      if (d < 0.01) {
        p.x = to.x;
        p.y = to.y;
        p.path.shift();
        continue;
      }
      const next = {
        x: p.x + ((to.x - p.x) * step) / d,
        y: p.y + ((to.y - p.y) * step) / d,
      };
      if (!this.nav.clear(p, next)) {
        p.path = [];
        p.started = false;
        return false;
      }
      p.facing = angleTo(p, to);
      p.x = next.x;
      p.y = next.y;
      p.moving = true;
      left -= step;
      if (step === d) p.path.shift();
    }
    return !p.path.length;
  }
  private finish(p: Unit, message?: string): void {
    p.orders.shift();
    p.path = [];
    p.started = false;
    p.repath = 0;
    if (message) this.message = p.name + ": " + message;
  }
  private execute(p: Unit, dt: number): void {
    const o = p.orders[0];
    if (!o || p.health <= 0) return;
    if (o.kind === "distract") {
      if (p.cooldown > 0) return;
      if (
        distance(p, o.position) <= 8 &&
        this.nav.clear(p, o.position, true) &&
        p.charges > 0
      ) {
        p.charges--;
        p.cooldown = 6;
        this.emit(o.position, 12);
        this.finish(p, "Odlakanie.");
        return;
      }
      this.finish(p, "Hod nie je dostupny.");
      return;
    }
    const target = this.units.find((u) => u.id === o.target);
    if (
      (o.kind === "takedown" && (!target || target.health <= 0)) ||
      (o.kind === "carry" &&
        (!target || target.health > 0 || target.hidden || target.carriedBy))
    ) {
      this.finish(p, "Ciel uz nie je dostupny.");
      return;
    }
    p.repath -= dt;
    if (
      !p.started ||
      (target && p.repath <= 0 && distance(o.position, target) > 0.5)
    ) {
      const destination = target ?? o.position,
        path = this.nav.path(p, destination);
      if (!path) {
        this.finish(p, "Cesta uz nie je dostupna.");
        return;
      }
      o.position = { x: destination.x, y: destination.y };
      p.path = path;
      p.started = true;
      p.repath = 0.4;
      if (o.run) p.stance = "run";
    }
    const arrived =
      target && distance(p, target) <= 0.95 && this.nav.clear(p, target, true)
        ? true
        : this.move(p, dt, SPEED[p.stance] * (p.carrying ? 0.55 : 1));
    if (p.moving) {
      const range =
        p.stance === "run"
          ? 11
          : p.stance === "walk"
            ? 2.6
            : p.stance === "crouch"
              ? 0.6
              : 0.3;
      // Footsteps are continuous hearing, not per-click/turn events.
      for (const g of this.guards)
        if (
          g.health > 0 &&
          g.alert === "patrol" &&
          distance(g, p) < range &&
          this.nav.clear(g, p, true)
        ) {
          g.alert = "investigate";
          g.lastKnown = { x: p.x, y: p.y };
          g.path = [];
          g.repath = 0;
        }
    }
    if (!arrived) return;
    if (target && distance(p, target) > 1.1) {
      p.started = false;
      return;
    }
    if (o.kind === "takedown" && target) {
      if (p.cooldown > 0) return;
      const behind =
        Math.cos(angleDelta(target.facing, angleTo(target, p))) < -0.25;
      if (behind && this.nav.clear(p, target, true)) {
        target.health = 0;
        target.path = [];
        p.cooldown = 3;
        this.finish(p, "Hliadka zneskodnena.");
      } else this.finish(p, "Priblizte sa odzadu.");
    } else if (o.kind === "carry" && target) {
      target.carriedBy = p.id;
      p.carrying = target.id;
      this.finish(p, "Telo zdvihnute.");
    } else if (o.kind === "interact") {
      this.interact(p, o.position);
      this.finish(p);
    } else this.finish(p);
  }
  private eastBank(p: Point): boolean {
    return (
      !!this.mission.river &&
      p.x > this.nav.riverX(p.y) + this.mission.river.width / 2 + 0.35
    );
  }
  private interact(p: Unit, position: Point): void {
    if (distance(p, position) > 1.2) {
      this.message = "Priblizte sa k cielu.";
      return;
    }
    if (distance(position, this.mission.supply) < 0.7) {
      this.hasSupply = true;
      if (this.mission.goal === "documents") this.objectiveDone = true;
      this.message =
        this.mission.goal === "documents"
          ? "Dokumenty ziskane."
          : "TNT ziskane.";
    } else if (
      distance(position, this.mission.target) < 0.7 &&
      this.hasSupply
    ) {
      if (
        this.mission.goal === "bridge" &&
        !this.party.every((u) => this.eastBank(u))
      ) {
        this.message = "Najprv presunte oboch clenov na vychodny breh.";
        return;
      }
      this.objectiveDone = true;
      if (this.mission.goal === "bridge") {
        this.nav.bridgeDestroyed = true;
        this.nav.rebuild();
      }
      this.emit(position, 18);
      this.message = "Sabotaz dokoncena. Ustupte!";
    } else if (distance(position, this.mission.hide) < 0.7 && p.carrying)
      this.drop();
    else this.message = "Interakcia nie je dostupna.";
  }
  private enemy(g: Unit, dt: number): void {
    g.moving = false;
    const seen = this.party
      .filter((p) => p.health > 0 && this.visible(g, p))
      .sort(
        (a, b) =>
          VISIBILITY[b.stance] * (this.cover(b) ? 0.3 : 1) -
          VISIBILITY[a.stance] * (this.cover(a) ? 0.3 : 1),
      )[0];
    if (seen) {
      const factor = VISIBILITY[seen.stance] * (this.cover(seen) ? 0.3 : 1),
        near = this.visible(g, seen) === "near";
      g.suspicion =
        near && factor >= 0.9
          ? 1
          : Math.min(1, g.suspicion + dt * (near ? 2.4 : 0.4) * factor);
      g.lastKnown = { x: seen.x, y: seen.y };
      g.search = 0;
      if (g.suspicion >= 1 && g.alert !== "combat") {
        g.alert = "combat";
        g.path = [];
        g.repath = 0;
        g.shot = 0.65;
        this.message = "Poplach!";
        this.guards
          .filter(
            (other) =>
              other !== g && other.health > 0 && distance(g, other) < 14,
          )
          .forEach((other) => {
            other.alert = "investigate";
            other.lastKnown = { ...g.lastKnown! };
            other.path = [];
            other.repath = 0;
          });
      }
    } else {
      g.suspicion = Math.max(0, g.suspicion - dt * 0.18);
      g.search += dt;
    }
    for (const body of this.guards)
      if (
        body.health <= 0 &&
        !body.hidden &&
        !body.carriedBy &&
        this.visible(g, body) &&
        g.alert === "patrol"
      ) {
        g.alert = "investigate";
        g.lastKnown = { x: body.x, y: body.y };
        g.path = [];
        g.repath = 0;
        g.search = 0;
      }
    if (g.alert !== "patrol" && g.lastKnown) {
      g.repath -= dt;
      if (g.repath <= 0) {
        g.path = this.nav.path(g, g.lastKnown) ?? [];
        g.repath = 0.5;
      }
      this.move(g, dt, g.alert === "combat" ? 2.5 : 1.7);
      if (g.alert === "combat" && seen) {
        g.facing = angleTo(g, seen);
        g.shot -= dt;
        if (g.shot <= 0) {
          seen.health = Math.max(0, seen.health - 35);
          g.shot = 1.1;
        }
      }
      if (!seen && g.search > 6) {
        g.alert = "patrol";
        g.path = [];
        g.wait = 0;
        g.lastKnown = undefined;
      }
      return;
    }
    const stop = g.patrol[g.stop];
    if (distance(g, stop) > 0.12) {
      if (!g.path.length) g.path = this.nav.path(g, stop) ?? [];
      this.move(g, dt, 1.4);
      return;
    }
    const turn = angleDelta(g.facing, stop.facing);
    if (Math.abs(turn) > 0.025) {
      g.facing +=
        Math.sign(turn) * Math.min(Math.abs(turn), (dt * Math.PI) / 2);
      return;
    }
    g.wait += dt;
    if (g.wait >= stop.wait) {
      g.wait = 0;
      g.stop = (g.stop + 1) % g.patrol.length;
      g.path = [];
    }
  }
}

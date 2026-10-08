import { describe, expect, it } from "vitest";
import { readFileSync } from "node:fs";
import { Simulation } from "../src/rt/Simulation";
import { Navigation } from "../src/rt/Navigation";
import { MISSIONS } from "../src/rt/missions";
import { type Mission, type Point, distance } from "../src/rt/types";

function fixture(): Mission {
  return {
    id: 99,
    name: "Test",
    briefing: "",
    width: 30,
    height: 30,
    start: { x: 3, y: 24 },
    supply: { x: 4, y: 23 },
    target: { x: 7, y: 23 },
    exit: { x: 9, y: 23 },
    hide: { x: 3, y: 25 },
    goal: "command",
    props: [],
    patrols: [
      [
        { x: 15, y: 5, facing: 0, wait: 0.5 },
        { x: 20, y: 5, facing: 0, wait: 0.5 },
      ],
    ],
  };
}
function advance(s: Simulation, seconds: number, dt = 1 / 60): void {
  for (let left = seconds; left > 1e-8; left -= dt) s.tick(Math.min(dt, left));
}
function complete(s: Simulation, limit = 80): void {
  for (
    let i = 0;
    i < limit * 30 && s.party.some((p) => p.orders.length) && !s.outcome;
    i++
  )
    s.tick(1 / 30);
  expect(s.party.map((p) => p.orders.length)).toEqual([0, 0]);
}
describe("default entry point", () => {
  it("loads the realtime engine, not the legacy GO turn coordinator", () => {
    const entry = readFileSync(
      new URL("../src/App.tsx", import.meta.url),
      "utf8",
    );
    expect(entry).toContain("RealtimeApp");
    expect(entry).not.toContain("GoApp");
  });
});
describe("continuous world simulation", () => {
  it("starts running and patrols without player input", () => {
    const s = new Simulation(fixture()),
      x = s.guards[0].x;
    expect(s.paused).toBe(false);
    advance(s, 3);
    expect(s.elapsed).toBeCloseTo(3);
    expect(s.guards[0].x).toBeGreaterThan(x + 2);
    expect(s.party[0].x).toBe(3);
  });
  it("a single move travels continuously through fractional coordinates", () => {
    const s = new Simulation(fixture());
    expect(s.command("move", { x: 12, y: 24 })).toBe(true);
    advance(s, 0.37);
    expect(s.party[0].x).toBeCloseTo(3 + 0.37 * 1.9, 5);
    advance(s, 2);
    expect(s.party[0].x).toBeGreaterThan(7);
    expect(s.party[0].orders).toHaveLength(1);
    advance(s, 4);
    expect(s.party[0].x).toBeCloseTo(12);
    expect(s.party[0].orders).toHaveLength(0);
    expect(s.paused).toBe(false);
  });
  it("moves both selected characters simultaneously", () => {
    const s = new Simulation(fixture()),
      before = s.party.map((p) => ({ ...p }));
    s.selectAll();
    expect(s.command("move", { x: 10, y: 24 })).toBe(true);
    advance(s, 1);
    s.party.forEach((p, i) =>
      expect(distance(p, before[i])).toBeCloseTo(1.9, 2),
    );
  });
  it("is stable across render frame rates", () => {
    const a = new Simulation(fixture()),
      b = new Simulation(fixture());
    a.command("move", { x: 20, y: 24 });
    b.command("move", { x: 20, y: 24 });
    advance(a, 2, 1 / 30);
    advance(b, 2, 1 / 144);
    expect(a.party[0].x).toBeCloseTo(b.party[0].x, 5);
    expect(a.guards[0].x).toBeCloseTo(b.guards[0].x, 1);
  });
  it("only explicit pause freezes time, movement, AI and cooldowns", () => {
    const s = new Simulation(fixture());
    s.command("move", { x: 20, y: 24 });
    s.party[0].cooldown = 5;
    advance(s, 1);
    s.togglePause();
    const before = JSON.stringify(s.units),
      elapsed = s.elapsed;
    advance(s, 8);
    expect(JSON.stringify(s.units)).toBe(before);
    expect(s.elapsed).toBe(elapsed);
    s.togglePause();
    advance(s, 1);
    expect(s.elapsed).toBeCloseTo(2);
    expect(s.party[0].cooldown).toBeCloseTo(3);
  });
  it("menu freezes without changing the tactical pause state", () => {
    const s = new Simulation(fixture());
    advance(s, 1);
    s.menu = true;
    advance(s, 2);
    s.menu = false;
    expect(s.paused).toBe(false);
    advance(s, 1);
    expect(s.elapsed).toBeCloseTo(2);
    s.togglePause();
    s.menu = true;
    s.menu = false;
    advance(s, 1);
    expect(s.elapsed).toBeCloseTo(2);
  });
  it("never performs background catch-up turns", () => {
    const s = new Simulation(fixture());
    s.tick(60);
    expect(s.elapsed).toBeCloseTo(0.25);
    s.tick(NaN);
    s.tick(-1);
    expect(s.elapsed).toBeCloseTo(0.25);
  });
});
describe("orders and skills", () => {
  it("cycles the active specialist without dropping group selection", () => {
    const s = new Simulation(fixture());
    s.selectAll();
    s.next();
    expect(s.active).toBe(1);
    expect(s.party.every((p) => p.selected)).toBe(true);
    s.select(0);
    s.next();
    expect(s.active).toBe(1);
    expect(s.party[0].selected).toBe(false);
  });
  it("previews from the current or appended origin without changing the plan", () => {
    const s = new Simulation(fixture());
    s.command("move", { x: 7, y: 24 });
    expect(s.preview({ x: 9, y: 24 })?.[0]).toEqual({ x: 3, y: 24 });
    expect(s.preview({ x: 9, y: 24 }, true)?.[0]).toEqual({ x: 7, y: 24 });
    s.togglePause();
    expect(s.preview({ x: 9, y: 24 })?.[0]).toEqual({ x: 7, y: 24 });
    expect(s.party[0].orders).toHaveLength(1);
    expect(s.preview({ x: -2, y: 24 })).toBeNull();
  });
  it("replacing queued distractions releases their unspent reservations", () => {
    const s = new Simulation(fixture());
    s.togglePause();
    for (let i = 0; i < 5; i++)
      expect(s.command("distract", { x: 5, y: 24 })).toBe(true);
    s.togglePause();
    expect(s.command("distract", { x: 6, y: 24 })).toBe(true);
    expect(s.party[0].orders).toHaveLength(1);
    expect(s.party[0].charges).toBe(5);
  });
  it("queues while paused and replaces a running plan only after validation", () => {
    const s = new Simulation(fixture());
    s.togglePause();
    s.command("move", { x: 6, y: 24 });
    s.command("move", { x: 10, y: 24 });
    expect(s.party[0].orders).toHaveLength(2);
    expect(s.party[0].x).toBe(3);
    expect(s.command("move", { x: -20, y: 0 })).toBe(false);
    expect(s.party[0].orders).toHaveLength(2);
    expect(s.undo()).toBe(true);
    expect(s.party[0].orders).toHaveLength(1);
    s.togglePause();
    s.command("move", { x: 8, y: 24 });
    expect(s.party[0].orders).toHaveLength(1);
    s.command("move", { x: 10, y: 24 }, undefined, true);
    expect(s.party[0].orders).toHaveLength(2);
    complete(s);
    expect(s.party[0].x).toBeCloseTo(10);
  });
  it("enforces bounded queues, cancellation and no turn rewind", () => {
    const s = new Simulation(fixture());
    s.togglePause();
    for (let i = 0; i < 32; i++)
      expect(s.command("move", { x: 5, y: 24 })).toBe(true);
    expect(s.command("move", { x: 5, y: 24 })).toBe(false);
    s.stop();
    expect(s.party[0].orders).toHaveLength(0);
    s.togglePause();
    expect(s.undo()).toBe(false);
  });
  it("runs cooldown and ammunition in seconds, not commands", () => {
    const s = new Simulation(fixture());
    s.command("distract", { x: 5, y: 24 });
    s.command("distract", { x: 6, y: 24 }, undefined, true);
    advance(s, 0.1);
    expect(s.party[0].charges).toBe(4);
    expect(s.party[0].orders).toHaveLength(1);
    advance(s, 5);
    expect(s.party[0].charges).toBe(4);
    advance(s, 1.2);
    expect(s.party[0].charges).toBe(3);
  });
  it("takedown stops behind the target, then carries and hides the body", () => {
    const m = fixture();
    m.start = { x: 12, y: 5 };
    m.hide = { x: 12, y: 6 };
    m.patrols[0] = [{ x: 15, y: 5, facing: 0, wait: 100 }];
    const s = new Simulation(m);
    s.stance("crouch");
    expect(s.command("takedown", s.guards[0], s.guards[0].id)).toBe(true);
    complete(s);
    expect(s.guards[0].health).toBe(0);
    expect(s.party[0].x).toBeLessThan(15);
    expect(s.command("carry", s.guards[0], s.guards[0].id)).toBe(true);
    complete(s);
    expect(s.party[0].carrying).toBe(s.guards[0].id);
    s.command("move", m.hide);
    complete(s);
    s.drop();
    expect(s.guards[0].hidden).toBe(true);
  });
});
describe("continuous detection", () => {
  it("uses facing, range and line of sight, not tiles", () => {
    const m = fixture(),
      s = new Simulation(m),
      g = s.guards[0];
    expect(s.visible(g, { x: 17, y: 5 })).toBe("near");
    expect(s.visible(g, { x: 22, y: 5 })).toBe("far");
    expect(s.visible(g, { x: 13, y: 5 })).toBeNull();
    expect(s.visible(g, { x: 25.5, y: 5 })).toBeNull();
    m.props.push({ x: 18, y: 5, kind: "house1", width: 1, height: 2 });
    expect(s.visible(g, { x: 22, y: 5 })).toBeNull();
  });
  it("crouching slows far-zone suspicion and close standing triggers combat", () => {
    const m = fixture();
    m.start = { x: 22, y: 5 };
    m.patrols[0][0].wait = 100;
    const a = new Simulation(m),
      b = new Simulation(m);
    b.selectAll();
    b.stance("crouch");
    advance(a, 1);
    advance(b, 1);
    expect(a.guards[0].suspicion).toBeGreaterThan(b.guards[0].suspicion * 2);
    a.party[0].x = 17;
    advance(a, 0.02);
    expect(a.guards[0].alert).toBe("combat");
  });
  it("sound causes investigation without a player turn", () => {
    const s = new Simulation(fixture());
    s.emit({ x: 13, y: 8 }, 8);
    expect(s.guards[0].alert).toBe("investigate");
    const before = { ...s.guards[0] };
    advance(s, 1);
    expect(distance(s.guards[0], before)).toBeGreaterThan(1);
  });
});
describe("navigation and playable missions", () => {
  it("revalidates a route if the bridge disappears during movement", () => {
    const s = new Simulation({
      ...MISSIONS[0],
      start: { x: 8, y: 7.2 },
      patrols: [],
    });
    expect(s.command("move", { x: 13, y: 7.2 })).toBe(true);
    advance(s, 0.2);
    s.nav.bridgeDestroyed = true;
    s.nav.rebuild();
    advance(s, 5);
    expect(s.party[0].x).toBeLessThan(9.5);
    expect(s.party[0].orders).toHaveLength(0);
    expect(s.message).toContain("Cesta uz nie je dostupna");
  });
  for (const m of MISSIONS)
    it(m.name + " has reachable spawns, objectives and patrol routes", () => {
      const nav = new Navigation(m);
      const points = [
        m.supply,
        m.target,
        m.exit,
        m.hide,
        { x: m.start.x + 1.8, y: m.start.y },
      ];
      for (const p of points)
        expect(
          nav.path(m.start, p),
          "unreachable " + JSON.stringify(p),
        ).not.toBeNull();
      for (const route of m.patrols)
        for (let i = 0; i < route.length; i++)
          expect(
            nav.path(route[i], route[(i + 1) % route.length]),
          ).not.toBeNull();
    });
  it("river is impassable except across the bridge, destruction disconnects banks", () => {
    const s = new Simulation(MISSIONS[0]),
      a = { x: 9, y: 7.2 },
      b = { x: 13, y: 7.2 };
    expect(s.nav.clear(a, b)).toBe(true);
    expect(s.nav.clear({ x: 9, y: 12 }, { x: 13, y: 12 })).toBe(false);
    expect(s.nav.path(a, b)).not.toBeNull();
    s.nav.bridgeDestroyed = true;
    s.nav.rebuild();
    expect(s.nav.path(a, b)).toBeNull();
  });
  it("all mission objectives and extraction execute through production orders", () => {
    for (const mission of MISSIONS) {
      // Isolate objective sequencing here; active guard timing has separate tests.
      const s = new Simulation({ ...mission, patrols: [] });
      s.command("interact", mission.supply);
      complete(s, 120);
      expect(s.hasSupply).toBe(true);
      if (mission.goal === "bridge") {
        s.selectAll();
        s.command("move", { x: 13.5, y: 7.2 });
        complete(s, 120);
      }
      s.select(0);
      if (mission.goal !== "documents") {
        expect(s.command("interact", mission.target)).toBe(true);
        complete(s, 120);
      }
      expect(s.objectiveDone).toBe(true);
      s.selectAll();
      expect(s.command("move", mission.exit)).toBe(true);
      complete(s, 120);
      advance(s, 0.1);
      expect(s.outcome).toBe("won");
    }
  });
  it("mission 1 is winnable with active guards, no teleports or invulnerability", () => {
    const s = new Simulation(MISSIONS[0]);
    s.stance("crouch");
    expect(s.command("takedown", s.guards[0], s.guards[0].id)).toBe(true);
    complete(s, 40);
    expect(s.guards[0].health).toBe(0);
    s.command("carry", s.guards[0], s.guards[0].id);
    complete(s);
    s.command("move", s.mission.hide);
    complete(s);
    s.drop();
    expect(s.guards[0].hidden).toBe(true);
    s.command("interact", s.mission.supply);
    complete(s);
    s.selectAll();
    s.stance("crouch");
    s.command("move", { x: 13.5, y: 7.2 });
    complete(s, 120);
    s.select(0);
    s.command("interact", s.mission.target);
    complete(s);
    s.selectAll();
    s.command("move", s.mission.exit);
    complete(s, 120);
    advance(s, 0.1);
    expect(s.outcome).toBe("won");
    expect(s.party.every((p) => p.health > 0)).toBe(true);
  });
});

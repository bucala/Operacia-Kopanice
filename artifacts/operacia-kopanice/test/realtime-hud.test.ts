import { describe, expect, it, vi } from "vitest";
import { Simulation } from "../src/rt/Simulation";
import { MISSIONS } from "../src/rt/missions";
import { Renderer } from "../src/rt/Renderer";
import {
  bodyHeight,
  contains,
  intersects,
  layoutPartyMarkers,
} from "../src/rt/PartyMarkers";
import { orderName, orderPage, orderState } from "../src/rt/OrderDisplay";

const project = () => ({ x: 300, y: 300 });
function markerFixture() {
  const sim = new Simulation(MISSIONS[0]);
  const renderer = Object.assign(Object.create(Renderer.prototype), {
    sim,
    width: 600,
    height: 480,
    scale: 28,
    yaw: 0,
    center: { x: 12.5, y: 10 },
    markers: true,
    markerExclusions: [],
  }) as Renderer;
  return { sim, renderer };
}
describe("Friendly selection markers", () => {
  it("does not inherit wide road strokes on individual stone outlines", () => {
    const { renderer } = markerFixture();
    const widths: number[] = [];
    const ctx = {
      lineWidth: 90,
      beginPath: () => {},
      moveTo: () => {},
      lineTo: () => {},
      closePath: () => {},
      fill: () => {},
      stroke: () => {
        widths.push(ctx.lineWidth);
      },
    };
    (
      renderer as unknown as {
        polygon: (
          ctx: unknown,
          p: unknown[],
          fill: string,
          stroke: string,
        ) => void;
      }
    ).polygon(
      ctx,
      [
        { x: 1, y: 1 },
        { x: 2, y: 1 },
        { x: 2, y: 2 },
      ],
      "#b5bec1",
      "#d7e0e4",
    );
    expect(widths).toEqual([1]);
  });
  it("separates overlapping units with stable 40px hit regions", () => {
    const { sim } = markerFixture();
    const markers = layoutPartyMarkers(sim.party, project, 28, 600, 480);
    expect(markers).toHaveLength(2);
    expect(intersects(markers[0].bounds, markers[1].bounds)).toBe(false);
    expect(markers.every((m) => m.bounds.right - m.bounds.left === 40)).toBe(
      true,
    );
    expect(markers.map((m) => m.number)).toEqual([1, 2]);
    expect(layoutPartyMarkers(sim.party, project, 28, 600, 480)).toEqual(
      markers,
    );
  });
  it("avoids HUD panels and suppresses impossible or offscreen placements", () => {
    const { sim } = markerFixture();
    const blocked = { left: 275, right: 325, top: 190, bottom: 245 };
    const markers = layoutPartyMarkers(sim.party, project, 28, 600, 480, [
      blocked,
    ]);
    expect(markers).toHaveLength(2);
    expect(markers.every((m) => !intersects(m.bounds, blocked))).toBe(true);
    expect(
      layoutPartyMarkers(sim.party, project, 28, 600, 480, [
        { left: 0, right: 600, top: 0, bottom: 480 },
      ]),
    ).toHaveLength(0);
    expect(
      layoutPartyMarkers(sim.party, () => ({ x: -1, y: 300 }), 28, 600, 480),
    ).toHaveLength(0);
    expect(layoutPartyMarkers(sim.party, project, 28, 20, 20)).toHaveLength(0);
  });
  it("does not expose enemies, dead, hidden or carried units", () => {
    const { sim } = markerFixture();
    expect(layoutPartyMarkers(sim.guards, project, 28, 600, 480)).toHaveLength(
      0,
    );
    sim.party[0].health = 0;
    sim.party[1].hidden = true;
    expect(layoutPartyMarkers(sim.party, project, 28, 600, 480)).toHaveLength(
      0,
    );
    sim.party[0].health = 100;
    sim.party[0].carriedBy = "officer";
    expect(layoutPartyMarkers(sim.party, project, 28, 600, 480)).toHaveLength(
      0,
    );
  });
  it("picks exactly the drawn marker after pan, rotation, zoom and stance changes", () => {
    const { renderer, sim } = markerFixture();
    for (const scale of [9, 28, 110])
      for (const yaw of [0, Math.PI / 4, Math.PI]) {
        renderer.scale = scale;
        renderer.yaw = yaw;
        renderer.center = { x: sim.party[0].x, y: sim.party[0].y };
        for (const stance of ["walk", "crouch", "prone"] as const) {
          sim.party[0].stance = stance;
          const m = renderer.partyMarkers.find((m) => m.unit === sim.party[0])!;
          expect(m).toBeDefined();
          expect(renderer.pickPartyMarker(m.center)).toBe(sim.party[0]);
          expect(contains(m.bounds, m.center)).toBe(true);
          expect(m.anchor.y).toBe(
            renderer.project(sim.party[0]).y - bodyHeight(sim.party[0], scale),
          );
        }
      }
    renderer.markers = false;
    expect(renderer.partyMarkers).toHaveLength(0);
    renderer.markers = true;
    sim.menu = true;
    expect(renderer.partyMarkers).toHaveLength(0);
    sim.menu = false;
    sim.outcome = "won";
    expect(renderer.partyMarkers).toHaveLength(0);
  });
  it("draws marker handles after the entire scenery and objective passes", () => {
    const { renderer } = markerFixture();
    const calls: string[] = [];
    const ctx = new Proxy(
      {},
      { get: (_, key) => (key === "canvas" ? {} : () => {}) },
    );
    Object.assign(renderer, {
      canvas: { width: 600, height: 480, getContext: () => ctx },
      ground: { getContext: () => ctx },
      cones: false,
      paths: false,
      terrain: () => calls.push("terrain"),
      prop: () => calls.push("scenery"),
      person: () => calls.push("person"),
      ring: () => calls.push("objective"),
      drawPartyMarkers: vi.fn(() => calls.push("markers")),
    });
    renderer.draw();
    expect(calls).toContain("scenery");
    expect(calls).toContain("person");
    expect(calls.at(-1)).toBe("markers");
  });
});
describe("Real-time order overview", () => {
  it("names movement, abilities and mission-specific interactions", () => {
    const mission = MISSIONS[0];
    expect(orderName({ kind: "move", position: mission.start }, mission)).toBe(
      "Presun",
    );
    expect(
      orderName({ kind: "move", run: true, position: mission.start }, mission),
    ).toBe("Beh");
    expect(
      orderName({ kind: "interact", position: mission.supply }, mission),
    ).toBe("Ziskat TNT");
    expect(
      orderName({ kind: "interact", position: mission.target }, mission),
    ).toBe("Odpalit most");
    expect(
      orderName({ kind: "interact", position: mission.hide }, mission),
    ).toBe("Ukryt telo");
    expect(
      orderName(
        { kind: "interact", position: MISSIONS[1].target },
        MISSIONS[1],
      ),
    ).toBe("Ziskat dokumenty");
    expect(
      orderName(
        { kind: "interact", position: MISSIONS[2].target },
        MISSIONS[2],
      ),
    ).toBe("Sabotaz velitelstva");
    for (const kind of ["takedown", "carry", "distract"] as const)
      expect(
        orderName({ kind, position: mission.start }, mission).length,
      ).toBeGreaterThan(3);
  });
  it("reports only an actually started head as running or suspended", () => {
    const { sim } = markerFixture();
    const unit = sim.party[0];
    expect(orderState(unit, 0, false)).toBe("Caka");
    unit.started = true;
    expect(orderState(unit, 0, false)).toBe("Vykonava sa");
    expect(orderState(unit, 0, true)).toBe("Pozastavene");
    expect(orderState(unit, 1, false)).toBe("Caka");
  });
  it("bounds five-slot pages for empty, full and shrinking plans", () => {
    expect(orderPage(0, 7)).toEqual({ page: 0, pages: 1, start: 0 });
    expect(orderPage(5, -1)).toEqual({ page: 0, pages: 1, start: 0 });
    expect(orderPage(32, 99)).toEqual({ page: 6, pages: 7, start: 30 });
    expect(orderPage(6, 1)).toEqual({ page: 1, pages: 2, start: 5 });
    expect(orderPage(5, 1)).toEqual({ page: 0, pages: 1, start: 0 });
  });
});

import { describe, expect, it, vi } from "vitest";
import { Pointer } from "../src/rt/Pointer";
import { type Renderer } from "../src/rt/Renderer";
import { Simulation } from "../src/rt/Simulation";
import { type Mission, type Point } from "../src/rt/types";

function setup() {
  const m: Mission = {
    id: 99,
    name: "Input",
    briefing: "",
    width: 40,
    height: 40,
    start: { x: 4, y: 30 },
    supply: { x: 3, y: 3 },
    target: { x: 4, y: 3 },
    exit: { x: 5, y: 3 },
    hide: { x: 6, y: 3 },
    goal: "command",
    props: [],
    patrols: [],
  };
  const sim = new Simulation(m),
    listeners = new Map<string, (e: unknown) => void>();
  const canvas = {
    addEventListener: (name: string, fn: (e: unknown) => void) =>
      listeners.set(name, fn),
    removeEventListener: (name: string) => listeners.delete(name),
    setPointerCapture: vi.fn(),
    getBoundingClientRect: () => ({ left: 0, top: 0 }),
  };
  const renderer = {
    canvas,
    sim,
    paths: false,
    project: (p: Point) => p,
    unproject: (p: Point) => p,
    pick: () => undefined,
    pan: vi.fn(),
    zoom: vi.fn(),
  } as unknown as Renderer;
  const input = new Pointer(renderer, vi.fn());
  const event = (
    name: string,
    x = 10,
    y = 30,
    button = 2,
    id = 1,
    touch = false,
    shift = false,
  ) =>
    listeners.get(name)?.({
      clientX: x,
      clientY: y,
      button,
      pointerId: id,
      pointerType: touch ? "touch" : "mouse",
      shiftKey: shift,
      preventDefault: vi.fn(),
    });
  const click = (x = 10, y = 30, button = 2, shift = false) => {
    event("pointerdown", x, y, button, 1, false, shift);
    event("pointerup", x, y, button, 1, false, shift);
  };
  return { sim, input, event, click, renderer, listeners };
}
describe("RTT pointer controls", () => {
  it("left ground click never moves; right click sends a full destination", () => {
    const { sim, click } = setup();
    click(10, 30, 0);
    expect(sim.party[0].orders).toHaveLength(0);
    click(10, 30);
    expect(sim.party[0].orders[0].position).toEqual({ x: 10, y: 30 });
  });
  it("shift appends and double-click upgrades instead of duplicating paused orders", () => {
    const { sim, click } = setup();
    sim.togglePause();
    click(10, 30);
    click(10, 30);
    expect(sim.party[0].orders).toHaveLength(1);
    expect(sim.party[0].orders[0].run).toBe(true);
    expect(sim.party[0].stance).toBe("walk");
    click(12, 30, 2, true);
    expect(sim.party[0].orders).toHaveLength(2);
  });
  it("rejected double-click preserves a plan and its stance", () => {
    const { sim, click } = setup();
    click(10, 30);
    click(-10, 30);
    click(-10, 30);
    expect(sim.party[0].orders).toHaveLength(1);
    expect(sim.party[0].orders[0].position.x).toBe(10);
    expect(sim.party[0].stance).toBe("walk");
  });
  it("right click cancels armed skills without an accidental move or resource cost", () => {
    const { sim, click, input } = setup();
    input.action = "distract";
    click(7, 30);
    expect(input.action).toBe("move");
    expect(sim.party[0].orders).toHaveLength(0);
    expect(sim.party[0].charges).toBe(5);
  });
  it("touch tap issues a continuous move, touch drag only pans", () => {
    const { sim, event, renderer } = setup();
    event("pointerdown", 10, 30, 0, 1, true);
    event("pointerup", 10, 30, 0, 1, true);
    expect(sim.party[0].orders).toHaveLength(1);
    sim.stop();
    event("pointerdown", 10, 30, 0, 1, true);
    event("pointermove", 25, 30, 0, 1, true);
    event("pointerup", 25, 30, 0, 1, true);
    expect(renderer.pan).toHaveBeenCalled();
    expect(sim.party[0].orders).toHaveLength(0);
  });
  it("pinch release and pointer cancellation cannot issue orders", () => {
    const { sim, event, renderer } = setup();
    event("pointerdown", 10, 30, 0, 1, true);
    event("pointerdown", 20, 30, 0, 2, true);
    event("pointermove", 24, 30, 0, 2, true);
    event("pointerup", 24, 30, 0, 2, true);
    event("pointerup", 10, 30, 0, 1, true);
    expect(renderer.zoom).toHaveBeenCalled();
    expect(sim.party[0].orders).toHaveLength(0);
    event("pointerdown");
    event("pointercancel");
    event("pointerup");
    expect(sim.party[0].orders).toHaveLength(0);
  });
  it("menu blocks input and destroy removes every canvas listener", () => {
    const { sim, click, input, listeners } = setup();
    sim.menu = true;
    click();
    expect(sim.party[0].orders).toHaveLength(0);
    input.destroy();
    expect(listeners.size).toBe(0);
  });
});

import { useEffect, useRef, useState, type ReactNode } from "react";
import {
  Pause,
  Play,
  Square,
  Hand,
  Swords,
  CircleDot,
  Backpack,
  Footprints,
  PersonStanding,
  ArrowDown,
  ChevronsDown,
  Eye,
  Menu,
  RotateCcw,
  RotateCw,
  Focus,
  Plus,
  Minus,
  X,
  Undo2,
  Users,
  Route,
} from "lucide-react";
import { Simulation } from "./Simulation";
import { Renderer } from "./Renderer";
import { Pointer } from "./Pointer";
import { MISSIONS } from "./missions";
import { type Action, type Point } from "./types";
import "./realtime.css";

type Page = "main" | "missions" | "pause" | "options" | null;
type Settings = { cones: boolean; paths: boolean; cameraSpeed: number };
function readSettings(): Settings {
  try {
    const s = JSON.parse(localStorage.getItem("kopanice-rtt-settings") ?? "{}");
    return {
      cones: s.cones !== false,
      paths: s.paths !== false,
      cameraSpeed: Math.min(2, Math.max(0.5, Number(s.cameraSpeed) || 1)),
    };
  } catch {
    return { cones: true, paths: true, cameraSpeed: 1 };
  }
}
const time = (seconds: number) =>
  Math.floor(seconds / 60) +
  ":" +
  String(Math.floor(seconds % 60)).padStart(2, "0");
function Tool({
  label,
  active = false,
  disabled = false,
  onClick,
  children,
}: {
  label: string;
  active?: boolean;
  disabled?: boolean;
  onClick: () => void;
  children: ReactNode;
}) {
  return (
    <button
      type="button"
      className={"rt-tool" + (active ? " is-active" : "")}
      title={label}
      aria-label={label}
      aria-pressed={active}
      disabled={disabled}
      onClick={onClick}
    >
      {children}
    </button>
  );
}
export default function RealtimeApp() {
  const canvas = useRef<HTMLCanvasElement>(null);
  const [initial] = useState(() => new Simulation(MISSIONS[0]));
  const sim = useRef(initial),
    hasDeployed = useRef(false);
  const renderer = useRef<Renderer | null>(null),
    pointer = useRef<Pointer | null>(null);
  const [page, setPage] = useState<Page>("main"),
    [mission, setMission] = useState(0),
    [deployed, setDeployed] = useState(false);
  const [settings, setSettings] = useState(readSettings),
    [action, setAction] = useState<Action>("move");
  const [, refresh] = useState(0),
    pageRef = useRef<Page>("main"),
    settingsRef = useRef(settings);
  const update = () => refresh((n) => n + 1);
  function menu(next: Page) {
    pointer.current?.reset();
    sim.current.menu = next !== null;
    pageRef.current = next;
    setPage(next);
    setAction("move");
    if (pointer.current) pointer.current.action = "move";
    update();
  }
  function deploy(id: number) {
    sim.current = new Simulation(MISSIONS[id]);
    if (renderer.current) {
      renderer.current.sim = sim.current;
      renderer.current.fit();
    }
    hasDeployed.current = true;
    setMission(id);
    setDeployed(true);
    menu(null);
  }
  function arm(a: Action) {
    const next = action === a ? "move" : a;
    setAction(next);
    if (pointer.current) pointer.current.action = next;
  }
  function changeSettings(next: Settings) {
    setSettings(next);
    settingsRef.current = next;
    try {
      localStorage.setItem("kopanice-rtt-settings", JSON.stringify(next));
    } catch {
      /* Storage can be disabled. */
    }
  }
  useEffect(() => {
    if (!canvas.current) return;
    const r = new Renderer(canvas.current, sim.current);
    renderer.current = r;
    sim.current.menu = true;
    const p = new Pointer(r, () => {
      setAction(p.action);
      update();
    });
    pointer.current = p;
    const resize = () => {
      r.resize(innerWidth, innerHeight);
    };
    resize();
    r.fit();
    const keys = new Set<string>();
    const keydown = (e: KeyboardEvent) => {
      if ((e.target as HTMLElement)?.matches("input,select,textarea")) return;
      const s = sim.current;
      if (e.key === "Escape") {
        e.preventDefault();
        if (p.action !== "move") {
          p.action = "move";
          setAction("move");
        } else
          menu(
            pageRef.current ? (hasDeployed.current ? null : "main") : "pause",
          );
        return;
      }
      if (pageRef.current || s.outcome) return;
      if (
        [
          " ",
          "ArrowUp",
          "ArrowDown",
          "ArrowLeft",
          "ArrowRight",
          "Backspace",
          "Tab",
        ].includes(e.key) ||
        (e.ctrlKey && e.key === "a")
      )
        e.preventDefault();
      keys.add(e.key);
      if (e.repeat) return;
      if (e.code === "Space") s.togglePause();
      else if (e.key === "1" || e.key === "2")
        s.select(Number(e.key) - 1, e.shiftKey);
      else if (e.key === "Tab") s.next();
      else if (e.key === "3" || (e.ctrlKey && e.key.toLowerCase() === "a"))
        s.selectAll();
      else if (["s", "x"].includes(e.key.toLowerCase())) s.stop();
      else if (e.key.toLowerCase() === "w") s.stance("walk");
      else if (e.key.toLowerCase() === "r") s.stance("run");
      else if (e.key.toLowerCase() === "c") s.stance("crouch");
      else if (e.key.toLowerCase() === "v") s.stance("prone");
      else if (e.key === "Backspace") s.undo();
      else if (e.key.toLowerCase() === "q") r.yaw -= Math.PI / 4;
      else if (e.key === "]") r.yaw += Math.PI / 4;
      else if (e.key === "Home") r.focus();
      else if (["t", "f", "b", "e"].includes(e.key.toLowerCase())) {
        const target: Record<string, Action> = {
          t: "takedown",
          f: "distract",
          b: "carry",
          e: "interact",
        };
        if (e.key.toLowerCase() === "b" && s.party[s.active].carrying) s.drop();
        else {
          p.action = target[e.key.toLowerCase()];
          setAction(p.action);
        }
      }
      update();
    };
    const keyup = (e: KeyboardEvent) => keys.delete(e.key);
    const suspend = () => {
      p.reset();
      keys.clear();
      if (!pageRef.current && !sim.current.outcome) {
        sim.current.paused = true;
        sim.current.message = "Takticka pauza.";
        update();
      }
    };
    const visibility = () => {
      if (document.hidden) suspend();
    };
    window.addEventListener("resize", resize);
    window.addEventListener("keydown", keydown);
    window.addEventListener("keyup", keyup);
    window.addEventListener("blur", suspend);
    document.addEventListener("visibilitychange", visibility);
    let last = performance.now(),
      hud = last,
      frame = 0;
    const loop = (now: number) => {
      const dt = Math.min(0.1, (now - last) / 1000);
      last = now;
      const options = settingsRef.current;
      r.cones = options.cones;
      r.paths = options.paths;
      if (!pageRef.current) {
        const amount = dt * 450 * options.cameraSpeed;
        r.pan(
          (keys.has("ArrowLeft") ? amount : 0) -
            (keys.has("ArrowRight") ? amount : 0),
          (keys.has("ArrowUp") ? amount : 0) -
            (keys.has("ArrowDown") ? amount : 0),
        );
      }
      sim.current.tick(dt);
      r.draw();
      if (now - hud > 100) {
        update();
        hud = now;
      }
      frame = requestAnimationFrame(loop);
    };
    frame = requestAnimationFrame(loop);
    // Read-only diagnostics for browser tests; absent in production bundles.
    if (import.meta.env.DEV)
      Object.defineProperty(window, "__kopanice", {
        configurable: true,
        value: {
          snapshot: () => ({
            elapsed: sim.current.elapsed,
            paused: sim.current.paused,
            menu: sim.current.menu,
            party: sim.current.party.map((u) => ({
              x: u.x,
              y: u.y,
              orders: u.orders.length,
              health: u.health,
            })),
            guards: sim.current.guards.map((u) => ({
              x: u.x,
              y: u.y,
              facing: u.facing,
              alert: u.alert,
            })),
            outcome: sim.current.outcome,
          }),
          project: (point: Point) => r.project(point),
        },
      });
    return () => {
      cancelAnimationFrame(frame);
      p.destroy();
      window.removeEventListener("resize", resize);
      window.removeEventListener("keydown", keydown);
      window.removeEventListener("keyup", keyup);
      window.removeEventListener("blur", suspend);
      document.removeEventListener("visibilitychange", visibility);
      if (import.meta.env.DEV)
        delete (window as unknown as Record<string, unknown>).__kopanice;
    };
  }, []);
  const s = sim.current,
    active = s.party[s.active],
    r = renderer.current;
  const use = (fn: () => void) => () => {
    fn();
    update();
  };
  return (
    <main
      id="app"
      className="rt-app"
      data-mode="real-time"
      data-elapsed={s.elapsed.toFixed(3)}
      data-paused={s.paused}
      data-menu={s.menu}
      data-state={
        s.outcome ?? (s.menu ? "menu" : s.paused ? "paused" : "running")
      }
    >
      <canvas id="game" ref={canvas} aria-label="Takticke bojisko" />
      <div className="rt-hud">
        <section className="rt-party" aria-label="Tim">
          {s.party.map((u, i) => (
            <button
              key={u.id}
              className={"rt-unit" + (u.selected ? " is-active" : "")}
              disabled={!!page || !!s.outcome}
              onClick={(e) => {
                s.select(i, e.shiftKey);
                update();
              }}
              aria-label={"Vybrat " + u.name}
              data-world-x={u.x.toFixed(3)}
              data-world-y={u.y.toFixed(3)}
              data-orders={u.orders.length}
              aria-pressed={u.selected}
            >
              <img
                src={
                  "/assets/sprites/" + (i ? "guard-officer" : "player") + ".png"
                }
                alt=""
              />
              <span>{u.name}</span>
              <meter
                min="0"
                max="100"
                value={u.health}
                aria-label={u.name + " zdravie"}
              />
              <small>
                {i + 1} ·{" "}
                {u.orders.length > 0
                  ? u.orders.length + " rozkazov"
                  : u.carrying
                    ? "Nesie telo"
                    : "Pripraveny"}
              </small>
            </button>
          ))}
        </section>
        <section className="rt-objective">
          <strong>{s.objective}</strong>
          <div>
            <span data-testid="realtime-state">
              {s.paused ? "TAKTICKA PAUZA" : "REALNY CAS"}
            </span>
            <time data-testid="mission-clock">{time(s.elapsed)}</time>
          </div>
        </section>
        <div className="rt-message" role="status">
          {s.message}
        </div>
        {!page && !s.outcome && (
          <section className="rt-controls" aria-label="Rozkazy">
            <div className="rt-action-info">
              {active.name}{" "}
              <span>
                {active.charges} ·{" "}
                {active.cooldown > 0
                  ? active.cooldown.toFixed(1) + " s"
                  : "Pripraveny"}
              </span>
            </div>
            <div className="rt-tools">
              <Tool
                label="Takticka pauza (Space)"
                active={s.paused}
                onClick={use(() => s.togglePause())}
              >
                {s.paused ? <Play /> : <Pause />}
              </Tool>
              <Tool label="Zastavit (S)" onClick={use(() => s.stop())}>
                <Square />
              </Tool>
              <Tool label="Vybrat tim" onClick={use(() => s.selectAll())}>
                <Users />
              </Tool>
              <Tool
                label="Interakcia"
                active={action === "interact"}
                onClick={() => arm("interact")}
              >
                <Hand />
              </Tool>
              <Tool
                label="Tiche zneskodnenie"
                active={action === "takedown"}
                onClick={() => arm("takedown")}
              >
                <Swords />
              </Tool>
              <Tool
                label="Hodit kamen"
                active={action === "distract"}
                disabled={active.charges <= 0}
                onClick={() => arm("distract")}
              >
                <CircleDot />
              </Tool>
              <Tool
                label={active.carrying ? "Polozit telo" : "Zdvihnut telo"}
                active={action === "carry"}
                onClick={() =>
                  active.carrying ? use(() => s.drop())() : arm("carry")
                }
              >
                <Backpack />
              </Tool>
              <Tool
                label="Odstranit posledny cakajuci rozkaz"
                disabled={
                  !s.paused || active.orders.length <= (active.started ? 1 : 0)
                }
                onClick={use(() => s.undo())}
              >
                <Undo2 />
              </Tool>
              <Tool
                label="Chodza (W)"
                active={active.stance === "walk"}
                onClick={use(() => s.stance("walk"))}
              >
                <PersonStanding />
              </Tool>
              <Tool
                label="Beh (R)"
                active={active.stance === "run"}
                onClick={use(() => s.stance("run"))}
              >
                <Footprints />
              </Tool>
              <Tool
                label="Prikrcenie (C)"
                active={active.stance === "crouch"}
                onClick={use(() => s.stance("crouch"))}
              >
                <ArrowDown />
              </Tool>
              <Tool
                label="Plazenie (V)"
                active={active.stance === "prone"}
                onClick={use(() => s.stance("prone"))}
              >
                <ChevronsDown />
              </Tool>
              <Tool
                label="Zorne polia"
                active={settings.cones}
                onClick={() =>
                  changeSettings({ ...settings, cones: !settings.cones })
                }
              >
                <Eye />
              </Tool>
              <Tool
                label="Nahlad cesty"
                active={settings.paths}
                onClick={() =>
                  changeSettings({ ...settings, paths: !settings.paths })
                }
              >
                <Route />
              </Tool>
              <Tool label="Zamerat postavu (Home)" onClick={() => r?.focus()}>
                <Focus />
              </Tool>
              <Tool label="Menu (Esc)" onClick={() => menu("pause")}>
                <Menu />
              </Tool>
            </div>
            <div className="rt-camera">
              <Tool
                label="Otocit dolava (Q)"
                onClick={() => {
                  if (r) r.yaw -= Math.PI / 4;
                }}
              >
                <RotateCcw />
              </Tool>
              <Tool
                label="Otocit doprava (])"
                onClick={() => {
                  if (r) r.yaw += Math.PI / 4;
                }}
              >
                <RotateCw />
              </Tool>
              <Tool label="Oddialit" onClick={() => r?.zoom(0.85)}>
                <Minus />
              </Tool>
              <Tool label="Priblizit" onClick={() => r?.zoom(1.18)}>
                <Plus />
              </Tool>
            </div>
          </section>
        )}
      </div>
      {page && (
        <div className="rt-overlay">
          <section
            className="rt-menu"
            role="dialog"
            aria-modal="true"
            aria-label="Menu hry"
          >
            <header>
              <div>
                <small>SNP · MYJAVSKE KOPANICE · 1944</small>
                <h1>Operacia Kopanice</h1>
              </div>
              {deployed && (
                <Tool label="Pokracovat v misii" onClick={() => menu(null)}>
                  <X />
                </Tool>
              )}
            </header>
            {page === "main" && (
              <nav className="rt-menu-actions">
                {deployed && (
                  <button onClick={() => menu(null)}>Pokracovat v misii</button>
                )}
                <button className="rt-primary" onClick={() => menu("missions")}>
                  Nova operacia
                </button>
                <button onClick={() => menu("options")}>Nastavenia</button>
              </nav>
            )}
            {page === "pause" && (
              <nav className="rt-menu-actions">
                <h2>Pozastavena misia</h2>
                <button className="rt-primary" onClick={() => menu(null)}>
                  Pokracovat
                </button>
                <button onClick={() => menu("missions")}>Operacie</button>
                <button onClick={() => menu("options")}>Nastavenia</button>
                <button
                  onClick={() => {
                    if (confirm("Restartovat aktualnu misiu?")) deploy(mission);
                  }}
                >
                  Restartovat misiu
                </button>
                <button onClick={() => menu("main")}>Hlavne menu</button>
              </nav>
            )}
            {page === "missions" && (
              <>
                <h2>Operacie</h2>
                <div className="rt-missions">
                  {MISSIONS.map((m, i) => (
                    <button
                      key={m.id}
                      className={mission === i ? "is-active" : ""}
                      onClick={() => setMission(i)}
                      aria-pressed={mission === i}
                    >
                      <b>0{i + 1}</b>
                      <span>{m.name}</span>
                      <small>
                        {m.width} × {m.height} m
                      </small>
                    </button>
                  ))}
                </div>
                <p className="rt-briefing">{MISSIONS[mission].briefing}</p>
                <footer>
                  <button onClick={() => menu(deployed ? "pause" : "main")}>
                    Spat
                  </button>
                  <button
                    className="rt-primary"
                    onClick={() => deploy(mission)}
                  >
                    Nasadit tim
                  </button>
                </footer>
              </>
            )}
            {page === "options" && (
              <>
                <h2>Nastavenia</h2>
                <label className="rt-setting">
                  <span>Zorne polia nepriatelov</span>
                  <input
                    type="checkbox"
                    checked={settings.cones}
                    onChange={(e) =>
                      changeSettings({ ...settings, cones: e.target.checked })
                    }
                  />
                </label>
                <label className="rt-setting">
                  <span>Nahlad navigacnej cesty</span>
                  <input
                    type="checkbox"
                    checked={settings.paths}
                    onChange={(e) =>
                      changeSettings({ ...settings, paths: e.target.checked })
                    }
                  />
                </label>
                <label className="rt-setting">
                  <span>Rychlost kamery</span>
                  <input
                    type="range"
                    min=".5"
                    max="2"
                    step=".1"
                    value={settings.cameraSpeed}
                    onChange={(e) =>
                      changeSettings({
                        ...settings,
                        cameraSpeed: Number(e.target.value),
                      })
                    }
                  />
                </label>
                <footer>
                  <button onClick={() => menu(deployed ? "pause" : "main")}>
                    Spat
                  </button>
                </footer>
              </>
            )}
            <div className="rt-version">
              STEALTH RTT · WEB PROTOTYP · 2026.10.08
            </div>
          </section>
        </div>
      )}
      {!page && s.outcome && (
        <div className="rt-overlay">
          <section
            className="rt-menu"
            role="dialog"
            aria-label="Vysledok misie"
          >
            <h1>{s.outcome === "won" ? "Misia splnena" : "Misia zlyhala"}</h1>
            <p>
              {s.mission.name} · {time(s.elapsed)}
            </p>
            <footer>
              <button onClick={() => menu("missions")}>Operacie</button>
              <button
                className="rt-primary"
                onClick={() =>
                  deploy(
                    s.outcome === "won"
                      ? (mission + 1) % MISSIONS.length
                      : mission,
                  )
                }
              >
                {s.outcome === "won" ? "Dalsia operacia" : "Skusit znova"}
              </button>
            </footer>
          </section>
        </div>
      )}
    </main>
  );
}

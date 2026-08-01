import { Camera } from '@/core/Camera';
import { Input } from '@/core/Input';
import { defaultIso, type IsoConfig, screenToGrid } from '@/core/math/iso';
import { GoGrid } from './model/grid';
import {
  advanceGuards,
  applyPlayerMove,
  cloneState,
  dangerCells,
  initState,
  legalMoves,
  resolve,
} from './model/logic';
import {
  type Dir,
  DIR_VEC,
  type GoLevel,
  type GoState,
  type GuardKind,
  type Move,
  type Outcome,
  type Phase,
} from './model/types';
import { LEVELS } from './levels';
import { GoRenderer, type GuardView } from './GoRenderer';
import { SpriteCache } from './SpriteCache';

/** Per-type guard count shown in the enemy panel. */
export interface GuardTypeCount {
  kind: GuardKind;
  alive: number;
  total: number;
  maxSight: number;
  alerted: number;
}

/** Snapshot the DOM HUD needs each frame. */
export interface GoHudModel {
  levelName: string;
  levelIndex: number;
  levelCount: number;
  intro: string;
  turn: number;
  phase: Phase;
  outcome: Outcome;
  guardsAlive: number;
  guardsTotal: number;
  /** Unique guard types present in the level, with live/total counts. */
  guardTypes: GuardTypeCount[];
  canUndo: boolean;
  isLast: boolean;
  log: string[];
}

export interface GoGameCallbacks {
  /** Called every frame with the current HUD snapshot. */
  onHud?: (m: GoHudModel) => void;
  /** Fired once, after the animation settles, when a level is won or lost. */
  onOutcome?: (phase: 'won' | 'lost', info: { levelIndex: number; turns: number }) => void;
}

type GuardAnim = GuardView;

const EASE = 0.3;
const EPS = 0.02;

/**
 * Turn-based controller for the GO puzzle. The logical state advances instantly
 * through the pure rules in `model/logic.ts`; this class only animates the board
 * toward that state (player half, then guard half) and routes input into moves.
 *
 * Level selection, menu, and win/lose UI live in {@link GoApp}: this class is
 * *paused* (`active = false`) whenever a modal is up, so the board freezes as a
 * backdrop, and it reports outcomes via {@link GoGameCallbacks.onOutcome}.
 */
export class GoGame {
  private readonly input = new Input();
  private readonly cam: Camera;
  private readonly iso: IsoConfig = defaultIso;
  private readonly renderer: GoRenderer;

  private levelIndex = 0;
  private level!: GoLevel;
  private grid!: GoGrid;
  private state!: GoState;

  private readonly undoStack: GoState[] = [];
  private readonly logLines: string[] = [];

  // Visual (interpolated) actor positions.
  private pv = { x: 0, y: 0 };
  private guardViews = new Map<string, GuardAnim>();

  /** Two-phase turn animation: player step, then the guards' step. */
  private anim: 'idle' | 'player' | 'guards' = 'idle';
  private highlightedKind: GuardKind | null = null;
  private highlightUntil = 0;

  private running = false;
  /** When false the board is frozen (a menu/overlay is showing over it). */
  private active = false;
  private outcomeReported = false;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly cb: GoGameCallbacks = {},
  ) {
    const ctx = canvas.getContext('2d');
    if (!ctx) throw new Error('2D canvas context unavailable');
    this.cam = new Camera(canvas.width, canvas.height, this.iso);
    this.renderer = new GoRenderer(ctx, canvas, this.cam, this.iso, new SpriteCache());
  }

  /** Attach input and begin the render loop with level 0 as a frozen backdrop. */
  start(): void {
    this.input.attach(this.canvas);
    this.loadLevel(0);
    this.running = true;
    requestAnimationFrame(this.loop);
  }

  resize(width: number, height: number): void {
    this.canvas.width = width;
    this.canvas.height = height;
    this.cam.viewportWidth = width;
    this.cam.viewportHeight = height;
    if (this.grid) this.renderer.frameBoard(this.grid);
  }

  // --- Public controls (driven by GoApp) -------------------------------------

  get index(): number {
    return this.levelIndex;
  }
  get turns(): number {
    return this.state.turn;
  }
  get isLast(): boolean {
    return this.levelIndex + 1 >= LEVELS.length;
  }
  get canUndo(): boolean {
    return this.undoStack.length > 0;
  }

  highlightGuardKind(kind: GuardKind): void {
    if (!this.active) return;
    this.highlightedKind = kind;
    this.highlightUntil = performance.now() + 1400;
  }

  /** Load a level and start playing it. */
  play(index: number): void {
    this.loadLevel(index);
    this.activate();
  }

  restart(): void {
    this.loadLevel(this.levelIndex);
    this.activate();
  }

  /** Advance to the next level; returns false if this was the last one. */
  nextLevel(): boolean {
    if (this.isLast) return false;
    this.loadLevel(this.levelIndex + 1);
    this.activate();
    return true;
  }

  undo(): void {
    const prev = this.undoStack.pop();
    if (!prev) return;
    this.state = prev;
    this.anim = 'idle';
    this.highlightedKind = null;
    this.highlightUntil = 0;
    this.snapVisuals();
    this.log('Ťah vrátený späť.');
    this.activate();
  }

  /** Freeze the board (a menu/overlay is taking over). */
  pause(): void {
    this.active = false;
  }

  private activate(): void {
    this.outcomeReported = false;
    this.active = true;
    this.input.takeKeys();
    this.input.takeClicks(); // drop anything buffered while paused
  }

  // --- Level lifecycle -------------------------------------------------------

  private loadLevel(index: number): void {
    this.levelIndex = index;
    this.level = LEVELS[index];
    this.grid = new GoGrid(this.level);
    this.state = initState(this.level);
    this.undoStack.length = 0;
    this.anim = 'idle';
    this.outcomeReported = false;
    this.highlightedKind = null;
    this.highlightUntil = 0;
    this.snapVisuals();
    this.renderer.frameBoard(this.grid);
    this.logLines.length = 0;
    this.log(`Úroveň ${index + 1}/${LEVELS.length}: ${this.level.name}`);
    if (this.level.intro) this.log(this.level.intro);
  }

  /** Snap all visual positions to the current logical state (no tween). */
  private snapVisuals(): void {
    this.pv = { x: this.state.player.x, y: this.state.player.y };
    this.guardViews.clear();
    for (const g of this.state.guards) {
      this.guardViews.set(g.id, {
        id: g.id,
        x: g.x,
        y: g.y,
        facing: g.facing,
        alive: g.alive,
        alerted: g.alerted,
        fade: g.alive ? 1 : 0,
        variant: g.variant,
      });
    }
  }

  // --- Turn execution --------------------------------------------------------

  private tryMove(move: Move): void {
    if (this.anim !== 'idle' || this.state.phase !== 'await') return;
    const before = this.state;
    const after = applyPlayerMove(this.grid, before, move);
    if (after.turn === before.turn) return; // refused / no-op
    this.highlightedKind = null;
    this.highlightUntil = 0;

    this.undoStack.push(cloneState(before));
    this.state = after;
    this.anim = 'player';
    for (const guard of after.guards) {
      if (!before.guards.find((previous) => previous.id === guard.id)?.alerted && guard.alerted) {
        this.log(`Dôstojník zalarmoval pechotu (${guard.id}).`);
      }
    }
    if (after.distractions.some((d) => d.used && !before.distractions.find((b) => b.id === d.id)?.used)) {
      this.log('Generátor aktivovaný — pozornosť odvrátená.');
    }

    // Narrate anything decisive from the player half.
    const killed = before.guards.filter(
      (g) => g.alive && !after.guards.find((h) => h.id === g.id)?.alive,
    );
    for (const g of killed) this.log(`Ticho zneškodnený strážca (${g.id}).`);
    if (this.togglesGate(before, after)) this.log('Terminál aktivovaný — brána sa mení.');
    if (after.phase === 'won') this.log('Východ dosiahnutý!');
    if (after.phase === 'lost') this.log('Odhalený!');
  }

  private togglesGate(before: GoState, after: GoState): boolean {
    return before.gates.some((g) => after.gates.find((h) => h.id === g.id)?.open !== g.open);
  }

  /** Drive the two-phase animation forward once the current phase has settled. */
  private stepAnim(): void {
    if (this.anim === 'idle') return;
    if (!this.settled()) return;

    if (this.anim === 'player') {
      if (this.state.phase !== 'await') {
        this.anim = 'idle';
        return;
      }
      this.state = resolve(this.grid, advanceGuards(this.grid, this.state));
      this.anim = 'guards';
      if (this.state.phase === 'lost') this.log('Odhalený!');
    } else if (this.anim === 'guards') {
      this.anim = 'idle';
    }
  }

  /** After the animation settles on a terminal phase, report it once. */
  private reportOutcome(): void {
    if (this.outcomeReported || this.anim !== 'idle') return;
    if (this.state.phase === 'await') return;
    this.outcomeReported = true;
    this.active = false; // freeze the board under the outcome overlay
    this.cb.onOutcome?.(this.state.phase, {
      levelIndex: this.levelIndex,
      turns: this.state.turn,
    });
  }

  /** True when every visual position/fade has reached its logical target. */
  private settled(): boolean {
    if (Math.abs(this.pv.x - this.state.player.x) > EPS) return false;
    if (Math.abs(this.pv.y - this.state.player.y) > EPS) return false;
    for (const g of this.state.guards) {
      const v = this.guardViews.get(g.id);
      if (!v) continue;
      if (Math.abs(v.x - g.x) > EPS || Math.abs(v.y - g.y) > EPS) return false;
      const target = g.alive ? 1 : 0;
      if (Math.abs(v.fade - target) > EPS) return false;
    }
    return true;
  }

  private ease(): void {
    this.pv.x = lerp(this.pv.x, this.state.player.x, EASE);
    this.pv.y = lerp(this.pv.y, this.state.player.y, EASE);
    for (const g of this.state.guards) {
      const v = this.guardViews.get(g.id);
      if (!v) continue;
      v.x = lerp(v.x, g.x, EASE);
      v.y = lerp(v.y, g.y, EASE);
      v.facing = g.facing;
      v.alive = g.alive;
      v.alerted = g.alerted;
      v.fade = lerp(v.fade, g.alive ? 1 : 0, EASE);
    }
  }

  // --- Input -----------------------------------------------------------------

  private handleInput(): void {
    for (const k of this.input.takeKeys()) this.handleKey(k);

    const clicks = this.input.takeClicks();
    if (clicks.length && this.anim === 'idle' && this.state.phase === 'await') {
      const c = clicks[clicks.length - 1];
      const cell = this.pickCell(c.x, c.y);
      const dir = this.dirToCell(cell.x, cell.y);
      if (dir) this.tryMove({ kind: 'step', dir });
      else if (cell.x === this.state.player.x && cell.y === this.state.player.y) {
        const distraction = legalMoves(this.grid, this.state).find(
          (move) => move.kind === 'activateDistraction',
        );
        this.tryMove(distraction ?? { kind: 'wait' });
      }
    }
  }

  /** Only movement keys live here; shell keys (undo/restart/menu) are GoApp's. */
  private handleKey(k: string): void {
    if (this.anim !== 'idle' || this.state.phase !== 'await') return;
    const dir = KEY_DIR[k];
    if (dir) this.tryMove({ kind: 'step', dir });
    else if (k === 'e') {
      const distraction = legalMoves(this.grid, this.state).find(
        (move) => move.kind === 'activateDistraction',
      );
      if (distraction) this.tryMove(distraction);
    }
    else if (k === ' ' || k === '.') this.tryMove({ kind: 'wait' });
  }

  /** Screen pixel → grid cell on the z=0 plane. */
  private pickCell(sx: number, sy: number): { x: number; y: number } {
    const w = this.cam.screenToWorld(sx, sy);
    const g = screenToGrid(w.x, w.y, this.iso);
    return { x: Math.round(g.x), y: Math.round(g.y) };
  }

  /** If (x, y) is orthogonally adjacent to the player, the step direction to it. */
  private dirToCell(x: number, y: number): Dir | null {
    const dx = x - this.state.player.x;
    const dy = y - this.state.player.y;
    for (const [dir, v] of Object.entries(DIR_VEC) as [Dir, { dx: number; dy: number }][]) {
      if (v.dx === dx && v.dy === dy) {
        return legalMoves(this.grid, this.state).some((m) => m.kind === 'step' && m.dir === dir)
          ? dir
          : null;
      }
    }
    return null;
  }

  // --- Frame loop ------------------------------------------------------------

  private loop = (): void => {
    if (!this.running) return;
    if (this.active) {
      this.handleInput();
      this.ease();
      this.stepAnim();
      this.reportOutcome();
    }
    if (this.grid) this.renderer.render(this.buildRenderModel());
    this.cb.onHud?.(this.buildHud());
    requestAnimationFrame(this.loop);
  };

  private buildRenderModel() {
    const now = performance.now();
    const highlightedGuardIds =
      this.highlightedKind && now < this.highlightUntil
        ? new Set(
            this.state.guards
              .filter((guard) => guard.alive && guard.kind === this.highlightedKind)
              .map((guard) => guard.id),
          )
        : new Set<string>();
    if (highlightedGuardIds.size === 0 && now >= this.highlightUntil) {
      this.highlightedKind = null;
    }
    const showLegal = this.active && this.anim === 'idle' && this.state.phase === 'await';
    const legal = showLegal
      ? legalMoves(this.grid, this.state)
          .filter((m) => m.kind === 'step')
          .map((m) => {
            const v = DIR_VEC[(m as { dir: Dir }).dir];
            return { x: this.state.player.x + v.dx, y: this.state.player.y + v.dy };
          })
      : [];
    const hover = this.pickCell(this.input.mouseScreen.x, this.input.mouseScreen.y);
    return {
      grid: this.grid,
      state: this.state,
      player: { x: this.pv.x, y: this.pv.y, facing: this.state.player.facing },
      guards: [...this.guardViews.values()],
      highlightedGuardIds,
      danger: dangerCells(this.grid, this.state),
      legal,
      hover: this.active && this.grid.inBounds(hover.x, hover.y) ? hover : null,
    };
  }

  private buildHud(): GoHudModel {
    const guardsAlive = this.state.guards.filter((g) => g.alive).length;

    // Aggregate unique guard types with alive/total counts.
    const typeMap = new Map<GuardKind, { alive: number; total: number; maxSight: number; alerted: number }>();
    for (const g of this.state.guards) {
      const entry = typeMap.get(g.kind) ?? { alive: 0, total: 0, maxSight: 0, alerted: 0 };
      entry.total += 1;
      if (g.alive) entry.alive += 1;
      if (g.alerted) entry.alerted += 1;
      entry.maxSight = Math.max(entry.maxSight, g.sight);
      typeMap.set(g.kind, entry);
    }
    // Stable order: sentry first, then patrol.
    const kindOrder: GuardKind[] = ['sentry', 'patrol'];
    const guardTypes = kindOrder
      .filter((k) => typeMap.has(k))
      .map((k) => ({ kind: k, ...typeMap.get(k)! }));

    return {
      levelName: this.level.name,
      levelIndex: this.levelIndex,
      levelCount: LEVELS.length,
      intro: this.level.intro ?? '',
      turn: this.state.turn,
      phase: this.state.phase,
      outcome: this.state.outcome,
      guardsAlive,
      guardsTotal: this.state.guards.length,
      guardTypes,
      canUndo: this.undoStack.length > 0,
      isLast: this.isLast,
      log: [...this.logLines],
    };
  }

  private log(message: string): void {
    this.logLines.push(message);
    while (this.logLines.length > 8) this.logLines.shift();
  }
}

function lerp(a: number, b: number, t: number): number {
  const n = a + (b - a) * t;
  return Math.abs(n - b) < EPS ? b : n;
}

const KEY_DIR: Record<string, Dir> = {
  arrowup: 'N',
  arrowdown: 'S',
  arrowleft: 'W',
  arrowright: 'E',
  w: 'N',
  s: 'S',
  a: 'W',
  d: 'E',
};

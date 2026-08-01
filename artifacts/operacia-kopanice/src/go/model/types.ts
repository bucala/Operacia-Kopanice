/**
 * Data model for the turn-based, node-graph tactics puzzle (in the vein of
 * *Lara Croft GO* / *Deus Ex GO*).
 *
 * Everything here is plain, serialisable data. The rules in `logic.ts` are pure
 * functions over these shapes, which keeps the whole game deterministic and
 * trivially snapshot-able for undo.
 */

/** The four orthogonal directions a GO node-graph connects. */
export type Dir = 'N' | 'E' | 'S' | 'W';

export const DIRS: readonly Dir[] = ['N', 'E', 'S', 'W'];

/** Unit grid vector for each direction (screen: +x right, +y down). */
export const DIR_VEC: Record<Dir, { dx: number; dy: number }> = {
  N: { dx: 0, dy: -1 },
  E: { dx: 1, dy: 0 },
  S: { dx: 0, dy: 1 },
  W: { dx: -1, dy: 0 },
};

/** The direction facing back the way you came. */
export function opposite(d: Dir): Dir {
  return d === 'N' ? 'S' : d === 'S' ? 'N' : d === 'E' ? 'W' : 'E';
}

/** Direction of the step (dx, dy) if it is a unit orthogonal move, else null. */
export function dirOf(dx: number, dy: number): Dir | null {
  if (dx === 0 && dy === -1) return 'N';
  if (dx === 0 && dy === 1) return 'S';
  if (dx === 1 && dy === 0) return 'E';
  if (dx === -1 && dy === 0) return 'W';
  return null;
}

/** Static terrain of a single node. */
export type CellKind =
  /** No node here — a gap in the graph. */
  | 'void'
  /** A walkable node. */
  | 'floor'
  /** A compacted village road; walkable and visually distinct from snow. */
  | 'road'
  /** A wooden plank bridge or footpath; walkable. */
  | 'plank'
  /** A muddy track; walkable but visually softer than the road. */
  | 'mud'
  /** Solid: blocks movement and line of sight. */
  | 'wall'
  /** Static tree cover; blocks movement and line of sight. */
  | 'tree'
  /** Static rock cover; blocks movement and line of sight. */
  | 'rock'
  /** The goal node; stepping here wins the level. */
  | 'exit';

export type DecorationKind = 'house1' | 'house2' | 'tree' | 'crate' | 'fence';

/** A visual village object anchored to one logical cell. */
export interface DecorationSpec {
  kind: DecorationKind;
  x: number;
  y: number;
  /** Width as a multiple of the isometric tile width. */
  scale?: number;
  /** Optional painter offset for objects that sit in front of their cell. */
  layer?: number;
  /**
   * Optional logical collision flags for decorative props.
   * Houses and trees are solid by default; lightweight props remain visual-only
   * unless a level opts them into collision explicitly.
   */
  blocksMovement?: boolean;
  blocksSight?: boolean;
}

export type GuardKind =
  /** Walks a fixed route, ping-ponging between its ends; faces its travel dir. */
  | 'patrol'
  /** Stays put; optionally rotates its facing through a fixed cycle each turn. */
  | 'sentry';

/**
 * Visual variant for a guard — selects which sprite is rendered.
 * Falls back to the default officer sprite when unset.
 */
export type GuardVariant = 'officer' | 'sniper' | 'machinegunner';

/** Authoring spec for a guard, as written in a level file. */
export interface GuardSpec {
  id: string;
  kind: GuardKind;
  x: number;
  y: number;
  facing: Dir;
  /** Patrol waypoints (grid cells), travelled in order then reversed. */
  route?: [number, number][];
  /** Sentry facing cycle, advanced one entry per turn. */
  rotate?: Dir[];
  /** Straight-line sight range in cells (a guard sees along its facing). */
  sight: number;
  /** Visual sprite variant — determines which character sprite is drawn. */
  variant?: GuardVariant;
}

/** A hackable terminal (Deus Ex GO): entering it toggles the linked gate. */
export interface TerminalSpec {
  id: string;
  x: number;
  y: number;
  /** Id of the {@link GateSpec} this terminal opens/closes. */
  gate: string;
}

/** A gate that overlays a node: open = walkable & see-through, closed = wall. */
export interface GateSpec {
  id: string;
  x: number;
  y: number;
  open: boolean;
}

/** A fully hand-authored puzzle level. */
export interface GoLevel {
  name: string;
  /** One-line Slovak briefing shown on entry. */
  intro?: string;
  width: number;
  height: number;
  /**
   * Terrain rows. Each char: `#` wall · `.` floor · `=` road · `-` plank ·
 * `~` mud · `T` tree · `R` rock · `X` exit · ` ` or `_` void.
   * Entities (player/guards/terminals/gates) are placed by the specs below.
   */
  cells: string[];
  /** Optional anchored art objects with explicit logical collision policy. */
  decorations?: DecorationSpec[];
  start: { x: number; y: number; facing: Dir };
  guards: GuardSpec[];
  terminals?: TerminalSpec[];
  gates?: GateSpec[];
}

// --- Runtime state (snapshotted for undo) ------------------------------------

export interface GuardState {
  id: string;
  kind: GuardKind;
  x: number;
  y: number;
  facing: Dir;
  /** Index of the route waypoint the guard is currently heading toward. */
  routeIndex: number;
  /** Ping-pong direction along the route: +1 forward, -1 back. */
  routeDir: 1 | -1;
  /** Index into the rotate cycle (sentries). */
  rotateIndex: number;
  route: [number, number][];
  rotate: Dir[];
  sight: number;
  alive: boolean;
  /** Visual sprite variant carried from GuardSpec. */
  variant?: GuardVariant;
}

export interface GateState {
  id: string;
  x: number;
  y: number;
  open: boolean;
}

export interface TerminalState {
  id: string;
  x: number;
  y: number;
  gate: string;
}

export type Phase = 'await' | 'won' | 'lost';

/** Why the mission ended, for the HUD. */
export type Outcome = null | 'exit' | 'spotted' | 'collision';

/** The complete mutable state of a play-through at a single turn boundary. */
export interface GoState {
  player: { x: number; y: number; facing: Dir };
  guards: GuardState[];
  gates: GateState[];
  terminals: TerminalState[];
  phase: Phase;
  outcome: Outcome;
  turn: number;
}

/** A single action the player can take on their turn. */
export type Move =
  | { kind: 'step'; dir: Dir }
  | { kind: 'wait' };

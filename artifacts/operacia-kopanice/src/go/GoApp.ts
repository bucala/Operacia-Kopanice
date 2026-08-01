import { GoGame, type GoHudModel, type GuardTypeCount } from './GoGame';
import { LEVELS } from './levels';
import {
  bestTurns,
  firstPlayable,
  isCompleted,
  isUnlocked,
  loadProgress,
  type Progress,
  recordWin,
  saveProgress,
} from './progress';

type OverlayKind = 'none' | 'menu' | 'win' | 'lose';

interface ElOpts {
  class?: string;
  text?: string;
  onclick?: () => void;
  attrs?: Record<string, string>;
}

function el<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  opts: ElOpts = {},
  children: Node[] = [],
): HTMLElementTagNameMap[K] {
  const node = document.createElement(tag);
  if (opts.class) node.className = opts.class;
  if (opts.text !== undefined) node.textContent = opts.text;
  if (opts.onclick) node.addEventListener('click', opts.onclick);
  if (opts.attrs) for (const [k, v] of Object.entries(opts.attrs)) node.setAttribute(k, v);
  for (const c of children) node.appendChild(c);
  return node;
}

/**
 * The UI/UX shell around {@link GoGame}: a title/level-select menu, an in-game
 * top bar (level, turn count, undo/restart/menu buttons), a bottom hint+legend
 * bar, and win/lose modals — all plain DOM layered over the canvas. It owns
 * screen state, level progression, and localStorage progress; the canvas game
 * is paused (frozen as a backdrop) whenever a modal is up.
 */
export class GoApp {
  private readonly game: GoGame;
  private progress: Progress = loadProgress();
  private overlayKind: OverlayKind = 'none';

  // Long-lived DOM the HUD updates each frame.
  private readonly topbar: HTMLElement;
  private readonly hintbar: HTMLElement;
  private readonly enemyPanel: HTMLElement;
  private readonly overlay: HTMLElement;
  private readonly elLevel = el('span', { class: 'tb-strong' });
  private readonly elTurn = el('span');
  private readonly elGuards = el('span');
  private readonly elIntro = el('div', { class: 'hint-intro' });
  private readonly undoBtn: HTMLButtonElement;
  private readonly restartBtn: HTMLButtonElement;
  /** Live card elements keyed by guard kind, updated each HUD frame. */
  private readonly epCards = new Map<string, HTMLElement>();

  constructor(canvas: HTMLCanvasElement, root: HTMLElement) {
    this.game = new GoGame(canvas, {
      onHud: (m) => this.onHud(m),
      onOutcome: (phase, info) => this.onOutcome(phase, info),
    });

    this.undoBtn = el('button', {
      class: 'btn ghost',
      text: '↶ Späť',
      onclick: () => this.doUndo(),
      attrs: {
        type: 'button',
        title: 'Vrátiť posledný ťah späť',
        'aria-label': 'Vrátiť posledný ťah späť',
        'aria-keyshortcuts': 'U Z',
      },
    });
    this.restartBtn = el('button', {
      class: 'btn ghost',
      text: '⟳ Reset',
      onclick: () => this.doRestart(),
      attrs: {
        type: 'button',
        title: 'Reštartovať aktuálnu misiu',
        'aria-label': 'Reštartovať aktuálnu misiu',
        'aria-keyshortcuts': 'R',
      },
    });
    this.topbar = this.buildTopbar();
    this.hintbar = this.buildHintbar();
    this.enemyPanel = this.buildEnemyPanel();
    this.overlay = el('div', { class: 'overlay hidden' });

    root.append(this.topbar, this.hintbar, this.enemyPanel, this.overlay);
  }

  start(): void {
    this.game.start();
    window.addEventListener('keydown', (e) => this.onKey(e));
    this.showMenu();
  }

  resize(width: number, height: number): void {
    this.game.resize(width, height);
  }

  // --- Screen transitions ----------------------------------------------------

  private showMenu(): void {
    this.game.pause();
    this.overlayKind = 'menu';
    this.setChrome(false);
    this.renderOverlay(this.buildMenu());
  }

  private play(index: number): void {
    if (!isUnlocked(this.progress, index)) return;
    this.overlayKind = 'none';
    this.setChrome(true);
    this.hideOverlay();
    this.game.play(index);
  }

  private onOutcome(phase: 'won' | 'lost', info: { levelIndex: number; turns: number }): void {
    if (phase === 'won') {
      this.progress = recordWin(this.progress, info.levelIndex, info.turns);
      saveProgress(this.progress);
      this.overlayKind = 'win';
      this.renderOverlay(this.buildWin(info));
    } else {
      this.overlayKind = 'lose';
      this.renderOverlay(this.buildLose());
    }
  }

  private doNext(): void {
    this.hideOverlay();
    this.overlayKind = 'none';
    if (!this.game.nextLevel()) this.showMenu();
  }

  private doRestart(): void {
    this.overlayKind = 'none';
    this.hideOverlay();
    this.setChrome(true);
    this.game.restart();
  }

  private doUndo(): void {
    if (!this.game.canUndo) return;
    this.overlayKind = 'none';
    this.hideOverlay();
    this.setChrome(true);
    this.game.undo();
  }

  // --- Keyboard shortcuts (shell-level) --------------------------------------

  private onKey(e: KeyboardEvent): void {
    const k = e.key.toLowerCase();
    // Holding a recovery shortcut must not undo/restart multiple times.
    if (e.repeat && (k === 'u' || k === 'z' || k === 'r')) return;
    if (k === 'escape') {
      if (this.overlayKind === 'menu') return;
      e.preventDefault();
      this.showMenu();
    } else if (this.overlayKind === 'win' && (k === 'n' || k === 'enter')) {
      e.preventDefault();
      this.doNext();
    } else if (this.overlayKind !== 'menu' && (k === 'u' || k === 'z')) {
      e.preventDefault();
      this.doUndo();
    } else if (this.overlayKind !== 'menu' && k === 'r') {
      e.preventDefault();
      this.doRestart();
    }
  }

  // --- HUD (per-frame) -------------------------------------------------------

  private onHud(m: GoHudModel): void {
    this.elLevel.textContent = `${m.levelName}`;
    this.elTurn.textContent = `Ťah ${m.turn}`;
    this.elGuards.textContent = `Stráže ${m.guardsAlive}/${m.guardsTotal}`;
    this.elIntro.textContent = m.intro;
    this.undoBtn.disabled = !m.canUndo;
    this.updateEnemyPanel(m.guardTypes);
  }

  // --- DOM builders ----------------------------------------------------------

  private buildTopbar(): HTMLElement {
    const info = el('div', { class: 'tb-info' }, [
      this.elLevel,
      el('span', { class: 'tb-sep', text: '·' }),
      this.elTurn,
      el('span', { class: 'tb-sep', text: '·' }),
      this.elGuards,
    ]);
    const actions = el('div', { class: 'tb-actions' }, [
      this.undoBtn,
      this.restartBtn,
      el('button', {
        class: 'btn',
        text: '◈ Menu',
        onclick: () => this.showMenu(),
        attrs: {
          type: 'button',
          title: 'Vrátiť sa do výberu úrovní',
          'aria-label': 'Vrátiť sa do výberu úrovní',
        },
      }),
    ]);
    return el('header', { class: 'topbar hidden' }, [info, actions]);
  }

  private buildEnemyPanel(): HTMLElement {
    // The panel is initially empty; cards are added/updated per-level via updateEnemyPanel().
    return el('div', { class: 'enemy-panel hidden' });
  }

  private updateEnemyPanel(types: GuardTypeCount[]): void {
    // Add any new cards not yet in the panel.
    for (const gt of types) {
      if (!this.epCards.has(gt.kind)) {
        const card = buildEpCard(gt.kind, () => this.game.highlightGuardKind(gt.kind));
        this.epCards.set(gt.kind, card);
        this.enemyPanel.appendChild(card);
      }
      const card = this.epCards.get(gt.kind)!;
      const neutralised = gt.alive === 0;
      card.classList.toggle('ep-dead', neutralised);
      card.classList.toggle('ep-alerted', gt.alerted > 0);
      // Update the small count badge.
      const badge = card.querySelector<HTMLElement>('.ep-count');
      if (badge) {
        badge.textContent = `${gt.alive}/${gt.total}`;
      }
      const sight = card.querySelector<HTMLElement>('.ep-sight');
      if (sight) sight.textContent = `◉ ${gt.maxSight} polí`;
      const alert = card.querySelector<HTMLElement>('.ep-alert');
      if (alert) alert.textContent = gt.alerted > 0 ? `⚠ ${gt.alerted} v strehu` : '';
    }
    // Remove cards for kinds no longer in the level.
    for (const [kind, card] of this.epCards) {
      if (!types.find((t) => t.kind === kind)) {
        card.remove();
        this.epCards.delete(kind);
      }
    }
  }

  private buildHintbar(): HTMLElement {
    const legend = el('div', { class: 'legend' }, [
      legendItem('sw-danger', 'smrteľný lúč'),
      legendItem('sw-move', 'možný krok'),
      legendItem('sw-take', 'zozadu = tichá likvidácia'),
      legendItem('sw-distraction', 'E = generátor'),
    ]);
    return el('footer', { class: 'hintbar hidden' }, [this.elIntro, legend]);
  }

  private buildMenu(): HTMLElement {
    const started = Object.keys(this.progress.best).length > 0;
    const target = firstPlayable(this.progress, LEVELS.length);
    const cards = LEVELS.map((lvl, i) => this.buildLevelCard(lvl.name, i));

    return el('div', { class: 'panel menu' }, [
      el('div', { class: 'brand' }, [
        brandMark(),
        el('div', { class: 'subtitle', text: 'Ťahová taktická hádanka · v štýle Lara Croft GO' }),
      ]),
      el('button', {
        class: 'btn primary big',
        text: started ? '▶ Pokračovať' : '▶ Hrať',
        onclick: () => this.play(target),
      }),
      el('div', { class: 'section-label', text: 'Úrovne' }),
      el('div', { class: 'level-grid' }, cards),
      el('div', {
        class: 'menu-foot',
        text: 'Klik/šípky = krok · E = generátor · medzerník = čakaj · U = späť · R = znova · Esc = menu',
      }),
    ]);
  }

  private buildLevelCard(name: string, index: number): HTMLElement {
    const unlocked = isUnlocked(this.progress, index);
    const done = isCompleted(this.progress, index);
    const best = bestTurns(this.progress, index);

    const badge = done
      ? el('span', { class: 'badge done', text: `✓ ${best} ťahov` })
      : unlocked
        ? el('span', { class: 'badge open', text: '▶' })
        : el('span', { class: 'badge lock', text: '▣  zamknuté' });

    return el(
      'button',
      {
        class: `level-card${unlocked ? '' : ' locked'}${done ? ' cleared' : ''}`,
        onclick: () => this.play(index),
        attrs: unlocked ? {} : { disabled: 'true' },
      },
      [
        el('span', { class: 'level-num', text: String(index + 1) }),
        el('span', { class: 'level-name', text: name }),
        badge,
      ],
    );
  }

  private buildWin(info: { turns: number }): HTMLElement {
    const best = bestTurns(this.progress, this.game.index);
    const last = this.game.isLast;
    const buttons: Node[] = [];
    if (!last) {
      buttons.push(
        el('button', { class: 'btn primary', text: 'Ďalšia úroveň ▶', onclick: () => this.doNext() }),
      );
    }
    buttons.push(el('button', { class: 'btn ghost', text: '⟳ Znova', onclick: () => this.doRestart() }));
    buttons.push(el('button', { class: 'btn', text: '◈ Menu', onclick: () => this.showMenu() }));

    return el('div', { class: 'panel outcome win' }, [
      el('div', { class: 'outcome-icon', text: '✔' }),
      el('h2', { text: last ? 'Všetky úrovne splnené!' : 'Úroveň splnená' }),
      el('div', {
        class: 'outcome-sub',
        text: `Vyriešené za ${info.turns} ťahov${best !== null && best < info.turns ? ` · najlepšie ${best}` : ''}`,
      }),
      el('div', { class: 'outcome-actions' }, buttons),
    ]);
  }

  private buildLose(): HTMLElement {
    const buttons: Node[] = [];
    if (this.game.canUndo) {
      buttons.push(
        el('button', { class: 'btn primary', text: '↶ Vrátiť ťah (U)', onclick: () => this.doUndo() }),
      );
    }
    buttons.push(el('button', { class: 'btn ghost', text: '⟳ Skús znova', onclick: () => this.doRestart() }));
    buttons.push(el('button', { class: 'btn', text: '◈ Menu', onclick: () => this.showMenu() }));

    return el('div', { class: 'panel outcome lose' }, [
      el('div', { class: 'outcome-icon', text: '✖' }),
      el('h2', { text: 'Odhalený' }),
      el('div', { class: 'outcome-sub', text: 'Strážca ťa zbadal. Vráť ťah alebo skús úroveň znova.' }),
      el('div', { class: 'outcome-actions' }, buttons),
    ]);
  }

  // --- Overlay plumbing ------------------------------------------------------

  private renderOverlay(panel: HTMLElement): void {
    this.overlay.replaceChildren(panel);
    this.overlay.classList.remove('hidden');
  }

  private hideOverlay(): void {
    this.overlay.classList.add('hidden');
    this.overlay.replaceChildren();
  }

  private setChrome(playing: boolean): void {
    this.topbar.classList.toggle('hidden', !playing);
    this.hintbar.classList.toggle('hidden', !playing);
    this.enemyPanel.classList.toggle('hidden', !playing);
    if (!playing) {
      // Clear cards so next level starts fresh.
      this.epCards.clear();
      this.enemyPanel.replaceChildren();
    }
  }
}

function legendItem(swatchClass: string, label: string): HTMLElement {
  return el('span', { class: 'legend-item' }, [
    el('span', { class: `swatch ${swatchClass}` }),
    el('span', { text: label }),
  ]);
}

function brandMark(): HTMLElement {
  const mark = document.createElement('img');
  mark.className = 'brand-mark';
  mark.src = `${import.meta.env.BASE_URL}brand/operacia-kopanice-logo.png`;
  mark.alt = 'Operácia Kopanice';
  mark.draggable = false;
  return mark;
}

/** Labels and portrait image paths per guard kind. */
const GUARD_KIND_META: Record<string, { label: string; img: string }> = {
  sentry: { label: 'DÔSTOJNÍK', img: `${import.meta.env.BASE_URL}assets/sprites/guard-officer.png` },
  patrol: { label: 'PEŠIAK', img: `${import.meta.env.BASE_URL}assets/sprites/guard-soldier.png` },
};

/** Build a single portrait card for the enemy panel. */
function buildEpCard(kind: string, onClick: () => void): HTMLElement {
  const meta = GUARD_KIND_META[kind] ?? { label: kind.toUpperCase(), img: '' };

  const portrait = el('div', { class: 'ep-portrait' });
  if (meta.img) {
    const img = document.createElement('img');
    img.src = meta.img;
    img.alt = meta.label;
    img.draggable = false;
    portrait.appendChild(img);
  } else {
    // Fallback silhouette when no image is available.
    portrait.innerHTML = `<svg viewBox="0 0 60 70" xmlns="http://www.w3.org/2000/svg" class="ep-silhouette">
      <ellipse cx="30" cy="16" rx="12" ry="13" fill="currentColor"/>
      <path d="M10 70 Q10 40 30 38 Q50 40 50 70Z" fill="currentColor"/>
    </svg>`;
  }

  const badge = el('span', { class: 'ep-count', text: '' });
  const sight = el('span', { class: 'ep-sight', text: '' });
  const alert = el('span', { class: 'ep-alert', text: '' });
  const label = el('div', { class: 'ep-label' }, [
    el('span', { text: meta.label }),
    sight,
    alert,
    badge,
  ]);

  return el('button', {
    class: 'ep-card',
    onclick: onClick,
    attrs: { type: 'button', title: 'Zvýrazniť stráže tohto typu na mape' },
  }, [portrait, label]);
}

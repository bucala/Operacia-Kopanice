import { Game, type HudModel } from '@/game/Game';

const canvas = document.getElementById('game') as HTMLCanvasElement;
const hudEl = document.getElementById('hud') as HTMLDivElement;
const logEl = document.getElementById('log') as HTMLDivElement;

const SKILL_LABEL: Record<string, string> = {
  knife: '🗡 Nôž',
  disguise: '🎭 Prestrojenie',
  stone: '🪨 Kameň',
};

function renderHud(m: HudModel): void {
  const banner = !m.alive
    ? '<div style="color:#f25c5c;font-weight:bold">✖ MISIA ZLYHALA — stlač R pre reštart</div>'
    : m.won
      ? '<div style="color:#7bd88f;font-weight:bold">✔ MISIA SPLNENÁ — stlač R pre reštart</div>'
      : '';

  const skills = m.cooldowns
    .map((c) => {
      const label = SKILL_LABEL[c.id] ?? c.id;
      const ready = c.remaining <= 0.05;
      const sel = c.id === m.selected ? 'border-bottom:1px solid #8fd0ff;' : '';
      const color = ready ? '#d7dde6' : '#7a8290';
      const cd = ready ? '' : ` (${c.remaining.toFixed(1)}s)`;
      return `<span style="${sel}color:${color};margin-right:10px">${label}${cd}</span>`;
    })
    .join('');

  hudEl.innerHTML = `
    ${banner}
    <div><b>${m.mapName}</b></div>
    <div>Cieľ: ${m.objective}</div>
    <div>Zdravie: ${hpBar(m.hp, m.maxHp)} ${m.hp}/${m.maxHp}</div>
    <div>Stav hliadok: <span class="state-${m.alertLevel}">${m.alertLevel}</span>
      · živé stráže: ${m.guardsAlive}${m.disguised ? ' · 🎭 prestrojený' : ''}</div>
    <div style="margin-top:4px">${skills}</div>
    <div style="margin-top:4px;color:#7a8290;font-size:11px">
      sync: ${m.syncName} · asistent: ${m.assistant}${m.paused ? ' · ⏸ PAUZA' : ''}</div>
    <div style="color:#7a8290;font-size:11px">tip: stlač <b>H</b> pre radu asistenta</div>
  `;

  logEl.innerHTML = m.log
    .map((l) => `<div class="log-${l.level}" style="color:${logColor(l.level)}">› ${l.message}</div>`)
    .join('');
}

function hpBar(hp: number, max: number): string {
  const n = Math.round((hp / max) * 10);
  return `[${'█'.repeat(n)}${'░'.repeat(10 - n)}]`;
}

function logColor(level: string): string {
  return level === 'alert' ? '#f25c5c' : level === 'warn' ? '#f2c14e' : '#aeb9c6';
}

function fit(): void {
  game.resize(window.innerWidth, window.innerHeight);
}

const game = new Game(canvas, renderHud);
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;
window.addEventListener('resize', fit);

game.start().catch((err) => {
  hudEl.textContent = `Chyba pri štarte: ${(err as Error).message}`;
  console.error(err);
});

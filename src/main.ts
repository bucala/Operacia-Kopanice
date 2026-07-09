import { GoGame, type GoHudModel } from '@/go/GoGame';

const canvas = document.getElementById('game') as HTMLCanvasElement;
const hudEl = document.getElementById('hud') as HTMLDivElement;
const logEl = document.getElementById('log') as HTMLDivElement;

function renderHud(m: GoHudModel): void {
  const banner =
    m.phase === 'lost'
      ? '<div style="color:#f25c5c;font-weight:bold">✖ ODHALENÝ — U späť · R reštart</div>'
      : m.phase === 'won'
        ? m.isLast
          ? '<div style="color:#7bd88f;font-weight:bold">✔ VŠETKY ÚROVNE SPLNENÉ — R reštart</div>'
          : '<div style="color:#7bd88f;font-weight:bold">✔ ÚROVEŇ SPLNENÁ — stlač N pre ďalšiu</div>'
        : '';

  hudEl.innerHTML = `
    ${banner}
    <div><b>${m.levelName}</b> · úroveň ${m.levelIndex + 1}/${m.levelCount}</div>
    <div style="color:#aeb9c6">${m.intro}</div>
    <div>Ťah: ${m.turn} · stráže: ${m.guardsAlive}/${m.guardsTotal}${
      m.canUndo ? ' · <span style="color:#8fd0ff">U = späť</span>' : ''
    }</div>
    <div style="margin-top:4px;color:#7a8290;font-size:11px">
      Turn-based · klik/šípky = krok · zezadu na strážcu = tichá likvidácia</div>
  `;

  // Build log lines as text nodes rather than innerHTML — a safe habit for any
  // log line, kept from the endpoint-hardening review.
  logEl.replaceChildren(
    ...m.log.map((line) => {
      const div = document.createElement('div');
      div.style.color = '#aeb9c6';
      div.textContent = `› ${line}`;
      return div;
    }),
  );
}

const game = new GoGame(canvas, renderHud);
// Size the canvas + camera viewport before the first frame so the board is
// framed against the real window, not the default canvas size.
game.resize(window.innerWidth, window.innerHeight);
window.addEventListener('resize', () => game.resize(window.innerWidth, window.innerHeight));

try {
  game.start();
} catch (err) {
  hudEl.textContent = `Chyba pri štarte: ${(err as Error).message}`;
  console.error(err);
}

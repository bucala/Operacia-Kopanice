import { GoApp } from '@/go/GoApp';

const canvas = document.getElementById('game') as HTMLCanvasElement;
const ui = document.getElementById('ui') as HTMLDivElement;

const app = new GoApp(canvas, ui);
// Size the canvas + camera viewport before the first frame so the board is
// framed against the real window, not the default canvas size.
app.resize(window.innerWidth, window.innerHeight);
window.addEventListener('resize', () => app.resize(window.innerWidth, window.innerHeight));

try {
  app.start();
} catch (err) {
  ui.textContent = `Chyba pri štarte: ${(err as Error).message}`;
  console.error(err);
}

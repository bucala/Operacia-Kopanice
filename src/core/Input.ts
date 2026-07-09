/**
 * Tracks pointer and keyboard state. The InputSystem reads this each frame and
 * translates it into game actions. Edge-triggered events (clicks, key presses)
 * are buffered and consumed once.
 */
export class Input {
  mouseScreen = { x: 0, y: 0 };
  private clickBuffer: { x: number; y: number; button: number }[] = [];
  private keyBuffer: string[] = [];
  private readonly down = new Set<string>();

  attach(canvas: HTMLCanvasElement): void {
    canvas.addEventListener('mousemove', (e) => {
      const rect = canvas.getBoundingClientRect();
      this.mouseScreen = { x: e.clientX - rect.left, y: e.clientY - rect.top };
    });
    canvas.addEventListener('mousedown', (e) => {
      const rect = canvas.getBoundingClientRect();
      this.clickBuffer.push({
        x: e.clientX - rect.left,
        y: e.clientY - rect.top,
        button: e.button,
      });
      e.preventDefault();
    });
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    window.addEventListener('keydown', (e) => {
      if (!this.down.has(e.key)) this.keyBuffer.push(e.key.toLowerCase());
      this.down.add(e.key);
    });
    window.addEventListener('keyup', (e) => this.down.delete(e.key));
  }

  /** Drain buffered clicks since last frame. */
  takeClicks(): { x: number; y: number; button: number }[] {
    const c = this.clickBuffer;
    this.clickBuffer = [];
    return c;
  }

  /** Drain buffered key presses since last frame. */
  takeKeys(): string[] {
    const k = this.keyBuffer;
    this.keyBuffer = [];
    return k;
  }

  isDown(key: string): boolean {
    return this.down.has(key);
  }
}

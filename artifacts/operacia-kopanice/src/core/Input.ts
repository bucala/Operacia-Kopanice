type PointerState = { x: number; y: number; startX: number; startY: number; cancelled: boolean };

/** One action per tap/click. Pinch, drag, cancellation and focus loss never move a unit. */
export class Input {
  mouseScreen = { x: 0, y: 0 };
  private clickBuffer: { x: number; y: number; button: number }[] = [];
  private keyBuffer: string[] = [];
  private readonly down = new Set<string>();
  private readonly pointers = new Map<number, PointerState>();
  private wheelDelta = 0;
  private listeners?: AbortController;

  attach(canvas: HTMLCanvasElement): void {
    this.detach();
    this.listeners = new AbortController();
    const signal = this.listeners.signal;
    const point = (e: PointerEvent) => {
      const rect = canvas.getBoundingClientRect();
      return {
        x: (e.clientX - rect.left) * canvas.width / Math.max(1, rect.width),
        y: (e.clientY - rect.top) * canvas.height / Math.max(1, rect.height),
      };
    };
    const distance = () => {
      const [a, b] = [...this.pointers.values()];
      return a && b ? Math.hypot(a.x - b.x, a.y - b.y) : 0;
    };
    canvas.addEventListener('pointerdown', (e) => {
      if (e.button !== 0) return;
      e.preventDefault();
      this.mouseScreen = point(e);
      this.pointers.set(e.pointerId, {
        x: e.clientX, y: e.clientY, startX: e.clientX, startY: e.clientY, cancelled: false,
      });
      if (this.pointers.size > 1) {
        for (const p of this.pointers.values()) p.cancelled = true;
      }
      canvas.setPointerCapture(e.pointerId);
    }, { signal });
    canvas.addEventListener('pointermove', (e) => {
      this.mouseScreen = point(e);
      const p = this.pointers.get(e.pointerId);
      if (!p) return;
      const before = distance();
      p.x = e.clientX; p.y = e.clientY;
      if (Math.hypot(p.x - p.startX, p.y - p.startY) > 10) p.cancelled = true;
      const after = distance();
      if (this.pointers.size === 2 && before > 1 && after > 1) {
        this.wheelDelta -= Math.log(after / before) * 80 / Math.log(1.12);
      }
    }, { signal });
    canvas.addEventListener('pointerup', (e) => {
      const p = this.pointers.get(e.pointerId);
      if (p && !p.cancelled && this.pointers.size === 1 &&
          Math.hypot(e.clientX - p.startX, e.clientY - p.startY) <= 10) {
        this.clickBuffer.push({ ...point(e), button: 0 });
      }
      this.pointers.delete(e.pointerId);
      if (canvas.hasPointerCapture(e.pointerId)) canvas.releasePointerCapture(e.pointerId);
    }, { signal });
    const cancel = (e: PointerEvent) => this.pointers.delete(e.pointerId);
    canvas.addEventListener('pointercancel', cancel, { signal });
    canvas.addEventListener('lostpointercapture', cancel, { signal });
    canvas.addEventListener('contextmenu', (e) => e.preventDefault(), { signal });
    canvas.addEventListener('wheel', (e) => {
      e.preventDefault();
      this.wheelDelta += e.deltaY;
    }, { passive: false, signal });
    window.addEventListener('keydown', (e) => {
      const target = e.target as HTMLElement | null;
      if (target?.isContentEditable || /^(INPUT|TEXTAREA|SELECT)$/.test(target?.tagName ?? '')) return;
      const key = e.key.toLowerCase();
      if (['arrowup', 'arrowdown', 'arrowleft', 'arrowright', ' '].includes(key)) e.preventDefault();
      if (!this.down.has(key)) this.keyBuffer.push(key);
      this.down.add(key);
    }, { signal });
    window.addEventListener('keyup', (e) => this.down.delete(e.key.toLowerCase()), { signal });
    window.addEventListener('blur', () => this.reset(), { signal });
  }

  private reset(): void {
    this.down.clear(); this.pointers.clear();
    this.clickBuffer = []; this.keyBuffer = []; this.wheelDelta = 0;
  }
  detach(): void {
    this.listeners?.abort();
    this.listeners = undefined;
    this.reset();
  }
  takeClicks(): { x: number; y: number; button: number }[] {
    const clicks = this.clickBuffer; this.clickBuffer = []; return clicks;
  }
  takeKeys(): string[] {
    const keys = this.keyBuffer; this.keyBuffer = []; return keys;
  }
  takeWheel(): number {
    const delta = this.wheelDelta; this.wheelDelta = 0; return delta;
  }
  isDown(key: string): boolean { return this.down.has(key.toLowerCase()); }
}

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { Input } from '../src/core/Input';

class Canvas extends EventTarget {
  width = 800;
  height = 600;
  captured = new Set<number>();
  getBoundingClientRect() { return { left: 10, top: 20, width: 400, height: 300 }; }
  setPointerCapture(id: number) { this.captured.add(id); }
  hasPointerCapture(id: number) { return this.captured.has(id); }
  releasePointerCapture(id: number) { this.captured.delete(id); }
}
function send(target: EventTarget, type: string, props: object = {}) {
  const event = Object.assign(new Event(type, { cancelable: true }), props);
  target.dispatchEvent(event);
  return event;
}
describe('cross-platform input', () => {
  let canvas: Canvas;
  let input: Input;
  let win: EventTarget;
  beforeEach(() => {
    canvas = new Canvas(); input = new Input(); win = new EventTarget();
    vi.stubGlobal('window', win);
    input.attach(canvas as unknown as HTMLCanvasElement);
  });
  afterEach(() => { input.detach(); vi.unstubAllGlobals(); });
  function pointer(type: string, id = 1, x = 60, y = 70, button = 0) {
    send(canvas, type, { pointerId: id, clientX: x, clientY: y, button });
  }
  it('converts a tap to exactly one correctly scaled canvas click', () => {
    pointer('pointerdown');
    expect(input.takeClicks()).toEqual([]);
    pointer('pointerup');
    expect(input.takeClicks()).toEqual([{ x: 100, y: 100, button: 0 }]);
    expect(input.takeClicks()).toEqual([]);
  });
  it('does not move after dragging away and back', () => {
    pointer('pointerdown'); pointer('pointermove', 1, 100, 70); pointer('pointerup');
    expect(input.takeClicks()).toEqual([]);
  });
  it('pinches to zoom without issuing a move on either release', () => {
    pointer('pointerdown', 1, 50, 70); pointer('pointerdown', 2, 100, 70);
    pointer('pointermove', 2, 150, 70);
    expect(input.takeWheel()).toBeLessThan(0);
    pointer('pointerup', 2, 150, 70); pointer('pointerup', 1, 50, 70);
    expect(input.takeClicks()).toEqual([]);
    expect(input.takeWheel()).toBe(0);
  });
  it('ignores cancellation, lost capture and right clicks', () => {
    pointer('pointerdown'); pointer('pointercancel'); pointer('pointerup');
    pointer('pointerdown'); pointer('lostpointercapture'); pointer('pointerup');
    pointer('pointerdown', 1, 60, 70, 2); pointer('pointerup', 1, 60, 70, 2);
    expect(input.takeClicks()).toEqual([]);
  });
  it('deduplicates held keys and clears input on focus loss', () => {
    send(win, 'keydown', { key: 'W' }); send(win, 'keydown', { key: 'w' });
    expect(input.takeKeys()).toEqual(['w']);
    send(win, 'blur');
    expect(input.isDown('w')).toBe(false);
    send(win, 'keydown', { key: 'w' });
    expect(input.takeKeys()).toEqual(['w']);
  });
  it('reattaches without duplicate listeners and detaches cleanly', () => {
    input.attach(canvas as unknown as HTMLCanvasElement);
    pointer('pointerdown'); pointer('pointerup');
    expect(input.takeClicks()).toHaveLength(1);
    input.detach();
    pointer('pointerdown'); pointer('pointerup');
    send(win, 'keydown', { key: 'w' });
    expect(input.takeClicks()).toEqual([]); expect(input.takeKeys()).toEqual([]);
  });
});

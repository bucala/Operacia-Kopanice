/**
 * Lightweight image preloader for the GO renderer.
 *
 * Call `preload([...srcs])` once at startup; thereafter `get(src)` returns the
 * loaded HTMLImageElement synchronously (or null if still loading / failed).
 * The renderer draws with whatever images are ready and silently falls back to
 * geometric shapes for anything still pending.
 */
export class SpriteCache {
  private readonly cache = new Map<string, HTMLImageElement | null>();

  /**
   * Kick off loading for every src in the list. Returns a Promise that resolves
   * when all images have either loaded or errored (never rejects).
   */
  preload(srcs: string[]): Promise<void> {
    const pending = srcs.filter((s) => !this.cache.has(s));
    if (!pending.length) return Promise.resolve();
    const ps = pending.map((src) => this.loadOne(src));
    return Promise.all(ps).then(() => undefined);
  }

  private loadOne(src: string): Promise<void> {
    this.cache.set(src, null); // mark as in-flight
    return new Promise<void>((resolve) => {
      const img = new Image();
      img.onload = () => {
        this.cache.set(src, img);
        resolve();
      };
      img.onerror = () => {
        // Keep null so get() returns null → renderer falls back to shapes.
        resolve();
      };
      img.src = src;
    });
  }

  /** Returns the image if loaded, null otherwise. */
  get(src: string): HTMLImageElement | null {
    return this.cache.get(src) ?? null;
  }
}

/**
 * Lazy image loader for high-fidelity sprite/prop art.
 *
 * Images live under public/assets (served at runtime). The store loads each
 * source once, caches the decoded `HTMLImageElement`, and reports whether it is
 * ready yet. The RenderSystem calls {@link ready} every frame and falls back to
 * procedural art until the image has loaded — so a missing file never breaks
 * rendering, it just keeps the procedural look.
 */
export class ImageStore {
  private readonly cache = new Map<string, HTMLImageElement>();
  private readonly loaded = new Set<string>();
  private readonly failed = new Set<string>();
  /** Allowlist of source paths known to exist (from the manifest). */
  private available: Set<string> | null = null;

  private resolve(src: string): string {
    return `assets/${src}`;
  }

  /**
   * Restrict loading to a known set of files (the sprite manifest). This keeps
   * the console free of 404s for art that has not been added yet — only listed
   * files are ever requested. Pass `null` to load anything on demand.
   */
  setAvailable(sources: string[] | null): void {
    this.available = sources ? new Set(sources) : null;
  }

  /** Begin loading `src` if not already started. Safe to call repeatedly. */
  request(src: string): void {
    if (typeof Image === 'undefined') return; // non-browser (tests)
    if (this.available && !this.available.has(src)) return; // not yet added
    if (this.cache.has(src) || this.failed.has(src)) return;
    const img = new Image();
    img.onload = () => this.loaded.add(src);
    img.onerror = () => {
      this.failed.add(src);
      this.cache.delete(src);
    };
    img.src = this.resolve(src);
    this.cache.set(src, img);
  }

  /** Kick off loading for a batch of sources. */
  preload(sources: Iterable<string>): void {
    for (const src of sources) this.request(src);
  }

  /** The decoded image if it is loaded and ready to draw, else null. */
  ready(src: string): HTMLImageElement | null {
    if (!this.cache.has(src)) {
      this.request(src);
      return null;
    }
    return this.loaded.has(src) ? this.cache.get(src)! : null;
  }
}

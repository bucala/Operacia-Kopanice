/** Minimal binary min-heap used as the A* open set. */
export class BinaryHeap<T> {
  private readonly items: T[] = [];

  constructor(private readonly score: (item: T) => number) {}

  get size(): number {
    return this.items.length;
  }

  push(item: T): void {
    this.items.push(item);
    this.bubbleUp(this.items.length - 1);
  }

  pop(): T | undefined {
    const top = this.items[0];
    const end = this.items.pop();
    if (this.items.length > 0 && end !== undefined) {
      this.items[0] = end;
      this.sinkDown(0);
    }
    return top;
  }

  private bubbleUp(n: number): void {
    const item = this.items[n];
    const score = this.score(item);
    while (n > 0) {
      const parentN = (n - 1) >> 1;
      const parent = this.items[parentN];
      if (score >= this.score(parent)) break;
      this.items[parentN] = item;
      this.items[n] = parent;
      n = parentN;
    }
  }

  private sinkDown(n: number): void {
    const length = this.items.length;
    const item = this.items[n];
    const score = this.score(item);
    for (;;) {
      const child2N = (n + 1) << 1;
      const child1N = child2N - 1;
      let swap = -1;
      let child1Score = 0;
      if (child1N < length) {
        child1Score = this.score(this.items[child1N]);
        if (child1Score < score) swap = child1N;
      }
      if (child2N < length) {
        const child2Score = this.score(this.items[child2N]);
        if (child2Score < (swap === -1 ? score : child1Score)) swap = child2N;
      }
      if (swap === -1) break;
      this.items[n] = this.items[swap];
      this.items[swap] = item;
      n = swap;
    }
  }
}

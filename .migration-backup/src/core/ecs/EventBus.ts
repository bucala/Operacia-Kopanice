/**
 * Minimal typed publish/subscribe bus. Systems communicate through events
 * (e.g. a sound is emitted, an enemy is alerted, a skill fires) instead of
 * referencing each other directly.
 */
export type EventHandler<T> = (payload: T) => void;

export class EventBus {
  private readonly handlers = new Map<string, Set<EventHandler<unknown>>>();

  on<T>(event: string, handler: EventHandler<T>): () => void {
    let set = this.handlers.get(event);
    if (!set) {
      set = new Set();
      this.handlers.set(event, set);
    }
    set.add(handler as EventHandler<unknown>);
    return () => set!.delete(handler as EventHandler<unknown>);
  }

  emit<T>(event: string, payload: T): void {
    const set = this.handlers.get(event);
    if (!set) return;
    for (const h of set) (h as EventHandler<T>)(payload);
  }

  clear(): void {
    this.handlers.clear();
  }
}

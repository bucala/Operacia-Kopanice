import { type Component, kind } from '@/core/ecs/Component';

export interface ItemStack {
  id: string;
  name: string;
  qty: number;
  /** Weight per unit. */
  weight: number;
}

/** A weight-limited container of item stacks. */
export interface Inventory extends Component {
  type: 'inventory';
  items: ItemStack[];
  maxWeight: number;
}

export const Inventory = kind<Inventory>('inventory');

export function makeInventory(maxWeight = 30, items: ItemStack[] = []): Inventory {
  return { type: 'inventory', items, maxWeight };
}

export function totalWeight(inv: Inventory): number {
  return inv.items.reduce((sum, s) => sum + s.weight * s.qty, 0);
}

export function addItem(inv: Inventory, stack: ItemStack): boolean {
  if (totalWeight(inv) + stack.weight * stack.qty > inv.maxWeight) return false;
  const existing = inv.items.find((s) => s.id === stack.id);
  if (existing) existing.qty += stack.qty;
  else inv.items.push({ ...stack });
  return true;
}

export function removeItem(inv: Inventory, id: string, qty = 1): boolean {
  const existing = inv.items.find((s) => s.id === id);
  if (!existing || existing.qty < qty) return false;
  existing.qty -= qty;
  if (existing.qty <= 0) inv.items = inv.items.filter((s) => s.id !== id);
  return true;
}

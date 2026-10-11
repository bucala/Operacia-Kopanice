import { type Order, type Unit, type Mission, distance } from "./types";

export function orderName(order: Order, mission: Mission): string {
  switch (order.kind) {
    case "move":
      return order.run ? "Beh" : "Presun";
    case "takedown":
      return "Tiche zneskodnenie";
    case "distract":
      return "Odlakanie";
    case "carry":
      return "Preniest telo";
    case "interact":
      if (distance(order.position, mission.supply) < 1.1)
        return mission.goal === "documents" ? "Ziskat dokumenty" : "Ziskat TNT";
      if (distance(order.position, mission.target) < 1.1)
        return mission.goal === "bridge"
          ? "Odpalit most"
          : "Sabotaz velitelstva";
      if (distance(order.position, mission.hide) < 1.1) return "Ukryt telo";
      return "Interakcia";
  }
}
export function orderState(unit: Unit, index: number, paused: boolean): string {
  return index === 0 && unit.started
    ? paused
      ? "Pozastavene"
      : "Vykonava sa"
    : "Caka";
}
export function orderPage(count: number, requested: number) {
  const pages = Math.max(1, Math.ceil(count / 5));
  const page = Math.max(0, Math.min(pages - 1, Math.floor(requested)));
  return { page, pages, start: page * 5 };
}

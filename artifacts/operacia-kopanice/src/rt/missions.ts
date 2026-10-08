import { type Mission, type Prop, type Stop, distance } from "./types";

const hut = (x: number, y: number, alternate = false): Prop => ({
  x,
  y,
  kind: alternate ? "house2" : "house1",
  width: 7.2,
  height: 7.2,
});
const crate = (x: number, y: number): Prop => ({
  x,
  y,
  kind: "crate",
  width: 1,
  height: 1,
});
const stop = (x: number, y: number, degrees: number, wait: number): Stop => ({
  x,
  y,
  facing: (degrees * Math.PI) / 180,
  wait,
});

function forest(m: Mission): Mission {
  let seed = 101 + m.id * 907;
  const random = () => {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    return seed / 4294967296;
  };
  const clear = [
    m.start,
    m.supply,
    m.target,
    m.exit,
    m.hide,
    ...m.patrols.flat(),
  ];
  for (let i = 0; i < 220; i++) {
    const p = { x: random() * m.width, y: random() * m.height };
    if (
      clear.some((c) => distance(c, p) < 3) ||
      m.props.some(
        (c) =>
          Math.abs(c.x - p.x) < c.width / 2 + 1 &&
          Math.abs(c.y - p.y) < c.height / 2 + 1,
      )
    )
      continue;
    // Preserve the western bypass, bridge approaches and supply service lanes.
    if (
      p.x < 7 ||
      Math.abs(p.y - m.supply.y) < 1.5 ||
      (m.river && Math.abs(p.y - m.river.bridgeY) < 2)
    )
      continue;
    if (m.river && Math.abs(p.x - m.river.x) < m.river.width / 2 + 1) continue;
    const kind = i % 9 === 0 ? "rock" : i % 7 === 0 ? "shrub" : "tree";
    m.props.push({
      ...p,
      kind,
      width: kind === "tree" ? 0.55 : 1.3,
      height: kind === "tree" ? 0.55 : 1.3,
    });
  }
  m.props.push({
    x: m.hide.x,
    y: m.hide.y,
    kind: "shrub",
    width: 2,
    height: 2,
  });
  return m;
}

export const MISSIONS: Mission[] = [
  forest({
    id: 0,
    name: "Zimny viadukt",
    width: 25,
    height: 20,
    goal: "bridge",
    briefing:
      "Ziskajte TNT, presunte oboch clenov cez most a aktivujte detonator z vychodneho brehu. Potom ustupte k vychodu.",
    start: { x: 3, y: 13.8 },
    supply: { x: 5.4, y: 12.6 },
    target: { x: 14.4, y: 7.2 },
    exit: { x: 16.2, y: 1.8 },
    hide: { x: 5.4, y: 14.4 },
    river: { x: 10.8, width: 2.3, bridgeY: 7.2, bridgeWidth: 2.6 },
    props: [
      hut(4.5, 2.7),
      hut(22, 3, true),
      hut(22, 16.5),
      crate(6.3, 14.4),
      { x: 15, y: 17, kind: "vehicle", width: 5.4, height: 3.6 },
    ],
    patrols: [
      [stop(6.6, 7, -90, 12), stop(8.4, 7, 0, 2), stop(8.4, 7, -90, 1)],
      [stop(17.4, 12, 90, 8), stop(17.4, 10.2, 180, 2), stop(17.4, 10.2, 0, 1)],
    ],
  }),
  forest({
    id: 1,
    name: "Lesny kurier",
    width: 48,
    height: 45,
    goal: "documents",
    briefing:
      "Preniknite do lesneho tabora, ziskajte dokumenty a dostante oboch clenov timu k vychodu. Zapadny les ponuka obchadzku.",
    start: { x: 5, y: 37 },
    supply: { x: 13, y: 13 },
    target: { x: 13, y: 13 },
    exit: { x: 32, y: 5 },
    hide: { x: 6, y: 35 },
    props: [
      hut(17, 9, true),
      hut(28, 22),
      hut(36, 31, true),
      hut(20, 34),
      crate(14.5, 15),
      { x: 34, y: 23, kind: "vehicle", width: 5.4, height: 3.6 },
    ],
    patrols: [
      [stop(23, 13, 0, 8), stop(23, 18, 90, 3)],
      [stop(15, 26, 0, 9), stop(25, 26, 180, 3)],
      [stop(38, 35, 90, 8), stop(38, 27, 180, 2)],
    ],
  }),
  forest({
    id: 2,
    name: "Tiche velitelstvo",
    width: 48,
    height: 48,
    goal: "command",
    briefing:
      "Ziskajte TNT pri juznom sklade a vyradte velitelske stanoviste. Hluk sabotaze prilaka hliadky. Ustupte spolocne na severozapad.",
    start: { x: 5, y: 39 },
    supply: { x: 8, y: 35 },
    target: { x: 26, y: 16 },
    exit: { x: 6, y: 6 },
    hide: { x: 6, y: 37 },
    props: [
      hut(14, 34, true),
      hut(21, 27),
      hut(36, 10, true),
      hut(37, 35),
      hut(18, 6),
      crate(10, 32),
      { x: 31, y: 16, kind: "vehicle", width: 9, height: 3.6 },
    ],
    patrols: [
      [stop(28, 24, 90, 8), stop(28, 30, 0, 2)],
      [stop(38, 18, 0, 10), stop(38, 24, 90, 3)],
      [stop(23, 9, 0, 8), stop(28, 9, 180, 2)],
    ],
  }),
];

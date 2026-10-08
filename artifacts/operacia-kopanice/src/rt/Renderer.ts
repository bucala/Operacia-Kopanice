import { SpriteCache } from "../go/SpriteCache";
import { Simulation, SIGHT } from "./Simulation";
import { type Point, type Prop, type Unit, distance } from "./types";

const ROOT = "/assets/sprites/";
export class Renderer {
  readonly images = new SpriteCache();
  width = 1;
  height = 1;
  scale = 28;
  yaw = 0;
  center: Point = { x: 0, y: 0 };
  hover?: Point;
  preview: Point[] | null = null;
  cones = true;
  paths = true;
  selection?: { from: Point; to: Point };
  private ground = document.createElement("canvas");
  private groundKey = "";
  constructor(
    readonly canvas: HTMLCanvasElement,
    public sim: Simulation,
  ) {
    void this.images.preload(
      [
        "house1",
        "house2",
        "trees",
        "player",
        "guard-soldier",
        "guard-officer",
      ].map((s) => ROOT + s + ".png"),
    );
    this.fit();
  }
  resize(w: number, h: number): void {
    this.width = w;
    this.height = h;
    const dpr = Math.min(devicePixelRatio || 1, 2);
    this.canvas.width = Math.round(w * dpr);
    this.canvas.height = Math.round(h * dpr);
    this.canvas.style.width = w + "px";
    this.canvas.style.height = h + "px";
    this.groundKey = "";
  }
  fit(): void {
    const m = this.sim.mission;
    this.center = { x: m.width / 2, y: m.height / 2 };
    this.scale = Math.max(
      10,
      Math.min(
        (innerWidth - 80) / (m.width + m.height),
        (innerHeight - 180) / ((m.width + m.height) * 0.5),
        34,
      ),
    );
  }
  focus(): void {
    const p = this.sim.party[this.sim.active];
    this.center = { x: p.x, y: p.y };
  }
  project(p: Point): Point {
    const dx = p.x - this.center.x,
      dy = p.y - this.center.y,
      c = Math.cos(this.yaw),
      s = Math.sin(this.yaw);
    const x = dx * c - dy * s,
      y = dx * s + dy * c;
    return {
      x: this.width / 2 + (x - y) * this.scale,
      y: this.height / 2 + (x + y) * this.scale * 0.5,
    };
  }
  unproject(p: Point): Point {
    const u = (p.x - this.width / 2) / this.scale,
      v = (p.y - this.height / 2) / (this.scale * 0.5),
      c = Math.cos(this.yaw),
      s = Math.sin(this.yaw);
    const x = (u + v) / 2,
      y = (v - u) / 2;
    return {
      x: this.center.x + x * c + y * s,
      y: this.center.y - x * s + y * c,
    };
  }
  pan(dx: number, dy: number): void {
    const a = this.unproject({ x: 0, y: 0 }),
      b = this.unproject({ x: dx, y: dy });
    this.center.x -= b.x - a.x;
    this.center.y -= b.y - a.y;
  }
  zoom(
    factor: number,
    anchor: Point = { x: this.width / 2, y: this.height / 2 },
  ): void {
    const before = this.unproject(anchor);
    this.scale = Math.max(9, Math.min(110, this.scale * factor));
    const after = this.unproject(anchor);
    this.center.x += before.x - after.x;
    this.center.y += before.y - after.y;
  }
  pick(screen: Point): Unit | undefined {
    return [...this.sim.units]
      .filter((u) => !u.hidden && !u.carriedBy)
      .sort((a, b) => this.project(b).y - this.project(a).y)
      .find((u) => {
        const p = this.project(u);
        return (
          Math.abs(p.x - screen.x) < Math.max(12, this.scale * 0.45) &&
          screen.y > p.y - this.scale * 1.85 &&
          screen.y < p.y + 10
        );
      });
  }
  private polygon(
    ctx: CanvasRenderingContext2D,
    points: Point[],
    fill: string,
    stroke?: string,
  ): void {
    ctx.beginPath();
    points.forEach((p, i) => {
      const q = this.project(p);
      i ? ctx.lineTo(q.x, q.y) : ctx.moveTo(q.x, q.y);
    });
    ctx.closePath();
    ctx.fillStyle = fill;
    ctx.fill();
    if (stroke) {
      ctx.strokeStyle = stroke;
      ctx.stroke();
    }
  }
  private ring(
    ctx: CanvasRenderingContext2D,
    p: Point,
    r: number,
    color: string,
  ): void {
    const q = this.project(p);
    ctx.beginPath();
    ctx.ellipse(
      q.x,
      q.y,
      r * this.scale,
      r * this.scale * 0.5,
      0,
      0,
      Math.PI * 2,
    );
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.stroke();
  }
  private line(
    ctx: CanvasRenderingContext2D,
    points: Point[],
    color: string,
    width = 2,
  ): void {
    ctx.beginPath();
    points.forEach((p, i) => {
      const q = this.project(p);
      i ? ctx.lineTo(q.x, q.y) : ctx.moveTo(q.x, q.y);
    });
    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.stroke();
  }
  private terrain(ctx: CanvasRenderingContext2D): void {
    const m = this.sim.mission;
    ctx.fillStyle = "#c0cfd7";
    ctx.fillRect(0, 0, this.width, this.height);
    this.polygon(
      ctx,
      [
        { x: 0, y: 0 },
        { x: m.width, y: 0 },
        { x: m.width, y: m.height },
        { x: 0, y: m.height },
      ],
      "#dce5e9",
    );
    // Irregular snow patches and cobbles, not playable cells or grid overlays.
    for (let i = 0; i < 470; i++) {
      const x = (((i * 7919) % 1009) / 1009) * m.width,
        y = (((i * 4441) % 1013) / 1013) * m.height,
        q = this.project({ x, y });
      ctx.beginPath();
      ctx.ellipse(
        q.x,
        q.y,
        this.scale * (0.2 + (i % 7) * 0.09),
        this.scale * 0.1,
        0,
        0,
        Math.PI * 2,
      );
      ctx.fillStyle = i % 3 ? "rgba(160,184,199,.12)" : "rgba(255,255,255,.3)";
      ctx.fill();
    }
    const roads = m.river
      ? [
          [
            { x: 1, y: 7.2 },
            { x: m.width - 1, y: 7.2 },
          ],
          [
            { x: 15, y: 2 },
            { x: 15, y: 18 },
          ],
        ]
      : [[m.start, m.supply, m.target, m.exit]];
    for (const road of roads) {
      this.line(ctx, road, "rgba(179,193,198,.28)", this.scale * 3.3);
      this.line(ctx, road, "#b5bec1", this.scale * 2.3);
      for (let s = 0; s < road.length - 1; s++) {
        const a = road[s],
          b = road[s + 1],
          length = distance(a, b),
          dx = (b.x - a.x) / length,
          dy = (b.y - a.y) / length;
        for (let t = 0; t < length; t += 0.33)
          for (let row = -2; row <= 2; row++) {
            const p = {
              x: a.x + dx * t - dy * row * 0.34,
              y: a.y + dy * t + dx * row * 0.34,
            };
            if (
              m.river &&
              Math.abs(p.x - this.sim.nav.riverX(p.y)) < m.river.width / 2 + 0.2
            )
              continue;
            this.polygon(
              ctx,
              [
                { x: p.x - 0.13, y: p.y - 0.13 },
                { x: p.x + 0.13, y: p.y - 0.13 },
                { x: p.x + 0.13, y: p.y + 0.13 },
                { x: p.x - 0.13, y: p.y + 0.13 },
              ],
              t % 1 > 0.5 ? "#9ca9b1" : "#bac3c8",
              "#d7e0e4",
            );
          }
      }
    }
    if (m.river) {
      const r = m.river,
        left: Point[] = [],
        right: Point[] = [];
      for (let y = 0; y <= m.height; y += 0.25) {
        const x = this.sim.nav.riverX(y);
        left.push({ x: x - r.width / 2 - 0.4, y });
        right.unshift({ x: x + r.width / 2 + 0.4, y });
      }
      this.polygon(ctx, [...left, ...right], "#afc5cf");
      this.polygon(
        ctx,
        [
          ...left.map((p) => ({ ...p, x: p.x + 0.4 })),
          ...right.map((p) => ({ ...p, x: p.x - 0.4 })),
        ],
        "#527e8b",
      );
      if (!this.sim.nav.bridgeDestroyed)
        for (
          let y = r.bridgeY - r.bridgeWidth / 2;
          y < r.bridgeY + r.bridgeWidth / 2;
          y += 0.2
        ) {
          this.polygon(
            ctx,
            [
              { x: r.x - r.width / 2 - 0.7, y },
              { x: r.x + r.width / 2 + 0.7, y },
              { x: r.x + r.width / 2 + 0.7, y: y + 0.18 },
              { x: r.x - r.width / 2 - 0.7, y: y + 0.18 },
            ],
            "#777a76",
            "#a5abad",
          );
        }
    }
  }
  private prop(ctx: CanvasRenderingContext2D, p: Prop, index: number): void {
    const q = this.project(p),
      s = this.scale;
    if (p.kind === "house1" || p.kind === "house2") {
      const image = this.images.get(ROOT + p.kind + ".png"),
        w = p.width * s * 1.85;
      if (image) ctx.drawImage(image, q.x - w / 2, q.y - w * 0.82, w, w);
      else
        this.polygon(
          ctx,
          [
            { x: p.x - p.width / 2, y: p.y - p.height / 2 },
            { x: p.x + p.width / 2, y: p.y - p.height / 2 },
            { x: p.x + p.width / 2, y: p.y + p.height / 2 },
            { x: p.x - p.width / 2, y: p.y + p.height / 2 },
          ],
          "#626e73",
        );
    } else if (p.kind === "tree") {
      const image = this.images.get(ROOT + "trees.png"),
        w = s * (index % 3 === 0 ? 3.9 : 3),
        h = w * 1.7;
      const crop =
        index % 2 ? [0.236, 0.078, 0.145, 0.285] : [0.02, 0.028, 0.199, 0.32];
      if (image)
        ctx.drawImage(
          image,
          image.width * crop[0],
          image.height * crop[1],
          image.width * crop[2],
          image.height * crop[3],
          q.x - w / 2,
          q.y - h,
          w,
          h,
        );
    } else if (p.kind === "vehicle") {
      this.polygon(
        ctx,
        [
          { x: p.x - p.width / 2, y: p.y - p.height / 2 },
          { x: p.x + p.width / 2, y: p.y - p.height / 2 },
          { x: p.x + p.width / 2, y: p.y + p.height / 2 },
          { x: p.x - p.width / 2, y: p.y + p.height / 2 },
        ],
        "#505e60",
        "#83919a",
      );
      ctx.fillStyle = "#d9e4e8";
      ctx.fillRect(q.x - s, q.y - s * 1.1, s * 2, s * 0.5);
    } else {
      ctx.fillStyle =
        p.kind === "shrub"
          ? "#8b9b96"
          : p.kind === "rock"
            ? "#8c9ba5"
            : "#796f60";
      ctx.beginPath();
      ctx.ellipse(
        q.x,
        q.y - s * 0.2,
        s * p.width * 0.6,
        s * p.height * 0.4,
        0,
        0,
        Math.PI * 2,
      );
      ctx.fill();
      ctx.fillStyle = "#e4ecee";
      ctx.beginPath();
      ctx.ellipse(
        q.x - s * 0.07,
        q.y - s * 0.32,
        s * p.width * 0.45,
        s * p.height * 0.2,
        0,
        Math.PI,
        Math.PI * 2,
      );
      ctx.fill();
    }
  }
  private person(ctx: CanvasRenderingContext2D, u: Unit): void {
    if (u.hidden || u.carriedBy) return;
    const q = this.project(u),
      s = this.scale;
    const image = this.images.get(
      ROOT +
        (u.enemy
          ? "guard-soldier"
          : u.id === "officer"
            ? "guard-officer"
            : "player") +
        ".png",
    );
    ctx.save();
    if (u.health <= 0) {
      ctx.translate(q.x, q.y);
      ctx.rotate(Math.PI / 2);
      ctx.globalAlpha = 0.75;
    }
    const x = u.health > 0 ? q.x : 0,
      y = u.health > 0 ? q.y : 0;
    ctx.fillStyle = "rgba(38,60,72,.23)";
    ctx.beginPath();
    ctx.ellipse(x, y, s * 0.4, s * 0.2, 0, 0, Math.PI * 2);
    ctx.fill();
    const h =
        s * (u.stance === "prone" ? 0.7 : u.stance === "crouch" ? 1.35 : 1.85),
      w = h * 0.52;
    // Crop away the source display pedestal; character height is world-scaled.
    if (image)
      ctx.drawImage(
        image,
        image.width * 0.32,
        image.height * 0.04,
        image.width * 0.4,
        image.height * 0.7,
        x - w / 2,
        y - h,
        w,
        h,
      );
    else {
      ctx.fillStyle = u.enemy ? "#423f3a" : "#52644e";
      ctx.fillRect(x - w / 3, y - h, w * 0.65, h);
    }
    ctx.restore();
    if (u.health <= 0) return;
    if (u.selected && !u.enemy) this.ring(ctx, u, 0.5, "#e8cc78");
    if (!u.enemy) {
      ctx.font = "bold 12px system-ui";
      ctx.fillStyle = "#1f3037";
      ctx.textAlign = "center";
      ctx.fillText(u.id === "partisan" ? "1" : "2", q.x, q.y - s * 2 - 5);
      ctx.fillStyle = "#344951";
      ctx.fillRect(q.x - 15, q.y - s * 2, 30, 3);
      ctx.fillStyle = "#79a18b";
      ctx.fillRect(q.x - 15, q.y - s * 2, (30 * u.health) / 100, 3);
    } else if (u.suspicion > 0 || u.alert !== "patrol") {
      ctx.fillStyle = "#3c4549";
      ctx.fillRect(q.x - 18, q.y - s * 2, 36, 5);
      ctx.fillStyle = u.alert === "combat" ? "#c54e4e" : "#d0ac63";
      ctx.fillRect(
        q.x - 18,
        q.y - s * 2,
        36 * Math.max(u.suspicion, u.alert === "investigate" ? 0.25 : 0),
        5,
      );
    }
  }
  private cone(
    ctx: CanvasRenderingContext2D,
    g: Unit,
    radius: number,
    half: number,
    color: string,
  ): void {
    const points: Point[] = [g];
    for (let a = -half; a <= half + 0.001; a += half / 20) {
      const heading = g.facing + a;
      let r = 0.15;
      for (; r < radius; r += 0.2)
        if (
          this.sim.nav.blocked(
            { x: g.x + Math.cos(heading) * r, y: g.y + Math.sin(heading) * r },
            0,
            true,
          )
        )
          break;
      points.push({
        x: g.x + Math.cos(heading) * Math.min(radius, r - 0.1),
        y: g.y + Math.sin(heading) * Math.min(radius, r - 0.1),
      });
    }
    this.polygon(ctx, points, color);
  }
  draw(): void {
    const ctx = this.canvas.getContext("2d")!,
      dpr = this.canvas.width / this.width,
      m = this.sim.mission;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, this.width, this.height);
    const key = [
      m.id,
      this.width,
      this.height,
      this.scale,
      this.yaw,
      this.center.x,
      this.center.y,
      this.sim.nav.bridgeDestroyed,
    ].join(":");
    if (key !== this.groundKey) {
      this.ground.width = this.canvas.width;
      this.ground.height = this.canvas.height;
      const g = this.ground.getContext("2d")!;
      g.setTransform(dpr, 0, 0, dpr, 0, 0);
      this.terrain(g);
      this.groundKey = key;
    }
    ctx.drawImage(this.ground, 0, 0, this.width, this.height);
    if (m.river)
      for (let i = 0; i < 50; i++) {
        const y = (i * 0.73 + this.sim.elapsed * 0.6) % m.height,
          x = this.sim.nav.riverX(y) + ((i % 5) - 2) * 0.19;
        if (
          !this.sim.nav.bridgeDestroyed &&
          Math.abs(y - m.river.bridgeY) < m.river.bridgeWidth / 2
        )
          continue;
        this.line(
          ctx,
          [
            { x, y },
            { x: x + 0.04, y: y + 0.32 },
          ],
          "rgba(213,236,239,.32)",
          1,
        );
      }
    if (this.cones)
      for (const g of this.sim.guards)
        if (g.health > 0) {
          this.cone(ctx, g, SIGHT.far, SIGHT.farAngle, "rgba(176,60,59,.12)");
          this.cone(ctx, g, SIGHT.near, SIGHT.nearAngle, "rgba(193,58,54,.2)");
        }
    if (this.paths) {
      for (const p of this.sim.party)
        if (p.selected) {
          if (p.path.length) this.line(ctx, [p, ...p.path], "#f5e3a8", 2);
          if (p.orders.length > 1) {
            ctx.setLineDash([4, 6]);
            this.line(ctx, [p, ...p.orders.map((o) => o.position)], "#8d6b36");
            ctx.setLineDash([]);
          }
        }
      if (this.preview) {
        ctx.setLineDash([4, 5]);
        this.line(ctx, this.preview, "#728e84", 1.5);
        ctx.setLineDash([]);
      }
    }
    const items = [
      ...m.props.map((p, i) => ({
        y: this.project(p).y,
        draw: () => this.prop(ctx, p, i),
      })),
      ...this.sim.units.map((u) => ({
        y: this.project(u).y + 0.1,
        draw: () => this.person(ctx, u),
      })),
    ];
    items.sort((a, b) => a.y - b.y).forEach((o) => o.draw());
    for (const [p, label] of [
      [m.supply, m.goal === "documents" ? "Dokumenty" : "TNT"],
      [m.target, "Sabotaz"],
      [m.exit, "Vychod"],
      [m.hide, "Ukryt"],
    ] as [Point, string][]) {
      if (p === m.target && m.goal === "documents") continue;
      const q = this.project(p);
      this.ring(ctx, p, 0.55, p === m.exit ? "#8ba78b" : "#a58e5a");
      ctx.font = "12px system-ui";
      ctx.textAlign = "center";
      ctx.lineWidth = 3;
      ctx.strokeStyle = "rgba(239,246,247,.9)";
      ctx.strokeText(label, q.x, q.y + 18);
      ctx.fillStyle = "#344249";
      ctx.fillText(label, q.x, q.y + 18);
    }
    this.sim.noises.forEach((n) =>
      this.ring(
        ctx,
        n.position,
        n.radius * Math.min(1, n.age + 0.15),
        "rgba(188,141,75," + (1 - n.age / 1.5) + ")",
      ),
    );
    const f = this.sim.feedback;
    if (f && performance.now() - f.issued < 1400)
      this.ring(
        ctx,
        f.position,
        0.5 + (performance.now() - f.issued) / 2500,
        f.accepted === f.requested && f.accepted > 0
          ? "#4b8a67"
          : f.accepted
            ? "#b58d40"
            : "#c64c48",
      );
    if (this.selection) {
      const { from: a, to: b } = this.selection;
      ctx.strokeStyle = "#af955c";
      ctx.fillStyle = "rgba(210,190,125,.15)";
      ctx.fillRect(a.x, a.y, b.x - a.x, b.y - a.y);
      ctx.strokeRect(a.x, a.y, b.x - a.x, b.y - a.y);
    }
  }
}

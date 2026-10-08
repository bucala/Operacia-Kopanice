const assert = require("node:assert/strict");
const path = require("node:path");
const fs = require("node:fs");
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || "playwright");
const url = process.argv[2] || "http://127.0.0.1:4175";
const output = path.resolve(process.argv[3] || "web-rtt-evidence");
fs.mkdirSync(output, { recursive: true });
const errors = [];
const results = [];
async function pixels(page) {
  const colors = await page.locator("canvas").evaluate((canvas) => {
    const { data } = canvas
      .getContext("2d")
      .getImageData(0, 0, canvas.width, canvas.height);
    const set = new Set();
    for (let i = 0; i < data.length; i += 256)
      set.add(`${data[i]},${data[i + 1]},${data[i + 2]}`);
    return set.size;
  });
  assert(colors > 100, "canvas must contain visible assets, not a blank field");
  return colors;
}
async function launch(page) {
  await page.goto(url, { waitUntil: "networkidle" });
  await page
    .getByRole("button", { name: "Nova operacia", exact: true })
    .click();
  await page.getByRole("button", { name: "Nasadit tim", exact: true }).click();
  await page.locator('[data-state="running"]').waitFor();
  assert.equal(
    await page.getByTestId("realtime-state").textContent(),
    "REALNY CAS",
  );
}
async function snapshot(page) {
  await page.waitForTimeout(120);
  return page.evaluate(() => {
    if (window.__kopanice) return window.__kopanice.snapshot();
    const app = document.getElementById("app");
    return {
      elapsed: Number(app.dataset.elapsed),
      paused: app.dataset.paused === "true",
      menu: app.dataset.menu === "true",
      party: [...document.querySelectorAll(".rt-unit")].map((u) => ({
        x: Number(u.dataset.worldX),
        y: Number(u.dataset.worldY),
        orders: Number(u.dataset.orders),
      })),
      guards: [],
    };
  });
}
async function point(page, p) {
  return page.evaluate((p) => {
    if (window.__kopanice) return window.__kopanice.project(p);
    const scale = Math.max(
      10,
      Math.min((innerWidth - 80) / 45, (innerHeight - 180) / 22.5, 34),
    );
    return {
      x: innerWidth / 2 + (p.x - 12.5 - (p.y - 10)) * scale,
      y: innerHeight / 2 + (p.x - 12.5 + (p.y - 10)) * scale * 0.5,
    };
  }, p);
}
async function checkLayout(page) {
  const problems = await page
    .locator(".rt-hud button,.rt-objective,.rt-message")
    .evaluateAll((nodes) =>
      nodes
        .filter((n) => {
          const r = n.getBoundingClientRect();
          return (
            r.width &&
            (r.left < 0 ||
              r.right > innerWidth + 1 ||
              r.top < 0 ||
              r.bottom > innerHeight + 1 ||
              n.scrollWidth > n.clientWidth + 2)
          );
        })
        .map((n) => n.getAttribute("aria-label") || n.className),
    );
  assert.deepEqual(problems, [], "HUD clipping");
  const overlaps = await page
    .locator(".rt-party,.rt-objective,.rt-controls,.rt-message")
    .evaluateAll((nodes) => {
      const issues = [];
      for (let i = 0; i < nodes.length; i++)
        for (let j = i + 1; j < nodes.length; j++) {
          const a = nodes[i].getBoundingClientRect(),
            b = nodes[j].getBoundingClientRect();
          if (
            Math.min(a.right, b.right) - Math.max(a.left, b.left) > 1 &&
            Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top) > 1
          )
            issues.push(nodes[i].className + " / " + nodes[j].className);
        }
      return issues;
    });
  assert.deepEqual(overlaps, [], "HUD panels must not overlap");
}
(async () => {
  const browser = await chromium.launch({
    headless: true,
    channel: process.env.PLAYWRIGHT_CHANNEL || "chrome",
  });
  try {
    const desktop = await browser.newContext({
      viewport: { width: 1920, height: 1080 },
    });
    const page = await desktop.newPage();
    page.on("pageerror", (e) => errors.push(e.message));
    await launch(page);
    const diagnostics = await page.evaluate(() => !!window.__kopanice);
    {
      const start = await snapshot(page);
      if (diagnostics)
        await page.waitForFunction(
          () => window.__kopanice.snapshot().elapsed > 14.5,
          undefined,
          { timeout: 25000 },
        );
      else await page.waitForTimeout(2000);
      const idle = await snapshot(page);
      if (diagnostics)
        assert(
          idle.guards.some(
            (g, i) =>
              Math.hypot(g.x - start.guards[i].x, g.y - start.guards[i].y) >
                0.3 || Math.abs(g.facing - start.guards[i].facing) > 0.3,
          ),
          "patrols run without input",
        );
      assert(
        idle.elapsed > start.elapsed + 1,
        "world advances without player input",
      );
      const dest = await point(page, { x: 3, y: 18 });
      await page.mouse.click(dest.x, dest.y, { button: "right" });
      await page.waitForTimeout(400);
      const moving = await snapshot(page);
      assert(
        moving.party[0].y > 14 && moving.party[0].y < 18,
        "single world click causes fractional continuous movement",
      );
      await page.keyboard.press("Space");
      const paused = await snapshot(page);
      await page.waitForTimeout(500);
      assert.deepEqual(
        await snapshot(page),
        paused,
        "explicit pause freezes the simulation",
      );
      const queued = await point(page, { x: 3, y: 15 });
      await page.mouse.click(queued.x, queued.y, { button: "right" });
      assert.equal(
        (await snapshot(page)).party[0].orders,
        2,
        "pause appends a destination",
      );
      await page.keyboard.press("Escape");
      await page
        .getByRole("button", { name: "Pokracovat", exact: true })
        .click();
      assert.equal(
        (await snapshot(page)).paused,
        true,
        "menus preserve tactical pause",
      );
      await page.keyboard.press("Space");
      await page
        .getByRole("button", { name: "Vybrat tim", exact: true })
        .click();
      const beforeGroup = await snapshot(page),
        group = await point(page, { x: 5, y: 18 });
      await page.mouse.click(group.x, group.y, { button: "right" });
      await page.waitForTimeout(500);
      const afterGroup = await snapshot(page);
      assert(
        afterGroup.party.every(
          (p, i) =>
            Math.hypot(
              p.x - beforeGroup.party[i].x,
              p.y - beforeGroup.party[i].y,
            ) > 0.2,
        ),
        "both characters move simultaneously",
      );
      await page.keyboard.press("Escape");
      await page
        .getByRole("button", { name: "Pokracovat", exact: true })
        .click();
      assert.equal(
        (await snapshot(page)).paused,
        false,
        "running stays running after menu",
      );
    }
    const desktopColors = await pixels(page);
    await checkLayout(page);
    await page.screenshot({ path: path.join(output, "web-rtt-desktop.png") });
    for (let i = 0; i < 5; i++) await page.mouse.wheel(0, -80);
    await page.waitForTimeout(300);
    assert((await pixels(page)) > 100, "zoom retains a rendered scene");
    await page.getByRole("button", { name: "Menu (Esc)", exact: true }).click();
    await page.getByRole("button", { name: "Nastavenia", exact: true }).click();
    await page.getByLabel("Zorne polia nepriatelov").uncheck();
    assert.equal(
      await page.getByLabel("Zorne polia nepriatelov").isChecked(),
      false,
    );
    await page.screenshot({ path: path.join(output, "web-rtt-options.png") });
    results.push({
      viewport: "1920x1080",
      canvasColors: desktopColors,
      diagnostics,
      result: "PASS",
    });
    await desktop.close();
    const mobile = await browser.newContext({
      viewport: { width: 480, height: 800 },
      isMobile: true,
      hasTouch: true,
      deviceScaleFactor: 1,
    });
    const touch = await mobile.newPage();
    touch.on("pageerror", (e) => errors.push(e.message));
    await launch(touch);
    {
      const dest = await point(touch, { x: 3, y: 18 });
      await touch.touchscreen.tap(dest.x, dest.y);
      await touch.waitForTimeout(600);
      assert(
        (await snapshot(touch)).party[0].y > 14.2,
        "touch sends a continuous movement order",
      );
    }
    await touch
      .getByRole("button", { name: "Takticka pauza (Space)", exact: true })
      .tap();
    assert.equal(
      await touch.getByTestId("realtime-state").textContent(),
      "TAKTICKA PAUZA",
    );
    await checkLayout(touch);
    const mobileColors = await pixels(touch);
    await touch.screenshot({ path: path.join(output, "web-rtt-mobile.png") });
    results.push({
      viewport: "480x800 touch",
      canvasColors: mobileColors,
      result: "PASS",
    });
    await mobile.close();
    for (const size of [
      { width: 360, height: 740 },
      { width: 600, height: 480 },
      { width: 800, height: 480 },
    ]) {
      const context = await browser.newContext({
        viewport: size,
        hasTouch: true,
      });
      const small = await context.newPage();
      small.on("pageerror", (e) => errors.push(e.message));
      await launch(small);
      await checkLayout(small);
      await pixels(small);
      results.push({
        viewport: size.width + "x" + size.height,
        result: "PASS",
      });
      await context.close();
    }
    assert.deepEqual(errors, [], "browser runtime errors");
    fs.writeFileSync(
      path.join(output, "web-rtt-verification.json"),
      JSON.stringify({ url, results, errors }, null, 2),
    );
    console.log(JSON.stringify({ url, results, errors }, null, 2));
  } finally {
    await browser.close();
  }
})().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});

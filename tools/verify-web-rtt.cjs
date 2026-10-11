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
async function canvasHash(page) {
  return page.locator("canvas").evaluate((canvas) => {
    const { data } = canvas
      .getContext("2d")
      .getImageData(0, 0, canvas.width, canvas.height);
    let hash = 2166136261;
    for (let i = 0; i < data.length; i += 64)
      hash = Math.imul(hash ^ data[i], 16777619);
    return hash >>> 0;
  });
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
    .locator(".rt-hud button,.rt-objective,.rt-message,.rt-order-tray header")
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
async function checkOrderTray(page) {
  await page.keyboard.press("s");
  await page.keyboard.press("1");
  if (!(await snapshot(page)).paused) await page.keyboard.press("Space");
  for (let i = 0; i < 7; i++) {
    const p = await point(page, { x: 3, y: i % 2 ? 16 : 18 });
    await page.mouse.click(p.x, p.y, { button: "right" });
  }
  assert.equal((await snapshot(page)).party[0].orders, 7);
  assert.equal(await page.locator("[data-order-index]").count(), 5);
  await page
    .getByRole("button", { name: "Dalsie rozkazy", exact: true })
    .click();
  assert.equal(await page.locator("[data-order-index]").count(), 2);
  assert.equal(
    await page
      .locator("[data-order-index]")
      .first()
      .getAttribute("data-order-index"),
    "5",
  );
  const before = await snapshot(page);
  await page.locator('[data-order-index="5"]').click();
  assert.deepEqual(
    await snapshot(page),
    before,
    "inspecting an order does not execute or replace it",
  );
  await page
    .getByRole("button", {
      name: "Odstranit posledny cakajuci rozkaz",
      exact: true,
    })
    .click();
  await page
    .getByRole("button", {
      name: "Odstranit posledny cakajuci rozkaz",
      exact: true,
    })
    .click();
  assert.equal((await snapshot(page)).party[0].orders, 5);
  assert.equal(
    await page
      .locator("[data-order-index]")
      .first()
      .getAttribute("data-order-index"),
    "0",
  );
  assert.deepEqual(
    (await snapshot(page)).party.map((p) => [p.x, p.y]),
    before.party.map((p) => [p.x, p.y]),
    "tail undo never rewinds world movement",
  );
  await page.keyboard.press("2");
  assert.equal(
    await page.locator(".rt-order-tray").count(),
    0,
    "each member owns a separate plan",
  );
  await page.keyboard.press("1");
  await checkLayout(page);
  if (await page.evaluate(() => !!window.__kopanice)) {
    await page
      .getByRole("button", { name: "Hodit kamen", exact: true })
      .click();
    const marker = await page.evaluate(() =>
      window.__kopanice.markers().find((m) => m.id === "officer"),
    );
    assert(marker, "officer marker must be visible");
    assert.equal(marker.bounds.right - marker.bounds.left, 40);
    await page.mouse.click(marker.x, marker.y);
    assert.equal(
      await page
        .getByRole("button", { name: "Vybrat Dostojnik", exact: true })
        .getAttribute("aria-pressed"),
      "true",
    );
    assert.equal(
      await page
        .getByRole("button", { name: "Hodit kamen", exact: true })
        .getAttribute("aria-pressed"),
      "true",
      "selection preserves armed skills",
    );
    assert.equal(
      (await snapshot(page)).party[0].orders,
      5,
      "selection does not destroy another member's plan",
    );
    assert.equal(
      (await snapshot(page)).party[1].orders,
      0,
      "marker never issues a stray terrain command",
    );
    const color = await page.locator("canvas").evaluate((canvas, marker) => {
      const dpr = canvas.width / innerWidth;
      return [
        ...canvas
          .getContext("2d")
          .getImageData(
            Math.round((marker.x + 10) * dpr),
            Math.round(marker.y * dpr),
            1,
            1,
          ).data,
      ];
    }, marker);
    assert.deepEqual(
      color,
      [67, 94, 81, 255],
      "marker is painted above scenery",
    );
    await page.keyboard.press("Escape"); // Cancel armed rock, do not open the menu.
    await page.keyboard.press("1");
  }
  await page.keyboard.press("Space");
  await page.waitForTimeout(180);
  await page.keyboard.press("Space");
  await snapshot(page);
  assert.equal(
    await page
      .locator('[data-order-index="0"]')
      .getAttribute("data-order-state"),
    "Pozastavene",
  );
  await page.screenshot({ path: path.join(output, "web-rtt-orders.png") });
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
    page.setDefaultTimeout(10000);
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
    await checkOrderTray(page);
    console.log("Desktop movement, markers and order tray: PASS");
    const desktopColors = await pixels(page);
    await checkLayout(page);
    await page.screenshot({ path: path.join(output, "web-rtt-desktop.png") });
    await page.mouse.move(500, 350);
    await page.waitForTimeout(120);
    const beforeZoom = await canvasHash(page);
    for (let i = 0; i < 5; i++) await page.mouse.wheel(0, -80);
    await page.waitForTimeout(300);
    assert.notEqual(
      await canvasHash(page),
      beforeZoom,
      "wheel over terrain changes the camera rendering",
    );
    assert((await pixels(page)) > 100, "zoom retains a rendered scene");
    if (diagnostics) {
      await page.keyboard.press("q");
      const marker = await page.evaluate(() =>
        window.__kopanice.markers().find((m) => m.id === "officer"),
      );
      assert(marker, "marker follows zoom and rotated camera");
      assert.equal(marker.bounds.right - marker.bounds.left, 40);
      await page.mouse.click(marker.x, marker.y);
      assert.equal(
        await page
          .getByRole("button", { name: "Vybrat Dostojnik", exact: true })
          .getAttribute("aria-pressed"),
        "true",
      );
    }
    await page.getByRole("button", { name: "Menu (Esc)", exact: true }).click();
    await page.getByRole("button", { name: "Nastavenia", exact: true }).click();
    await page.getByLabel("Zorne polia nepriatelov").uncheck();
    await page.getByLabel("Oznacenia clenov timu").uncheck();
    assert.equal(
      await page.getByLabel("Zorne polia nepriatelov").isChecked(),
      false,
    );
    await page.screenshot({ path: path.join(output, "web-rtt-options.png") });
    await page.reload({ waitUntil: "domcontentloaded" });
    await page.getByRole("button", { name: "Nastavenia", exact: true }).click();
    assert.equal(
      await page.getByLabel("Oznacenia clenov timu").isChecked(),
      false,
      "marker preference survives reload",
    );
    results.push({
      viewport: "1920x1080",
      canvasColors: desktopColors,
      diagnostics,
      result: "PASS",
    });
    console.log("Desktop layout, zoom and persisted options: PASS");
    await desktop.close();
    console.log("Checking touch viewport...");
    const mobile = await browser.newContext({
      viewport: { width: 480, height: 800 },
      isMobile: true,
      hasTouch: true,
      deviceScaleFactor: 1,
    });
    const touch = await mobile.newPage();
    touch.setDefaultTimeout(10000);
    touch.on("pageerror", (e) => errors.push(e.message));
    await launch(touch);
    console.log("Touch mission deployed");
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
    for (let i = 0; i < 6; i++) {
      // Keep touch targets clear of the moving member's body-selection hitbox.
      const p = await point(
        touch,
        i % 2 ? { x: 1.2, y: 17.5 } : { x: 3, y: 18 },
      );
      await touch.touchscreen.tap(p.x, p.y);
      await touch.waitForTimeout(60);
    }
    assert(
      (await snapshot(touch)).party[0].orders > 5,
      "touch appends a paged plan",
    );
    console.log("Touch movement and queued plan: PASS");
    await touch
      .getByRole("button", { name: "Dalsie rozkazy", exact: true })
      .tap();
    assert((await snapshot(touch)).party[0].orders > 5);
    if (await touch.evaluate(() => !!window.__kopanice)) {
      const marker = await touch.evaluate(() =>
        window.__kopanice.markers().find((m) => m.id === "officer"),
      );
      assert(marker);
      await touch.touchscreen.tap(marker.x, marker.y);
      assert.equal(
        await touch
          .getByRole("button", { name: "Vybrat Dostojnik", exact: true })
          .getAttribute("aria-pressed"),
        "true",
      );
      await touch
        .getByRole("button", { name: "Vybrat Partizan", exact: true })
        .tap();
    }
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
      small.setDefaultTimeout(10000);
      small.on("pageerror", (e) => errors.push(e.message));
      await launch(small);
      await small
        .getByRole("button", { name: "Takticka pauza (Space)", exact: true })
        .click();
      for (const y of [18, 16]) {
        const p = await point(small, { x: 3, y });
        await small.mouse.click(p.x, p.y, { button: "right" });
      }
      assert.equal(
        (await snapshot(small)).party[0].orders,
        2,
        "small viewport accepts a full plan",
      );
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

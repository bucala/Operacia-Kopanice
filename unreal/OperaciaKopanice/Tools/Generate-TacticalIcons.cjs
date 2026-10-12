// Rasterize licensed Lucide icons into the native HUD's source directory.
const fs = require('node:fs');
const path = require('node:path');
const { createRequire } = require('node:module');
const runtime = process.argv[2];
if (!runtime) throw new Error('Pass the directory containing lucide and sharp.');
const requireRuntime = createRequire(path.join(runtime, '__resolve.cjs'));
const lucide = requireRuntime('lucide');
const sharp = requireRuntime('sharp');
const out = path.resolve(__dirname, '../ArtSource/Generated/TacticalHUD');
const icons = { layers: 'Layers', check: 'Check', x: 'X', heart_pulse: 'HeartPulse', save: 'Save', folder_open: 'FolderOpen', map: 'Map' };
const escape = value => String(value).replaceAll('&', '&amp;').replaceAll('"', '&quot;');
async function main() {
  fs.mkdirSync(out, { recursive: true });
  for (const [name, key] of Object.entries(icons)) {
    const nodes = lucide[key];
    if (!nodes) throw new Error('Missing Lucide icon: ' + key);
    const body = nodes.map(([tag, attrs]) => '<' + tag + ' ' + Object.entries(attrs).map(([k, v]) => k + '="' + escape(v) + '"').join(' ') + '/>').join('');
    const svg = '<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round">' + body + '</svg>';
    await sharp(Buffer.from(svg)).png().toFile(path.join(out, 'T_' + name + '.png'));
  }
  console.log('Generated ' + Object.keys(icons).length + ' Lucide HUD textures.');
}
main().catch(error => { console.error(error.message); process.exitCode = 1; });

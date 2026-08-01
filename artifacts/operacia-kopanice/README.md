# Operácia Kopanice

Turn-based stealth puzzle set in a snowy Slovak mountain village during the SNP period. Each mission keeps the GO-style rhythm: make one careful move, let the guards react, and reach the exit unseen.

## Running locally

From the workspace root:

```bash
pnpm --filter @workspace/operacia-kopanice run dev
```

The artifact-managed workflow supplies its required `PORT` and `BASE_PATH` values.

## Checks

```bash
pnpm --filter @workspace/operacia-kopanice run typecheck
pnpm --filter @workspace/operacia-kopanice run test
pnpm --filter @workspace/operacia-kopanice run build
```

## Village board

The active GO board uses declarative terrain and decorative objects. Snow, roads, planks, mud, houses, trees, rocks, crates, and fences are rendered separately from the deterministic movement and guard logic. Houses and trees are solid by default; individual decorations can opt into or out of movement and sight blocking with explicit collision flags.
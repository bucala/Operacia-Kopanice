import { useEffect, useRef } from 'react';
import { GoApp } from '@/go/GoApp';

/**
 * Root React component for Operácia Kopanice.
 *
 * React owns the DOM structure (#app → canvas#game + div#ui) that the vanilla
 * TypeScript game engine expects. Once the nodes are mounted, a single
 * useEffect initialises the GoApp and attaches the resize listener.
 */
export default function App() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const uiRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const ui = uiRef.current;
    if (!canvas || !ui) return;

    let app: GoApp | null = null;
    try {
      app = new GoApp(canvas, ui);
      app.resize(window.innerWidth, window.innerHeight);
      app.start();
    } catch (err) {
      ui.textContent = `Chyba pri štarte: ${(err as Error).message}`;
      console.error(err);
    }

    function onResize() {
      app?.resize(window.innerWidth, window.innerHeight);
    }
    window.addEventListener('resize', onResize);

    return () => {
      window.removeEventListener('resize', onResize);
      // GoApp does not expose a destroy() method; DOM cleanup is handled by
      // React unmounting the canvas and ui nodes.
    };
  }, []);

  return (
    <div id="app">
      <canvas id="game" ref={canvasRef} />
      <div id="ui" ref={uiRef} />
    </div>
  );
}

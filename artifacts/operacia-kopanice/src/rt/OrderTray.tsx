import { useEffect, useState } from "react";
import {
  ArrowRight,
  Footprints,
  Hand,
  Swords,
  CircleDot,
  Backpack,
  ChevronLeft,
  ChevronRight,
} from "lucide-react";
import { type Order, type Point } from "./types";
import { type Simulation } from "./Simulation";
import { orderName, orderPage, orderState } from "./OrderDisplay";

function OrderIcon({ order }: { order: Order }) {
  switch (order.kind) {
    case "move":
      return order.run ? <Footprints /> : <ArrowRight />;
    case "interact":
      return <Hand />;
    case "takedown":
      return <Swords />;
    case "distract":
      return <CircleDot />;
    case "carry":
      return <Backpack />;
  }
}
export function OrderTray({
  sim,
  focus,
}: {
  sim: Simulation;
  focus: (p: Point) => void;
}) {
  const unit = sim.party[sim.active];
  const [requested, setPage] = useState(0);
  const { page, pages, start } = orderPage(unit.orders.length, requested);
  useEffect(() => {
    setPage((n) => Math.min(n, pages - 1));
  }, [pages]);
  if (!unit.orders.length) return null;
  return (
    <section className="rt-order-tray" aria-label={unit.name + " rozkazy"}>
      <header>
        <span>Rozkazy · {unit.orders.length}</span>
        <span>
          {orderState(unit, 0, sim.paused)} · {page + 1}/{pages}
        </span>
      </header>
      <div className="rt-order-slots">
        <button
          type="button"
          aria-label="Predosle rozkazy"
          title="Predosle rozkazy"
          disabled={!page}
          onClick={() => setPage(page - 1)}
        >
          <ChevronLeft />
        </button>
        {Array.from({ length: 5 }, (_, slot) => {
          const index = start + slot,
            order = unit.orders[index];
          if (!order) return <span className="rt-order-empty" key={slot} />;
          const state = orderState(unit, index, sim.paused);
          const label = `${index + 1}. ${orderName(order, sim.mission)} / ${state}`;
          return (
            <button
              key={slot}
              type="button"
              className={index === 0 && unit.started ? "is-active" : ""}
              data-order-index={index}
              data-order-state={state}
              aria-label={label}
              title={label}
              onClick={() => {
                const target = sim.units.find((u) => u.id === order.target);
                focus(target ?? order.position);
                sim.message = label;
              }}
            >
              <OrderIcon order={order} />
              <small>{index + 1}</small>
            </button>
          );
        })}
        <button
          type="button"
          aria-label="Dalsie rozkazy"
          title="Dalsie rozkazy"
          disabled={page === pages - 1}
          onClick={() => setPage(page + 1)}
        >
          <ChevronRight />
        </button>
      </div>
    </section>
  );
}

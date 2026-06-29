import { Position } from '@/components/Position';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Events, type SoundEvent } from '@/core/events';
import { gridToScreen, type IsoConfig } from '@/core/math/iso';
import { playerEntity, Res } from '@/core/resources';
import type { AssetBundle, SoundDef } from '@/assets/types';
import type { TileMap } from '@/map/TileMap';

/**
 * Positional 2D audio. Each in-world sound is attenuated by distance to the
 * listener (the player), panned by its on-screen horizontal offset, and damped
 * by occlusion — walls and tall props between the source and the listener
 * muffle the sound, summing per-blocker and per-height penalties.
 *
 * All sounds are synthesised from the audio bank (oscillator/noise), so no audio
 * files are shipped. The AudioContext starts suspended and is resumed on the
 * first user gesture (browsers require this).
 */
export class AudioSystem implements System {
  readonly name = 'audio';
  private audio: AudioContext | null = null;
  private master: GainNode | null = null;
  private bank!: AssetBundle['audio'];

  init(world: World): void {
    this.bank = world.resource<AssetBundle>(Res.Assets).audio;
    world.events.on<SoundEvent>(Events.Sound, (s) => this.play(world, s));
  }

  /** No per-frame work; audio is event-driven. */
  update(_world: World, _ctx: FrameContext): void {}

  /** Resume the AudioContext — call from a user-gesture handler. */
  resume(): void {
    this.ensureContext();
    void this.audio?.resume();
  }

  setMuted(muted: boolean): void {
    if (this.master) this.master.gain.value = muted ? 0 : 1;
  }

  private ensureContext(): void {
    if (this.audio || typeof window === 'undefined') return;
    const Ctor =
      window.AudioContext ?? (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
    if (!Ctor) return;
    this.audio = new Ctor();
    this.master = this.audio.createGain();
    this.master.gain.value = 1;
    this.master.connect(this.audio.destination);
  }

  private play(world: World, snd: SoundEvent): void {
    const def = this.bank.sounds[snd.name];
    if (!def || snd.loudness <= 0) return;
    this.ensureContext();
    if (!this.audio || !this.master || this.audio.state !== 'running') return;

    const map = world.resource<TileMap>(Res.Map);
    const iso = world.resource<IsoConfig>(Res.Iso);
    const listener = world.get(playerEntity(world), Position);
    if (!listener) return;

    const dist = Math.hypot(snd.gx - listener.fx, snd.gy - listener.fy);
    const { maxDistance, minGain } = this.bank.occlusion;
    if (dist > maxDistance) return;

    const distanceGain = Math.max(0, 1 - dist / maxDistance);
    const occlusion = this.occlusionFactor(map, listener.fx, listener.fy, snd.gx, snd.gy);
    const gain = Math.max(minGain, def.gain * distanceGain * occlusion);
    if (gain < minGain) return;

    // Pan from the horizontal screen offset between source and listener.
    const a = gridToScreen(snd.gx, snd.gy, 0, iso);
    const b = gridToScreen(listener.fx, listener.fy, 0, iso);
    const pan = Math.max(-1, Math.min(1, (a.x - b.x) / (iso.tileWidth * 6)));

    this.synthesize(def, gain, pan);
  }

  /** Multiplier in (0,1]; lower means more muffled by intervening cover. */
  private occlusionFactor(
    map: TileMap,
    lx: number,
    ly: number,
    sx: number,
    sy: number,
  ): number {
    const { perBlocker, perHeight } = this.bank.occlusion;
    const dx = sx - lx;
    const dy = sy - ly;
    const dist = Math.hypot(dx, dy);
    const samples = Math.ceil(dist / 0.5);
    let prevX = Math.round(lx);
    let prevY = Math.round(ly);
    let blockers = 0;
    let heightSum = 0;
    for (let i = 1; i < samples; i++) {
      const t = i / samples;
      const cx = Math.round(lx + dx * t);
      const cy = Math.round(ly + dy * t);
      if (cx === prevX && cy === prevY) continue;
      prevX = cx;
      prevY = cy;
      if (map.blocksVision(cx, cy)) {
        blockers += 1;
        heightSum += map.heightAt(cx, cy);
      }
    }
    return Math.max(0, 1 - (blockers * perBlocker + heightSum * perHeight));
  }

  private synthesize(def: SoundDef, gain: number, pan: number): void {
    const ctx = this.audio!;
    const now = ctx.currentTime;
    const env = ctx.createGain();
    env.gain.setValueAtTime(0, now);
    env.gain.linearRampToValueAtTime(gain, now + 0.005);
    env.gain.exponentialRampToValueAtTime(0.0001, now + def.duration);

    const panner = ctx.createStereoPanner();
    panner.pan.value = pan;
    env.connect(panner);
    panner.connect(this.master!);

    if (def.wave === 'noise') {
      const buffer = ctx.createBuffer(1, ctx.sampleRate * def.duration, ctx.sampleRate);
      const data = buffer.getChannelData(0);
      for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
      const src = ctx.createBufferSource();
      src.buffer = buffer;
      const filter = ctx.createBiquadFilter();
      filter.type = 'bandpass';
      filter.frequency.value = def.freq;
      src.connect(filter);
      filter.connect(env);
      src.start(now);
      src.stop(now + def.duration);
    } else {
      const osc = ctx.createOscillator();
      osc.type = def.wave;
      osc.frequency.value = def.freq;
      osc.connect(env);
      osc.start(now);
      osc.stop(now + def.duration);
    }
  }
}

import type { Entity } from './ecs/Entity';
import type { AIState } from '@/components/AIComp';

/** Canonical event names used on the World's EventBus. */
export const Events = {
  Sound: 'sound',
  SkillUsed: 'skill-used',
  ActorKilled: 'actor-killed',
  AIStateChanged: 'ai-state-changed',
  Log: 'log',
  StateSync: 'state-sync',
  AssistantRequest: 'assistant-request',
} as const;

/** A noise emitted into the world; heard by audio and by enemy AI. */
export interface SoundEvent {
  gx: number;
  gy: number;
  /** Sound id from assets/audio.json. */
  name: string;
  /** Loudness in tiles (effective hearing radius). */
  loudness: number;
  /** Entity that produced the sound, if any. */
  source: Entity | null;
}

export interface SkillUsedEvent {
  caster: Entity;
  skillId: string;
  gx: number;
  gy: number;
}

export interface ActorKilledEvent {
  victim: Entity;
  killer: Entity | null;
}

export interface AIStateChangedEvent {
  entity: Entity;
  from: AIState;
  to: AIState;
}

export interface LogEvent {
  message: string;
  level: 'info' | 'warn' | 'alert';
}

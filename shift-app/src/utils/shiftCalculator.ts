import { ShiftConfig, ShiftType, DayInfo } from '../types';

export function getShiftForDate(date: Date, config: ShiftConfig): ShiftType {
  const start = new Date(config.cycleStartDate);
  start.setHours(0, 0, 0, 0);
  const target = new Date(date);
  target.setHours(0, 0, 0, 0);

  const diffMs = target.getTime() - start.getTime();
  const diffDays = Math.round(diffMs / (1000 * 60 * 60 * 24));

  if (diffDays < 0) return 'FREI';

  const idx = diffDays % config.cyclePattern.length;
  return config.cyclePattern[idx];
}

export function getWakeTime(date: Date, shift: ShiftType, config: ShiftConfig): Date | null {
  if (shift === 'FREI') return null;

  const times = config.shiftTimes[shift as 'F' | 'S' | 'N'];
  const shiftStart = new Date(date);
  shiftStart.setHours(times.hour, times.minute, 0, 0);

  const wakeTime = new Date(shiftStart.getTime() - config.alarmOffsetMinutes * 60 * 1000);
  return wakeTime;
}

export function getDayInfo(date: Date, config: ShiftConfig): DayInfo {
  const shift = getShiftForDate(date, config);
  const wakeTime = getWakeTime(date, shift, config);
  return { date, shift, wakeTime };
}

export function getNextDays(count: number, config: ShiftConfig): DayInfo[] {
  const result: DayInfo[] = [];
  const today = new Date();
  for (let i = 0; i < count; i++) {
    const d = new Date(today);
    d.setDate(today.getDate() + i);
    result.push(getDayInfo(d, config));
  }
  return result;
}

export function getNextWorkDay(config: ShiftConfig): DayInfo | null {
  for (let i = 0; i < 14; i++) {
    const d = new Date();
    d.setDate(d.getDate() + i);
    const info = getDayInfo(d, config);
    if (info.shift !== 'FREI') return info;
  }
  return null;
}

export const SHIFT_LABELS: Record<ShiftType, string> = {
  F: 'Frühschicht',
  S: 'Spätschicht',
  N: 'Nachtschicht',
  FREI: 'Frei',
};

export const SHIFT_COLORS: Record<ShiftType, string> = {
  F: '#f59e0b',
  S: '#3b82f6',
  N: '#8b5cf6',
  FREI: '#22c55e',
};

export const SHIFT_ICONS: Record<ShiftType, string> = {
  F: '🌅',
  S: '☀️',
  N: '🌙',
  FREI: '✅',
};

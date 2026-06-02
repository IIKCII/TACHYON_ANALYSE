export type ShiftType = 'F' | 'S' | 'N' | 'FREI';

export interface ShiftTimes {
  F: { hour: number; minute: number };
  S: { hour: number; minute: number };
  N: { hour: number; minute: number };
}

export interface ShiftConfig {
  /** ISO date string of the first day of the cycle */
  cycleStartDate: string;
  /** Pattern repeating indefinitely, e.g. ['F','F','S','S','N','N','FREI','FREI'] */
  cyclePattern: ShiftType[];
  /** Minutes before shift start to trigger the alarm */
  alarmOffsetMinutes: number;
  shiftTimes: ShiftTimes;
}

export interface Reminder {
  id: string;
  title: string;
  message: string;
  /** Trigger before which shift types */
  triggerOnShifts: ShiftType[];
  /** Minutes before shift start */
  minutesBefore: number;
  enabled: boolean;
  /** Require explicit confirmation (can't be dismissed) */
  requiresAck: boolean;
}

export interface DayInfo {
  date: Date;
  shift: ShiftType;
  wakeTime: Date | null;
}

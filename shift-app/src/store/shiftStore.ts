import AsyncStorage from '@react-native-async-storage/async-storage';
import { create } from 'zustand';
import { createJSONStorage, persist } from 'zustand/middleware';
import { ShiftConfig, Reminder, ShiftType } from '../types';

const DEFAULT_CONFIG: ShiftConfig = {
  cycleStartDate: new Date().toISOString().split('T')[0],
  // Standard 3-Schicht: 2x Früh, 2x Spät, 2x Nacht, 2x Frei
  cyclePattern: ['F', 'F', 'S', 'S', 'N', 'N', 'FREI', 'FREI'],
  alarmOffsetMinutes: 60,
  shiftTimes: {
    F: { hour: 6, minute: 0 },
    S: { hour: 14, minute: 0 },
    N: { hour: 22, minute: 0 },
  },
};

const DEFAULT_REMINDERS: Reminder[] = [
  {
    id: 'r1',
    title: '🥪 Brotdose einpacken!',
    message: 'Nicht vergessen — Brotdose für die Schicht vorbereiten.',
    triggerOnShifts: ['F', 'S', 'N'],
    minutesBefore: 90,
    enabled: true,
    requiresAck: true,
  },
  {
    id: 'r2',
    title: '🪪 Ausweis mitnehmen!',
    message: 'Betriebsausweis eingesteckt?',
    triggerOnShifts: ['F', 'S', 'N'],
    minutesBefore: 30,
    enabled: false,
    requiresAck: true,
  },
];

interface ShiftStore {
  config: ShiftConfig;
  reminders: Reminder[];
  notificationsGranted: boolean;
  setConfig: (config: Partial<ShiftConfig>) => void;
  setCycleStartDate: (date: string) => void;
  setCyclePattern: (pattern: ShiftType[]) => void;
  setAlarmOffset: (minutes: number) => void;
  setShiftTime: (shift: 'F' | 'S' | 'N', hour: number, minute: number) => void;
  addReminder: (reminder: Reminder) => void;
  updateReminder: (id: string, patch: Partial<Reminder>) => void;
  deleteReminder: (id: string) => void;
  toggleReminder: (id: string) => void;
  setNotificationsGranted: (granted: boolean) => void;
}

export const useShiftStore = create<ShiftStore>()(
  persist(
    (set) => ({
      config: DEFAULT_CONFIG,
      reminders: DEFAULT_REMINDERS,
      notificationsGranted: false,

      setConfig: (patch) =>
        set((s) => ({ config: { ...s.config, ...patch } })),

      setCycleStartDate: (date) =>
        set((s) => ({ config: { ...s.config, cycleStartDate: date } })),

      setCyclePattern: (pattern) =>
        set((s) => ({ config: { ...s.config, cyclePattern: pattern } })),

      setAlarmOffset: (minutes) =>
        set((s) => ({ config: { ...s.config, alarmOffsetMinutes: minutes } })),

      setShiftTime: (shift, hour, minute) =>
        set((s) => ({
          config: {
            ...s.config,
            shiftTimes: { ...s.config.shiftTimes, [shift]: { hour, minute } },
          },
        })),

      addReminder: (reminder) =>
        set((s) => ({ reminders: [...s.reminders, reminder] })),

      updateReminder: (id, patch) =>
        set((s) => ({
          reminders: s.reminders.map((r) => (r.id === id ? { ...r, ...patch } : r)),
        })),

      deleteReminder: (id) =>
        set((s) => ({ reminders: s.reminders.filter((r) => r.id !== id) })),

      toggleReminder: (id) =>
        set((s) => ({
          reminders: s.reminders.map((r) =>
            r.id === id ? { ...r, enabled: !r.enabled } : r
          ),
        })),

      setNotificationsGranted: (granted) =>
        set({ notificationsGranted: granted }),
    }),
    {
      name: 'schicht-store',
      storage: createJSONStorage(() => AsyncStorage),
    }
  )
);

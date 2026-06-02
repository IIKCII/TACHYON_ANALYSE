import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { useShiftStore } from '../store/shiftStore';
import { getNextDays, SHIFT_COLORS, SHIFT_ICONS } from '../utils/shiftCalculator';
import { ShiftType } from '../types';

const WEEKS = 4;

export function ScheduleScreen() {
  const { config } = useShiftStore();
  const days = getNextDays(WEEKS * 7, config);

  // Group by week
  const weeks: (typeof days)[] = [];
  for (let i = 0; i < WEEKS; i++) {
    weeks.push(days.slice(i * 7, i * 7 + 7));
  }

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.header}>Schichtplan — {WEEKS} Wochen</Text>

      {weeks.map((week, wi) => {
        const weekStart = week[0].date;
        const weekEnd = week[week.length - 1].date;
        return (
          <View key={wi} style={styles.week}>
            <Text style={styles.weekLabel}>
              KW {getWeekNumber(weekStart)} · {fmt(weekStart)} – {fmt(weekEnd)}
            </Text>
            <View style={styles.weekRow}>
              {week.map((day, di) => {
                const isToday = isSameDay(day.date, new Date());
                return (
                  <View key={di} style={[styles.dayCell, isToday && styles.dayCellToday]}>
                    <Text style={styles.dayCellWeekday}>
                      {day.date.toLocaleDateString('de-DE', { weekday: 'short' })}
                    </Text>
                    <Text style={styles.dayCellNum}>
                      {day.date.getDate()}
                    </Text>
                    <Text style={styles.dayCellIcon}>{SHIFT_ICONS[day.shift]}</Text>
                    <Text style={[styles.dayCellShift, { color: SHIFT_COLORS[day.shift] }]}>
                      {day.shift}
                    </Text>
                    {day.wakeTime ? (
                      <Text style={styles.dayCellTime}>
                        {day.wakeTime.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' })}
                      </Text>
                    ) : null}
                  </View>
                );
              })}
            </View>
          </View>
        );
      })}

      <View style={styles.legend}>
        {(['F', 'S', 'N', 'FREI'] as ShiftType[]).map((s) => (
          <View key={s} style={styles.legendItem}>
            <Text style={styles.legendIcon}>{SHIFT_ICONS[s]}</Text>
            <Text style={[styles.legendText, { color: SHIFT_COLORS[s] }]}>{s}</Text>
          </View>
        ))}
      </View>
    </ScrollView>
  );
}

function fmt(d: Date) {
  return d.toLocaleDateString('de-DE', { day: '2-digit', month: '2-digit' });
}

function isSameDay(a: Date, b: Date) {
  return a.toDateString() === b.toDateString();
}

function getWeekNumber(d: Date) {
  const date = new Date(Date.UTC(d.getFullYear(), d.getMonth(), d.getDate()));
  date.setUTCDate(date.getUTCDate() + 4 - (date.getUTCDay() || 7));
  const yearStart = new Date(Date.UTC(date.getUTCFullYear(), 0, 1));
  return Math.ceil((((date.getTime() - yearStart.getTime()) / 86400000) + 1) / 7);
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f172a' },
  content: { padding: 16, paddingBottom: 40 },
  header: { color: '#f1f5f9', fontSize: 20, fontWeight: '700', marginBottom: 20 },
  week: { marginBottom: 20 },
  weekLabel: { color: '#64748b', fontSize: 12, marginBottom: 8, fontWeight: '600' },
  weekRow: { flexDirection: 'row', gap: 4 },
  dayCell: {
    flex: 1,
    backgroundColor: '#1e293b',
    borderRadius: 10,
    padding: 6,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: 'transparent',
  },
  dayCellToday: {
    borderColor: '#3b82f6',
    backgroundColor: '#172554',
  },
  dayCellWeekday: { color: '#64748b', fontSize: 9, fontWeight: '600' },
  dayCellNum: { color: '#e2e8f0', fontSize: 13, fontWeight: '700', marginVertical: 2 },
  dayCellIcon: { fontSize: 14 },
  dayCellShift: { fontSize: 10, fontWeight: '800', marginTop: 2 },
  dayCellTime: { color: '#475569', fontSize: 8, marginTop: 2 },
  legend: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 20,
    marginTop: 8,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: '#1e293b',
  },
  legendItem: { flexDirection: 'row', alignItems: 'center', gap: 4 },
  legendIcon: { fontSize: 16 },
  legendText: { fontSize: 13, fontWeight: '700' },
});

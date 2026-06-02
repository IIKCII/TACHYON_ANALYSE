import React, { useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Alert,
} from 'react-native';
import { useShiftStore } from '../store/shiftStore';
import { getDayInfo, getNextDays, SHIFT_COLORS, SHIFT_ICONS, SHIFT_LABELS } from '../utils/shiftCalculator';
import { setSystemAlarm } from '../utils/alarmManager';
import { ShiftBadge } from '../components/ShiftBadge';

function fmt(date: Date): string {
  return date.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' });
}

function fmtDate(date: Date): string {
  return date.toLocaleDateString('de-DE', { weekday: 'long', day: 'numeric', month: 'long' });
}

export function HomeScreen() {
  const { config } = useShiftStore();
  const today = getDayInfo(new Date(), config);
  const upcoming = getNextDays(7, config).slice(1);

  const handleSetAlarm = useCallback(async () => {
    if (!today.wakeTime) {
      Alert.alert('Heute frei', 'Heute kein Wecker nötig!');
      return;
    }
    await setSystemAlarm(
      today.wakeTime.getHours(),
      today.wakeTime.getMinutes(),
      `${SHIFT_LABELS[today.shift]} — SchichtApp`
    );
  }, [today]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Today card */}
      <View style={[styles.todayCard, { borderColor: SHIFT_COLORS[today.shift] }]}>
        <Text style={styles.todayLabel}>Heute · {fmtDate(today.date)}</Text>
        <View style={styles.shiftRow}>
          <Text style={[styles.shiftIcon, { fontSize: 48 }]}>{SHIFT_ICONS[today.shift]}</Text>
          <View>
            <Text style={[styles.shiftName, { color: SHIFT_COLORS[today.shift] }]}>
              {SHIFT_LABELS[today.shift]}
            </Text>
            {today.wakeTime ? (
              <Text style={styles.wakeTime}>Wecker um {fmt(today.wakeTime)} Uhr</Text>
            ) : (
              <Text style={styles.freeText}>Kein Wecker heute</Text>
            )}
          </View>
        </View>

        {today.shift !== 'FREI' && (
          <TouchableOpacity style={styles.alarmBtn} onPress={handleSetAlarm}>
            <Text style={styles.alarmBtnText}>⏰  Wecker jetzt setzen</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Upcoming */}
      <Text style={styles.sectionTitle}>Nächste 6 Tage</Text>
      {upcoming.map((day, i) => (
        <View key={i} style={styles.dayRow}>
          <Text style={styles.dayLabel}>
            {day.date.toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' })}
          </Text>
          <ShiftBadge shift={day.shift} size="sm" />
          {day.wakeTime ? (
            <Text style={styles.dayTime}>{fmt(day.wakeTime)}</Text>
          ) : (
            <Text style={styles.dayFree}>—</Text>
          )}
        </View>
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f172a' },
  content: { padding: 20, paddingBottom: 40 },
  todayCard: {
    backgroundColor: '#1e293b',
    borderRadius: 16,
    padding: 20,
    borderWidth: 1.5,
    marginBottom: 28,
  },
  todayLabel: { color: '#94a3b8', fontSize: 13, marginBottom: 12 },
  shiftRow: { flexDirection: 'row', alignItems: 'center', gap: 16, marginBottom: 16 },
  shiftIcon: {},
  shiftName: { fontSize: 26, fontWeight: '800' },
  wakeTime: { color: '#e2e8f0', fontSize: 16, marginTop: 4 },
  freeText: { color: '#22c55e', fontSize: 16, marginTop: 4 },
  alarmBtn: {
    backgroundColor: '#1e40af',
    borderRadius: 12,
    paddingVertical: 14,
    alignItems: 'center',
  },
  alarmBtnText: { color: '#fff', fontSize: 16, fontWeight: '700' },
  sectionTitle: { color: '#94a3b8', fontSize: 13, fontWeight: '600', letterSpacing: 1, marginBottom: 12, textTransform: 'uppercase' },
  dayRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#1e293b',
    gap: 12,
  },
  dayLabel: { color: '#e2e8f0', fontSize: 14, width: 80 },
  dayTime: { color: '#60a5fa', fontSize: 14, marginLeft: 'auto' },
  dayFree: { color: '#475569', fontSize: 14, marginLeft: 'auto' },
});

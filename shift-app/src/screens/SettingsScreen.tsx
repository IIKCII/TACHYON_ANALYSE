import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Alert,
  Switch,
} from 'react-native';
import { useShiftStore } from '../store/shiftStore';
import { ShiftType } from '../types';
import { TimePickerModal } from '../components/TimePickerModal';
import { rescheduleAllAlarms, requestNotificationPermissions, setupNotificationChannels, registerAckCategory } from '../utils/alarmManager';
import { SHIFT_LABELS, SHIFT_COLORS } from '../utils/shiftCalculator';

const OFFSET_OPTIONS = [30, 45, 60, 75, 90];
const SHIFT_TYPES: Array<'F' | 'S' | 'N'> = ['F', 'S', 'N'];

// Standard 3-Schicht patterns
const PATTERNS: Array<{ label: string; pattern: ShiftType[] }> = [
  { label: '2-2-2-2 (Standard)', pattern: ['F', 'F', 'S', 'S', 'N', 'N', 'FREI', 'FREI'] },
  { label: '3-3-3 (Dreier)', pattern: ['F', 'F', 'F', 'S', 'S', 'S', 'N', 'N', 'N', 'FREI', 'FREI', 'FREI'] },
  { label: '4-4-4', pattern: ['F', 'F', 'F', 'F', 'S', 'S', 'S', 'S', 'N', 'N', 'N', 'N', 'FREI', 'FREI', 'FREI', 'FREI'] },
];

export function SettingsScreen() {
  const { config, setShiftTime, setAlarmOffset, setCycleStartDate, setCyclePattern, notificationsGranted, setNotificationsGranted } = useShiftStore();
  const [timePicker, setTimePicker] = useState<{ shift: 'F' | 'S' | 'N' } | null>(null);
  const [rescheduling, setRescheduling] = useState(false);

  const handleRequestPermissions = async () => {
    await setupNotificationChannels();
    await registerAckCategory();
    const granted = await requestNotificationPermissions();
    setNotificationsGranted(granted);
    Alert.alert(
      granted ? '✅ Berechtigung erteilt' : '❌ Keine Berechtigung',
      granted
        ? 'Notifications sind aktiviert. Wecker werden automatisch geplant.'
        : 'Bitte in den Android-Einstellungen Benachrichtigungen erlauben.'
    );
  };

  const handleReschedule = async () => {
    setRescheduling(true);
    try {
      await rescheduleAllAlarms(config);
      Alert.alert('✅ Fertig', 'Alle Wecker für die nächsten 14 Tage wurden neu geplant.');
    } finally {
      setRescheduling(false);
    }
  };

  const handlePatternSelect = (pattern: ShiftType[]) => {
    Alert.alert('Muster übernehmen?', 'Das ändert die Zyklusberechnung ab sofort.', [
      { text: 'Abbrechen', style: 'cancel' },
      { text: 'Ja', onPress: () => setCyclePattern(pattern) },
    ]);
  };

  const handleCycleStartToday = () => {
    Alert.alert('Zyklus heute starten?', 'Heute wird als erster Tag des Zyklus gesetzt (= Frühschicht Tag 1).', [
      { text: 'Abbrechen', style: 'cancel' },
      { text: 'Ja', onPress: () => setCycleStartDate(new Date().toISOString().split('T')[0]) },
    ]);
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>

      {/* Notifications */}
      <Text style={styles.section}>Benachrichtigungen</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Status</Text>
          <Text style={[styles.rowValue, { color: notificationsGranted ? '#22c55e' : '#ef4444' }]}>
            {notificationsGranted ? '✅ Aktiv' : '❌ Inaktiv'}
          </Text>
        </View>
        <TouchableOpacity style={styles.btn} onPress={handleRequestPermissions}>
          <Text style={styles.btnText}>Berechtigung anfragen</Text>
        </TouchableOpacity>
      </View>

      {/* Shift Times */}
      <Text style={styles.section}>Schichtzeiten</Text>
      <View style={styles.card}>
        {SHIFT_TYPES.map((shift) => {
          const t = config.shiftTimes[shift];
          return (
            <TouchableOpacity
              key={shift}
              style={styles.row}
              onPress={() => setTimePicker({ shift })}
            >
              <Text style={[styles.rowLabel, { color: SHIFT_COLORS[shift] }]}>
                {SHIFT_LABELS[shift]}
              </Text>
              <Text style={styles.rowValue}>
                {t.hour.toString().padStart(2, '0')}:{t.minute.toString().padStart(2, '0')} Uhr ›
              </Text>
            </TouchableOpacity>
          );
        })}
      </View>

      {/* Alarm Offset */}
      <Text style={styles.section}>Wecker-Vorlauf</Text>
      <View style={styles.card}>
        <Text style={styles.hint}>Wecker klingelt X Minuten vor Schichtbeginn</Text>
        <View style={styles.optionRow}>
          {OFFSET_OPTIONS.map((m) => (
            <TouchableOpacity
              key={m}
              style={[styles.optionBtn, config.alarmOffsetMinutes === m && styles.optionBtnSelected]}
              onPress={() => setAlarmOffset(m)}
            >
              <Text style={[styles.optionText, config.alarmOffsetMinutes === m && styles.optionTextSelected]}>
                {m}'
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Cycle Pattern */}
      <Text style={styles.section}>Schichtmuster</Text>
      <View style={styles.card}>
        <Text style={styles.hint}>Aktuell: [{config.cyclePattern.join(', ')}]</Text>
        {PATTERNS.map((p) => (
          <TouchableOpacity key={p.label} style={styles.patternBtn} onPress={() => handlePatternSelect(p.pattern)}>
            <Text style={styles.patternLabel}>{p.label}</Text>
            <Text style={styles.patternSub}>[{p.pattern.join(', ')}]</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Cycle Start */}
      <Text style={styles.section}>Zyklusstart</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Erster Tag</Text>
          <Text style={styles.rowValue}>{config.cycleStartDate}</Text>
        </View>
        <TouchableOpacity style={styles.btn} onPress={handleCycleStartToday}>
          <Text style={styles.btnText}>Heute als Start setzen</Text>
        </TouchableOpacity>
      </View>

      {/* Reschedule */}
      <Text style={styles.section}>Aktionen</Text>
      <View style={styles.card}>
        <TouchableOpacity
          style={[styles.btn, styles.btnPrimary]}
          onPress={handleReschedule}
          disabled={rescheduling}
        >
          <Text style={styles.btnPrimaryText}>
            {rescheduling ? '⏳ Plane...' : '🔄  Alle Wecker neu planen (14 Tage)'}
          </Text>
        </TouchableOpacity>
      </View>

      {timePicker && (
        <TimePickerModal
          visible
          title={`${SHIFT_LABELS[timePicker.shift]} Startzeit`}
          initialHour={config.shiftTimes[timePicker.shift].hour}
          initialMinute={config.shiftTimes[timePicker.shift].minute}
          onConfirm={(h, m) => {
            setShiftTime(timePicker.shift, h, m);
            setTimePicker(null);
          }}
          onCancel={() => setTimePicker(null)}
        />
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f172a' },
  content: { padding: 16, paddingBottom: 40 },
  section: {
    color: '#64748b',
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
    marginTop: 24,
    marginBottom: 8,
  },
  card: {
    backgroundColor: '#1e293b',
    borderRadius: 12,
    padding: 16,
    gap: 12,
    borderWidth: 1,
    borderColor: '#334155',
  },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 4 },
  rowLabel: { color: '#e2e8f0', fontSize: 15 },
  rowValue: { color: '#60a5fa', fontSize: 15, fontWeight: '600' },
  hint: { color: '#64748b', fontSize: 12 },
  optionRow: { flexDirection: 'row', gap: 8 },
  optionBtn: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 8,
    backgroundColor: '#0f172a',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#334155',
  },
  optionBtnSelected: { backgroundColor: '#1e40af', borderColor: '#3b82f6' },
  optionText: { color: '#94a3b8', fontWeight: '600' },
  optionTextSelected: { color: '#fff' },
  patternBtn: {
    backgroundColor: '#0f172a',
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: '#334155',
  },
  patternLabel: { color: '#e2e8f0', fontWeight: '600', fontSize: 14 },
  patternSub: { color: '#64748b', fontSize: 11, marginTop: 4 },
  btn: {
    backgroundColor: '#0f172a',
    borderRadius: 8,
    paddingVertical: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#334155',
  },
  btnText: { color: '#94a3b8', fontWeight: '600' },
  btnPrimary: { backgroundColor: '#1e40af', borderColor: '#3b82f6' },
  btnPrimaryText: { color: '#fff', fontWeight: '700' },
});

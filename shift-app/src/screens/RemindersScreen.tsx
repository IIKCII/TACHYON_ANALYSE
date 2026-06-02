import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  TextInput,
  Alert,
  Modal,
} from 'react-native';
import { useShiftStore } from '../store/shiftStore';
import { Reminder, ShiftType } from '../types';
import { ShiftBadge } from '../components/ShiftBadge';
import { scheduleReminder } from '../utils/alarmManager';

const SHIFT_TYPES: ShiftType[] = ['F', 'S', 'N'];

function generateId() {
  return Math.random().toString(36).slice(2, 10);
}

interface ReminderFormState {
  title: string;
  message: string;
  triggerOnShifts: ShiftType[];
  minutesBefore: number;
  requiresAck: boolean;
}

const DEFAULT_FORM: ReminderFormState = {
  title: '',
  message: '',
  triggerOnShifts: ['F', 'S', 'N'],
  minutesBefore: 60,
  requiresAck: true,
};

export function RemindersScreen() {
  const { reminders, addReminder, updateReminder, deleteReminder, toggleReminder, config } = useShiftStore();
  const [modalVisible, setModalVisible] = useState(false);
  const [form, setForm] = useState<ReminderFormState>(DEFAULT_FORM);

  const handleAdd = async () => {
    if (!form.title.trim()) {
      Alert.alert('Fehler', 'Bitte einen Titel eingeben.');
      return;
    }
    const newReminder: Reminder = {
      id: generateId(),
      title: form.title.trim(),
      message: form.message.trim(),
      triggerOnShifts: form.triggerOnShifts,
      minutesBefore: form.minutesBefore,
      enabled: true,
      requiresAck: form.requiresAck,
    };
    addReminder(newReminder);
    await scheduleReminder(newReminder, config);
    setForm(DEFAULT_FORM);
    setModalVisible(false);
  };

  const handleDelete = (id: string, title: string) => {
    Alert.alert('Löschen?', `"${title}" wirklich löschen?`, [
      { text: 'Abbrechen', style: 'cancel' },
      { text: 'Löschen', style: 'destructive', onPress: () => deleteReminder(id) },
    ]);
  };

  const toggleShift = (shift: ShiftType) => {
    setForm((f) => ({
      ...f,
      triggerOnShifts: f.triggerOnShifts.includes(shift)
        ? f.triggerOnShifts.filter((s) => s !== shift)
        : [...f.triggerOnShifts, shift],
    }));
  };

  return (
    <View style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.hint}>
          Hartnäckige Erinnerungen müssen aktiv bestätigt werden — einfaches Wegwischen reicht nicht.
        </Text>

        {reminders.length === 0 && (
          <Text style={styles.empty}>Noch keine Erinnerungen. Tippe auf + um eine anzulegen.</Text>
        )}

        {reminders.map((r) => (
          <View key={r.id} style={[styles.card, !r.enabled && styles.cardDisabled]}>
            <View style={styles.cardHeader}>
              <View style={styles.cardTitle}>
                <Text style={styles.cardTitleText}>{r.title}</Text>
                {r.requiresAck && <Text style={styles.ackBadge}>🔒 Bestätigung</Text>}
              </View>
              <Switch
                value={r.enabled}
                onValueChange={() => toggleReminder(r.id)}
                trackColor={{ false: '#334155', true: '#1e40af' }}
                thumbColor={r.enabled ? '#60a5fa' : '#64748b'}
              />
            </View>
            {r.message ? <Text style={styles.cardMsg}>{r.message}</Text> : null}
            <View style={styles.cardMeta}>
              <Text style={styles.cardMetaText}>⏱ {r.minutesBefore} Min vorher</Text>
              <View style={styles.shiftPills}>
                {r.triggerOnShifts.map((s) => (
                  <ShiftBadge key={s} shift={s} size="sm" />
                ))}
              </View>
            </View>
            <TouchableOpacity style={styles.deleteBtn} onPress={() => handleDelete(r.id, r.title)}>
              <Text style={styles.deleteBtnText}>Löschen</Text>
            </TouchableOpacity>
          </View>
        ))}
      </ScrollView>

      <TouchableOpacity style={styles.fab} onPress={() => setModalVisible(true)}>
        <Text style={styles.fabText}>＋</Text>
      </TouchableOpacity>

      {/* Add Reminder Modal */}
      <Modal visible={modalVisible} transparent animationType="slide">
        <View style={styles.modalOverlay}>
          <View style={styles.modalContainer}>
            <Text style={styles.modalTitle}>Neue Erinnerung</Text>

            <Text style={styles.fieldLabel}>Titel</Text>
            <TextInput
              style={styles.input}
              placeholder="z.B. Brotdose einpacken"
              placeholderTextColor="#475569"
              value={form.title}
              onChangeText={(v) => setForm((f) => ({ ...f, title: v }))}
            />

            <Text style={styles.fieldLabel}>Nachricht (optional)</Text>
            <TextInput
              style={styles.input}
              placeholder="Weitere Details..."
              placeholderTextColor="#475569"
              value={form.message}
              onChangeText={(v) => setForm((f) => ({ ...f, message: v }))}
            />

            <Text style={styles.fieldLabel}>Bei welcher Schicht?</Text>
            <View style={styles.shiftSelector}>
              {SHIFT_TYPES.map((s) => (
                <TouchableOpacity
                  key={s}
                  style={[styles.shiftOption, form.triggerOnShifts.includes(s) && styles.shiftOptionSelected]}
                  onPress={() => toggleShift(s)}
                >
                  <ShiftBadge shift={s} size="sm" />
                </TouchableOpacity>
              ))}
            </View>

            <Text style={styles.fieldLabel}>Wann vorher? (Minuten)</Text>
            <View style={styles.minuteRow}>
              {[15, 30, 60, 90, 120].map((m) => (
                <TouchableOpacity
                  key={m}
                  style={[styles.minuteBtn, form.minutesBefore === m && styles.minuteBtnSelected]}
                  onPress={() => setForm((f) => ({ ...f, minutesBefore: m }))}
                >
                  <Text style={[styles.minuteBtnText, form.minutesBefore === m && styles.minuteBtnTextSelected]}>
                    {m}'
                  </Text>
                </TouchableOpacity>
              ))}
            </View>

            <View style={styles.ackRow}>
              <Text style={styles.fieldLabel}>Bestätigung erforderlich</Text>
              <Switch
                value={form.requiresAck}
                onValueChange={(v) => setForm((f) => ({ ...f, requiresAck: v }))}
                trackColor={{ false: '#334155', true: '#1e40af' }}
                thumbColor={form.requiresAck ? '#60a5fa' : '#64748b'}
              />
            </View>
            <Text style={styles.ackHint}>
              {form.requiresAck
                ? 'Notification bleibt bis du "Erledigt" tippst.'
                : 'Kann normal weggewischt werden.'}
            </Text>

            <View style={styles.modalButtons}>
              <TouchableOpacity style={styles.cancelBtn} onPress={() => setModalVisible(false)}>
                <Text style={styles.cancelText}>Abbrechen</Text>
              </TouchableOpacity>
              <TouchableOpacity style={styles.saveBtn} onPress={handleAdd}>
                <Text style={styles.saveText}>Speichern</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f172a' },
  content: { padding: 16, paddingBottom: 100 },
  hint: { color: '#64748b', fontSize: 13, marginBottom: 16, lineHeight: 18 },
  empty: { color: '#475569', textAlign: 'center', marginTop: 40, fontSize: 15 },
  card: {
    backgroundColor: '#1e293b',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#334155',
  },
  cardDisabled: { opacity: 0.5 },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: 6 },
  cardTitle: { flex: 1, gap: 4 },
  cardTitleText: { color: '#f1f5f9', fontSize: 15, fontWeight: '700' },
  ackBadge: { color: '#f59e0b', fontSize: 11 },
  cardMsg: { color: '#94a3b8', fontSize: 13, marginBottom: 8 },
  cardMeta: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' },
  cardMetaText: { color: '#64748b', fontSize: 12 },
  shiftPills: { flexDirection: 'row', gap: 4 },
  deleteBtn: { marginTop: 10, alignSelf: 'flex-end' },
  deleteBtnText: { color: '#ef4444', fontSize: 12 },
  fab: {
    position: 'absolute',
    right: 24,
    bottom: 24,
    backgroundColor: '#1e40af',
    width: 56,
    height: 56,
    borderRadius: 28,
    justifyContent: 'center',
    alignItems: 'center',
    elevation: 6,
  },
  fabText: { color: '#fff', fontSize: 28, fontWeight: '300' },
  modalOverlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.8)', justifyContent: 'flex-end' },
  modalContainer: {
    backgroundColor: '#1e293b',
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    padding: 24,
    paddingBottom: 40,
  },
  modalTitle: { color: '#f1f5f9', fontSize: 18, fontWeight: '700', marginBottom: 20 },
  fieldLabel: { color: '#94a3b8', fontSize: 12, fontWeight: '600', marginBottom: 6, marginTop: 12 },
  input: {
    backgroundColor: '#0f172a',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 10,
    color: '#f1f5f9',
    fontSize: 15,
    borderWidth: 1,
    borderColor: '#334155',
  },
  shiftSelector: { flexDirection: 'row', gap: 8 },
  shiftOption: { opacity: 0.4, borderRadius: 8, padding: 2 },
  shiftOptionSelected: { opacity: 1 },
  minuteRow: { flexDirection: 'row', gap: 8 },
  minuteBtn: {
    flex: 1,
    paddingVertical: 8,
    borderRadius: 8,
    backgroundColor: '#0f172a',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#334155',
  },
  minuteBtnSelected: { backgroundColor: '#1e40af', borderColor: '#3b82f6' },
  minuteBtnText: { color: '#94a3b8', fontWeight: '600', fontSize: 13 },
  minuteBtnTextSelected: { color: '#fff' },
  ackRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginTop: 12 },
  ackHint: { color: '#475569', fontSize: 11, marginTop: 4 },
  modalButtons: { flexDirection: 'row', gap: 12, marginTop: 24 },
  cancelBtn: {
    flex: 1,
    paddingVertical: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#475569',
    alignItems: 'center',
  },
  cancelText: { color: '#94a3b8', fontWeight: '600' },
  saveBtn: { flex: 1, paddingVertical: 12, borderRadius: 8, backgroundColor: '#1e40af', alignItems: 'center' },
  saveText: { color: '#fff', fontWeight: '700' },
});

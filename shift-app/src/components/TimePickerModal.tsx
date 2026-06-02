import React, { useState } from 'react';
import {
  Modal,
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
} from 'react-native';

interface Props {
  visible: boolean;
  title: string;
  initialHour: number;
  initialMinute: number;
  onConfirm: (hour: number, minute: number) => void;
  onCancel: () => void;
}

export function TimePickerModal({ visible, title, initialHour, initialMinute, onConfirm, onCancel }: Props) {
  const [hour, setHour] = useState(initialHour);
  const [minute, setMinute] = useState(initialMinute);

  const hours = Array.from({ length: 24 }, (_, i) => i);
  const minutes = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55];

  return (
    <Modal visible={visible} transparent animationType="fade">
      <View style={styles.overlay}>
        <View style={styles.container}>
          <Text style={styles.title}>{title}</Text>

          <View style={styles.pickers}>
            <View style={styles.pickerCol}>
              <Text style={styles.pickerLabel}>Stunde</Text>
              <ScrollView style={styles.scroll} showsVerticalScrollIndicator={false}>
                {hours.map((h) => (
                  <TouchableOpacity
                    key={h}
                    style={[styles.item, h === hour && styles.itemSelected]}
                    onPress={() => setHour(h)}
                  >
                    <Text style={[styles.itemText, h === hour && styles.itemTextSelected]}>
                      {h.toString().padStart(2, '0')}
                    </Text>
                  </TouchableOpacity>
                ))}
              </ScrollView>
            </View>

            <Text style={styles.colon}>:</Text>

            <View style={styles.pickerCol}>
              <Text style={styles.pickerLabel}>Minute</Text>
              <ScrollView style={styles.scroll} showsVerticalScrollIndicator={false}>
                {minutes.map((m) => (
                  <TouchableOpacity
                    key={m}
                    style={[styles.item, m === minute && styles.itemSelected]}
                    onPress={() => setMinute(m)}
                  >
                    <Text style={[styles.itemText, m === minute && styles.itemTextSelected]}>
                      {m.toString().padStart(2, '0')}
                    </Text>
                  </TouchableOpacity>
                ))}
              </ScrollView>
            </View>
          </View>

          <View style={styles.timeDisplay}>
            <Text style={styles.timeText}>
              {hour.toString().padStart(2, '0')}:{minute.toString().padStart(2, '0')} Uhr
            </Text>
          </View>

          <View style={styles.buttons}>
            <TouchableOpacity style={styles.cancelBtn} onPress={onCancel}>
              <Text style={styles.cancelText}>Abbrechen</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.confirmBtn} onPress={() => onConfirm(hour, minute)}>
              <Text style={styles.confirmText}>Speichern</Text>
            </TouchableOpacity>
          </View>
        </View>
      </View>
    </Modal>
  );
}

const styles = StyleSheet.create({
  overlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  container: {
    backgroundColor: '#1e293b',
    borderRadius: 16,
    padding: 24,
    width: '85%',
    borderWidth: 1,
    borderColor: '#334155',
  },
  title: {
    color: '#f1f5f9',
    fontSize: 18,
    fontWeight: '700',
    textAlign: 'center',
    marginBottom: 20,
  },
  pickers: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 8,
  },
  pickerCol: {
    alignItems: 'center',
    flex: 1,
  },
  pickerLabel: {
    color: '#94a3b8',
    fontSize: 12,
    marginBottom: 8,
  },
  scroll: {
    height: 160,
  },
  item: {
    paddingVertical: 10,
    paddingHorizontal: 16,
    borderRadius: 8,
    marginVertical: 2,
    alignItems: 'center',
  },
  itemSelected: {
    backgroundColor: '#1e40af',
  },
  itemText: {
    color: '#94a3b8',
    fontSize: 16,
    fontWeight: '500',
  },
  itemTextSelected: {
    color: '#fff',
    fontWeight: '700',
  },
  colon: {
    color: '#f1f5f9',
    fontSize: 28,
    fontWeight: '700',
    marginTop: 20,
  },
  timeDisplay: {
    alignItems: 'center',
    marginTop: 16,
    paddingVertical: 8,
    borderRadius: 8,
    backgroundColor: '#0f172a',
  },
  timeText: {
    color: '#60a5fa',
    fontSize: 24,
    fontWeight: '700',
  },
  buttons: {
    flexDirection: 'row',
    gap: 12,
    marginTop: 20,
  },
  cancelBtn: {
    flex: 1,
    paddingVertical: 12,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#475569',
    alignItems: 'center',
  },
  cancelText: {
    color: '#94a3b8',
    fontWeight: '600',
  },
  confirmBtn: {
    flex: 1,
    paddingVertical: 12,
    borderRadius: 8,
    backgroundColor: '#1e40af',
    alignItems: 'center',
  },
  confirmText: {
    color: '#fff',
    fontWeight: '700',
  },
});

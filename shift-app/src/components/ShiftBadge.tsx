import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { ShiftType } from '../types';
import { SHIFT_COLORS, SHIFT_ICONS, SHIFT_LABELS } from '../utils/shiftCalculator';

interface Props {
  shift: ShiftType;
  size?: 'sm' | 'md' | 'lg';
}

export function ShiftBadge({ shift, size = 'md' }: Props) {
  const color = SHIFT_COLORS[shift];
  const icon = SHIFT_ICONS[shift];

  const fontSize = size === 'lg' ? 18 : size === 'md' ? 14 : 12;
  const iconSize = size === 'lg' ? 24 : size === 'md' ? 18 : 14;
  const paddingH = size === 'lg' ? 14 : size === 'md' ? 10 : 6;
  const paddingV = size === 'lg' ? 8 : size === 'md' ? 5 : 3;

  return (
    <View style={[styles.badge, { backgroundColor: color + '22', borderColor: color, paddingHorizontal: paddingH, paddingVertical: paddingV }]}>
      <Text style={{ fontSize: iconSize }}>{icon}</Text>
      <Text style={[styles.label, { color, fontSize }]}>
        {size === 'sm' ? shift : SHIFT_LABELS[shift]}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    borderRadius: 8,
    borderWidth: 1,
  },
  label: {
    fontWeight: '700',
    letterSpacing: 0.5,
  },
});

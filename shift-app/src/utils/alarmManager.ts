import * as Notifications from 'expo-notifications';
import * as Linking from 'expo-linking';
import { Platform } from 'react-native';
import { DayInfo, Reminder, ShiftConfig } from '../types';
import { getDayInfo, getNextDays } from './shiftCalculator';

/** Opens Android's native alarm clock app with a pre-filled alarm time. */
export async function setSystemAlarm(hour: number, minute: number, label: string): Promise<void> {
  if (Platform.OS !== 'android') return;
  const url = `intent:#Intent;action=android.intent.action.SET_ALARM;S.android.intent.extra.alarm.MESSAGE=${encodeURIComponent(label)};i.android.intent.extra.alarm.HOUR=${hour};i.android.intent.extra.alarm.MINUTES=${minute};Z.android.intent.extra.alarm.SKIP_UI=false;end`;
  const canOpen = await Linking.canOpenURL(url);
  if (canOpen) {
    await Linking.openURL(url);
  }
}

export async function requestNotificationPermissions(): Promise<boolean> {
  const { status } = await Notifications.requestPermissionsAsync({
    android: { allowAlert: true, allowBadge: true, allowSound: true },
  });
  return status === 'granted';
}

export async function setupNotificationChannels(): Promise<void> {
  if (Platform.OS !== 'android') return;

  await Notifications.setNotificationChannelAsync('alarm', {
    name: 'Wecker',
    importance: Notifications.AndroidImportance.MAX,
    sound: 'default',
    vibrationPattern: [0, 500, 200, 500],
    lockscreenVisibility: Notifications.AndroidNotificationVisibility.PUBLIC,
    bypassDnd: true,
  });

  await Notifications.setNotificationChannelAsync('reminder', {
    name: 'Erinnerungen',
    importance: Notifications.AndroidImportance.HIGH,
    sound: 'default',
    vibrationPattern: [0, 300, 100, 300, 100, 300],
    lockscreenVisibility: Notifications.AndroidNotificationVisibility.PUBLIC,
    bypassDnd: false,
  });
}

/** Cancels all previously scheduled shift notifications and reschedules the next N days. */
export async function rescheduleAllAlarms(config: ShiftConfig): Promise<void> {
  await Notifications.cancelAllScheduledNotificationsAsync();

  const days = getNextDays(14, config);

  for (const day of days) {
    if (!day.wakeTime) continue;

    // Don't schedule alarms in the past
    if (day.wakeTime.getTime() <= Date.now()) continue;

    const dayLabel = day.date.toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' });

    await Notifications.scheduleNotificationAsync({
      identifier: `alarm-${day.date.toISOString().split('T')[0]}`,
      content: {
        title: `⏰ Wecker — ${day.shift}schicht`,
        body: `${dayLabel} · Schicht beginnt um ${config.shiftTimes[day.shift as 'F' | 'S' | 'N'].hour.toString().padStart(2, '0')}:${config.shiftTimes[day.shift as 'F' | 'S' | 'N'].minute.toString().padStart(2, '0')} Uhr`,
        sound: 'default',
        priority: 'max',
        sticky: true,
        android: {
          channelId: 'alarm',
          priority: 'max',
          sticky: true,
          vibrationPattern: [0, 500, 200, 500],
        },
      },
      trigger: {
        type: Notifications.SchedulableTriggerInputTypes.DATE,
        date: day.wakeTime,
      },
    });
  }
}

export async function scheduleReminder(reminder: Reminder, config: ShiftConfig): Promise<void> {
  const days = getNextDays(7, config);

  for (const day of days) {
    if (!reminder.triggerOnShifts.includes(day.shift)) continue;
    if (!day.wakeTime) continue;

    const triggerTime = new Date(day.wakeTime.getTime() - reminder.minutesBefore * 60 * 1000);
    if (triggerTime.getTime() <= Date.now()) continue;

    await Notifications.scheduleNotificationAsync({
      identifier: `reminder-${reminder.id}-${day.date.toISOString().split('T')[0]}`,
      content: {
        title: reminder.title,
        body: reminder.message,
        sound: 'default',
        sticky: reminder.requiresAck,
        categoryIdentifier: reminder.requiresAck ? 'ack-required' : undefined,
        android: {
          channelId: 'reminder',
          sticky: reminder.requiresAck,
          ongoing: reminder.requiresAck,
          actions: reminder.requiresAck
            ? [{ title: '✅ Erledigt', identifier: 'dismiss', options: { isDestructive: false } }]
            : undefined,
        },
      },
      trigger: {
        type: Notifications.SchedulableTriggerInputTypes.DATE,
        date: triggerTime,
      },
    });
  }
}

export async function registerAckCategory(): Promise<void> {
  await Notifications.setNotificationCategoryAsync('ack-required', [
    {
      identifier: 'dismiss',
      buttonTitle: '✅ Erledigt',
      options: { opensAppToForeground: false },
    },
    {
      identifier: 'snooze',
      buttonTitle: '⏰ +10 Min',
      options: { opensAppToForeground: false },
    },
  ]);
}

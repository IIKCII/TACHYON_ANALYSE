import React, { useEffect } from 'react';
import { StatusBar } from 'expo-status-bar';
import * as Notifications from 'expo-notifications';
import { AppNavigator } from './src/navigation/AppNavigator';
import {
  setupNotificationChannels,
  registerAckCategory,
  rescheduleAllAlarms,
} from './src/utils/alarmManager';
import { useShiftStore } from './src/store/shiftStore';

// How notifications behave while the app is in foreground
Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true,
    shouldPlaySound: true,
    shouldSetBadge: false,
  }),
});

export default function App() {
  const { config, notificationsGranted } = useShiftStore();

  useEffect(() => {
    (async () => {
      await setupNotificationChannels();
      await registerAckCategory();
    })();
  }, []);

  // Handle "Snooze +10 min" action from notification
  useEffect(() => {
    const sub = Notifications.addNotificationResponseReceivedListener(async (response) => {
      if (response.actionIdentifier === 'snooze') {
        const snoozeTime = new Date(Date.now() + 10 * 60 * 1000);
        await Notifications.scheduleNotificationAsync({
          content: {
            ...response.notification.request.content,
            title: `⏰ (Snooze) ${response.notification.request.content.title}`,
            sticky: true,
            android: { channelId: 'alarm', sticky: true, priority: 'max' },
          },
          trigger: {
            type: Notifications.SchedulableTriggerInputTypes.DATE,
            date: snoozeTime,
          },
        });
      }
    });
    return () => sub.remove();
  }, []);

  return (
    <>
      <StatusBar style="light" />
      <AppNavigator />
    </>
  );
}

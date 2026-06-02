import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Text } from 'react-native';

import { HomeScreen } from '../screens/HomeScreen';
import { ScheduleScreen } from '../screens/ScheduleScreen';
import { RemindersScreen } from '../screens/RemindersScreen';
import { SettingsScreen } from '../screens/SettingsScreen';

const Tab = createBottomTabNavigator();

function TabIcon({ label, focused }: { label: string; focused: boolean }) {
  const icons: Record<string, string> = {
    Heute: '🏠',
    Plan: '📅',
    Erinnerungen: '🔔',
    Einstellungen: '⚙️',
  };
  return (
    <Text style={{ fontSize: focused ? 22 : 18, opacity: focused ? 1 : 0.5 }}>
      {icons[label]}
    </Text>
  );
}

export function AppNavigator() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ focused }) => <TabIcon label={route.name} focused={focused} />,
          tabBarStyle: {
            backgroundColor: '#0f172a',
            borderTopColor: '#1e293b',
            height: 64,
            paddingBottom: 8,
          },
          tabBarActiveTintColor: '#60a5fa',
          tabBarInactiveTintColor: '#475569',
          tabBarLabelStyle: { fontSize: 11, fontWeight: '600' },
          headerStyle: { backgroundColor: '#0f172a', shadowColor: 'transparent' },
          headerTintColor: '#f1f5f9',
          headerTitleStyle: { fontWeight: '700', fontSize: 18 },
        })}
      >
        <Tab.Screen name="Heute" component={HomeScreen} options={{ title: 'Meine Schicht' }} />
        <Tab.Screen name="Plan" component={ScheduleScreen} options={{ title: 'Schichtplan' }} />
        <Tab.Screen name="Erinnerungen" component={RemindersScreen} options={{ title: 'Erinnerungen' }} />
        <Tab.Screen name="Einstellungen" component={SettingsScreen} options={{ title: 'Einstellungen' }} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}

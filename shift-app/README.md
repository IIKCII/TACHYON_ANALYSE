# SchichtApp

Persönliche Android-App für 3-Schicht-Arbeit.

## Features (MVP)

- **Automatischer Wecker**: Öffnet die Android-Uhr-App mit vorausgefüllter Zeit basierend auf deinem Schichtplan
- **Lokale Notifications**: Werden 14 Tage im Voraus geplant, klingeln auch ohne offene App
- **Hartnäckige Erinnerungen**: Können nur über „Erledigt" Button bestätigt werden — kein Wegwischen
- **Schichtplan-Kalender**: 4-Wochen-Übersicht mit Schichttyp und Weckzeit
- **Konfigurierbar**: Schichtzeiten, Vorlauf, Zyklusmuster und Startdatum anpassbar

## Setup

```bash
cd shift-app
npm install
npm run android   # mit verbundenem Android-Gerät oder Emulator
```

## APK bauen (ohne Google Play)

```bash
npm install -g eas-cli
eas login
eas build -p android --profile preview
```

Dann APK runterladen und auf dem Handy installieren (USB oder Browser).

## Schicht-Konfiguration

1. **Einstellungen** → Schichtzeiten einstellen (Früh/Spät/Nacht Startzeit)
2. **Einstellungen** → Zyklusstart auf deinen ersten Tag setzen
3. **Einstellungen** → „Alle Wecker neu planen" drücken
4. → Fertig! App plant jetzt automatisch Wecker für 14 Tage

## Geplante Features (nach und nach)

- [ ] Automatische Wecker via Android AlarmManager (ohne Bestätigung)
- [ ] Widget für Homescreen (heute + morgen Schicht)
- [ ] Checklisten pro Schichttyp (was nicht vergessen)
- [ ] Urlaub/Tausch einplanen (Überschreibung einzelner Tage)
- [ ] Arbeitszeit-Tracker
- [ ] Dunkelmodus / Farb-Themes

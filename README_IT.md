# HDRTray con Gestione Profili Colore - Guida Rapida

## Introduzione

Questa versione di HDRTray integra la gestione automatica dei profili ICC e la calibrazione del monitor quando si passa tra modalità HDR e SDR, basandosi sul batch script originale del progetto Xiaomi 27i.

## Installazione Rapida

### 1. Compila il Progetto

```bash
# Dalla cartella HDRTray-main
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 2. Prepara i File Necessari

#### Opzione A: Script Automatico
```bash
# Esegui lo script di setup
setup_files.bat
```

#### Opzione B: Manuale
1. Crea le cartelle `bin` e `profiles` dentro `build\Release\`
2. Scarica e installa i tool necessari (vedi sotto)
3. Copia i profili ICC e i file di calibrazione

### 3. Strumenti Necessari

#### dispwin.exe (ArgyllCMS)
1. Scarica da: https://www.argyllcms.com/
2. Estrai l'archivio
3. Copia `dispwin.exe` dalla cartella `bin` in `build\Release\bin\dispwin.exe`

#### winddcutil.exe
1. Cerca "WinDDCUtil" o "ControlMyMonitor" online
2. Copia l'eseguibile come `build\Release\bin\winddcutil.exe`

**Alternative per winddcutil.exe:**
- ControlMyMonitor (NirSoft)
- ddcset
- Monitor Asset Manager

### 4. Profili Colore

Copia i seguenti file in `build\Release\profiles\`:
- `Xiaomi 27i Pro_Rtings.icm` - Profilo ICC per modalità SDR
- `xiaomi_miniled_1d.cal` - File di calibrazione per modalità HDR

Se hai già questi file nel progetto Xiaomi 27i, usa:
```bash
copy_profiles_example.bat
```

## Utilizzo

1. Avvia `HDRTray.exe`
2. Comparirà un'icona nella barra delle applicazioni che mostra "SDR" o "HDR"
3. **Click sinistro** sull'icona per passare da HDR a SDR (e viceversa)
4. La calibrazione e i profili colore verranno applicati automaticamente

## Cosa Fa Automaticamente

### Quando Attivi HDR (SDR → HDR)
- Attiva la modalità HDR di Windows
- Resetta e riattiva HDR (necessario per la calibrazione)
- Carica il file di calibrazione HDR (`xiaomi_miniled_1d.cal`)
- Imposta il monitor su:
  - Luminosità: 100%
  - Preset colore: 12
  - Gain RGB: 46, 49, 49

### Quando Disattivi HDR (HDR → SDR)
- Disattiva la modalità HDR di Windows
- Carica il profilo ICC per SDR (`Xiaomi 27i Pro_Rtings.icm`)
- Imposta il monitor su:
  - Luminosità: 50%
  - Gain RGB: 50, 49, 49

## Verifica Funzionamento

### Controlla che tutto sia a posto:
```bash
cd build\Release
dir bin          # Dovresti vedere dispwin.exe e winddcutil.exe
dir profiles     # Dovresti vedere i file .icm e .cal
```

### Debug
Se qualcosa non funziona:
1. Scarica **DebugView** da Microsoft Sysinternals
2. Avvia DebugView prima di HDRTray
3. Fai il toggle HDR/SDR
4. DebugView mostrerà i log di debug con eventuali errori

## Configurazione Monitor

### Il tuo monitor è diverso?

Se hai un monitor diverso dallo Xiaomi 27i Pro, dovrai:

1. **Modificare i codici VCP** in `ColorProfileManager.cpp`:
   - Trova le funzioni `ApplySDRProfile()` e `ApplyHDRCalibration()`
   - Modifica i valori VCP secondo le specifiche del tuo monitor

2. **Usare i tuoi profili ICC**:
   - Crea un profilo con DisplayCAL o simili
   - Rinominalo come `Xiaomi 27i Pro_Rtings.icm` o modifica il nome in `ColorProfileManager.hpp`

### Come trovare i codici VCP del tuo monitor?

1. Usa **ControlMyMonitor** (NirSoft) per vedere i codici supportati
2. Consulta il manuale del monitor per DDC/CI
3. Cerca online "nome_monitor DDC/CI VCP codes"

## Problemi Comuni

### "Color profile tools not available"
- Verifica che `dispwin.exe` e `winddcutil.exe` siano nella cartella `bin`
- Controlla i permessi dei file

### I profili non vengono caricati
- Verifica che i file `.icm` e `.cal` siano nella cartella `profiles`
- Controlla che i nomi dei file corrispondano esattamente
- Usa DebugView per vedere gli errori

### Il monitor non cambia impostazioni
- Abilita DDC/CI nel menu OSD del monitor
- Alcuni monitor chiamano questa opzione "DDC/CI", "External Control", o "PC Control"
- Prova a testare `winddcutil.exe` manualmente dal prompt dei comandi

### HDRTray funziona ma senza gestione profili
- Questo è normale! Se i tool esterni non sono trovati, HDRTray continua a funzionare per il toggle HDR/SDR
- La gestione profili è una feature opzionale

## File del Progetto

### File Principali
- `ColorProfileManager.hpp/cpp` - Gestione profili colore
- `NotifyIcon.cpp` - Integrazione con HDRTray
- `setup_files.bat` - Script di configurazione
- `COLOR_PROFILE_SETUP.md` - Documentazione dettagliata (inglese)

### Directory Runtime
```
build/Release/
├── HDRTray.exe                    # Applicazione principale
├── HDRCmd.exe                     # Utility command line
├── bin/
│   ├── dispwin.exe               # Gestore profili ICC
│   └── winddcutil.exe            # Controllo DDC/CI
└── profiles/
    ├── Xiaomi 27i Pro_Rtings.icm # Profilo SDR
    └── xiaomi_miniled_1d.cal      # Calibrazione HDR
```

## Link Utili

- **HDRTray originale**: https://github.com/res2k/HDRTray
- **ArgyllCMS**: https://www.argyllcms.com/
- **DisplayCAL**: https://displaycal.net/
- **ControlMyMonitor**: https://www.nirsoft.net/utils/control_my_monitor.html
- **DebugView**: https://docs.microsoft.com/en-us/sysinternals/downloads/debugview

## Supporto

Per problemi o domande:
1. Consulta `COLOR_PROFILE_SETUP.md` per la documentazione completa
2. Controlla i log con DebugView
3. Verifica che DDC/CI sia abilitato sul monitor
4. Apri una issue su GitHub

## Licenza

Questo progetto mantiene la licenza GPL-3.0 di HDRTray originale.

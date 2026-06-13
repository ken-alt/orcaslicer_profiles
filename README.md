# OrcaSlicer Profiles — Voron 2.4 & Trident

OrcaSlicer configuration backups for two Voron printers. Backed up automatically each night via `backup-orcaslicer.sh`.

## Printers

| Printer | Size | Toolhead board | Nozzle |
|---|---|---|---|
| Voron 2.4 | 350mm | EBBCan + SKR 1.4 | 0.4mm E3D Revo HF |
| Voron Trident | 250mm | EBBCan | 0.4mm E3D Revo HF |

Print hosts: `http://voron24.local` / `http://trident.local`

## Profile Structure

```
profiles/
  filament/   — per-filament settings (temps, PA, flow, fan, shrinkage)
  machine/    — printer definitions (bed size, speeds, PRINT_START gcode)
  process/    — layer height / print quality presets
```

## Filament Profiles

| Profile | Printer |
|---|---|
| ABS - Siraya Tech HT HF - VORON 2.4 | Voron 2.4 |
| ABS - Siraya Tech HT HF - TRIDENT | Voron Trident |
| ABS - Polymaker | Both |
| ASA - Ambrosia | Both |
| PLA - Elegoo Silk - 2.4 | Voron 2.4 |

## Process Profiles

| Profile | Use |
|---|---|
| 0.20mm @ Voron - Master Profile - LARGE PARTS | Most prints |
| 0.20mm @ Voron - Master Profile - MEDIUM PARTS | Medium parts |
| 0.15mm @ Voron - Master Profile - SMALL PARTS | Fine detail / small parts |
| 0.10mm Layer - Standard 0.25 nozzle @Voron | 0.25mm nozzle work |

## Key Filament Settings — Siraya Tech HT HF ABS (V2.4)

| Setting | Value |
|---|---|
| Nozzle temp | 246°C |
| Bed temp | 110°C |
| Flow ratio | 0.93 |
| PA (static fallback) | 0.035 |
| Adaptive PA | Enabled — 9-point model (3 speeds × 3 accels) |
| Max volumetric speed | 14 mm³/s |
| Fan min / max | 15% / 25% |
| XY shrinkage | 99.4% |
| Z shrinkage | 99.2% |

## Backup

Profiles are synced automatically each night at 9pm by `backup-orcaslicer.sh`. To run manually:

```bash
"$HOME/_Claude/3D Printers/orcaslicer_profiles/backup-orcaslicer.sh"
```

## Related Repos

- Voron 2.4 Klipper config: https://github.com/ken-alt/voron24_configs
- Voron Trident Klipper config: https://github.com/ken-alt/trident_klipper_config

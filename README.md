# OrcaSlicer Profiles — Voron 2.4 & Trident

OrcaSlicer configuration backups for two Voron printers.

## Printers

| Printer | Size | Board | Nozzle |
|---|---|---|---|
| Voron 2.4 | 350mm | SKR 1.4 + EBBCan | 0.4mm E3D Revo HF |
| Voron Trident | 250mm | Octopus Pro + EBBCan | 0.4mm E3D Revo HF |

## Files

### `Voron 2.4 350 - 0.4 nozzle.orca_printer`
Printer bundle for the Voron 2.4 350mm. Includes:
- Machine profile (printable area, speeds, PRINT_START gcode)
- All filament profiles compatible with this printer
- All process profiles

Print host: `http://voron24.local`

### `Voron Trident 250 - 0.4 nozzle.orca_printer`
Printer bundle for the Voron Trident 250mm. Includes:
- Machine profile (printable area, speeds, PRINT_START gcode)
- All filament profiles compatible with this printer
- All process profiles

Print host: `http://trident.local`

### `Generic ABS.orca_filament`
Standalone filament profile export for all ABS variants including:
- ABS - Siraya Tech HT HF - VORON 2.4
- ABS - Siraya Tech HT HF - TRIDENT
- ABS - Fusion (2.4 and Trident)
- ABS - KVP
- ABS - Polymaker

### `Process presets.zip`
Standalone process profile export:
- `0.20mm @ Voron - Master Profile - MEDIUM PARTS` — main profile for most prints
- `0.20mm @ Voron - Master Profile - LARGE PARTS` — large prints
- `0.15mm @ Voron - Master Profile - SMALL PARTS` — fine detail / small parts
- `0.20mm @ Voron Parts` — Voron structural parts

## Key Filament Settings — Siraya Tech HT-HF ABS

| Setting | Value |
|---|---|
| Nozzle temp | 246°C |
| Bed temp (first layer) | 110°C |
| Bed temp (subsequent) | 105°C |
| Flow ratio | 0.93 |
| PA — V2.4 | 0.042 |
| PA — Trident | 0.043 |
| Max volumetric speed | 14 mm³/s |
| Fan min | 15% (always on) |
| Fan max | 25% |
| XY shrinkage | 99.4% |
| Z shrinkage | 99.2% |

## Updating

When profiles change, export from OrcaSlicer and push:

```bash
cd ~/orcaslicer_profiles
# Copy new exports here
git add .
git commit -m "Update profiles"
git push origin main
```

## Related Repos

- Voron 2.4 Klipper config: https://github.com/ken-alt/voron24_configs
- Voron Trident Klipper config: https://github.com/ken-alt/trident_klipper_config

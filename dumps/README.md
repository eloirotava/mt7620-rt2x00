# Estado do radio: rt2x00 x rt2860v2

Archer C5 v4, OpenWrt 25.12.2, 2026-10-14.  AP `AP31-teste24`, canal 11,
HT20, WPA2-PSK, sem cliente.  Lido com `regdump` (dois dumps de cada
driver com 5 s de intervalo; `-a`/`-b`).

- `diff-mac.txt` e `diff-bbp-rf.txt`: diferencas estaveis entre os drivers
  (os 51 registradores que mudam entre `-a` e `-b` de um mesmo driver
  ficam de fora).

Candidatos para a vazao, do MAC:

| registrador | rt2x00 | rt2860v2 |
|---|---|---|
| `TX_RTY_CFG` (0x134c) | short 7 / long 4 | short 15 / long 31 |
| `MAX_LEN_CFG` (0x1018) MAX_PSDU | 1 (16 KB) | 3 (64 KB) |
| `EDCA_AC0_CFG` (0x1300) TXOP BE | 0 | 0x20 |
| `*_PROT_CFG` (0x1364-0x1378) | TX_OP_ALLOW menor | mais modos |

Do lado de RX/RF: BBP 66 (ganho, o link tuner reescreve), BBP 84,
GLRT 131/134/156/157, DCOC 31-89, RF bancos 5 e 7 (regs 4, 39-51) e
RF 0/34-35.

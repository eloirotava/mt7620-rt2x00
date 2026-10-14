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

Medido em 2026-10-14 (A71 a -63 dBm, iperf3 C5<->A71, 2x10 s): nenhum
desses ajustes nem os grupos BBP/GLRT/DCOC/RF do vendor (aplicados com
`regdump` sobre o rt2x00) mudaram a vazao alem do ruido (+-5 Mbit/s).

| condicao | rt2x00 de fabrica | rt2860v2 |
|---|---|---|
| canal 13, Cudy 2.4 desligado | down 25-26, up ~9 | down 15-19, up ~10 |
| canal 11 (cheio), Cudy ligado | down ~12, up 2-5 | down 8-12, up 3-6 |

Com este cliente o rt2x00 empata no download; a diferenca de 2x do S22
em 2026-10-08 nao se repetiu.  Sobra talvez o upload.

Bateria limpa (2026-10-14: canal 13, 2.4 do Cudy desligado, A71 parado,
iperf3 20 s, drivers alternados; retries de TX_STA_CNT do chip, no
rt2860v2 via patch 005 do mt7620-rt2860v2):

| rodada | rt2x00 down | retry/falha | rt2860v2 down | retry/falha |
|---|---|---|---|---|
| 1 | 18,5 | 31,6% / 0,08% | 9,0 | 33,6% / 0 |
| 2 | 17,9 | 33,9% / 0,10% | 11,2 | 34,1% / 0 |
| 3 | 17,2 | 37,3% / 0,12% | 12,5 | 23,5% / 0 |

Upload (C5 so manda ACKs): rt2x00 5,8 / 6,2 / 6,3; rt2860v2 5,3 / 5,5 / 6,6.
Conclusao: o rt2x00 de fabrica tem ~60% mais download, upload igual e
retries na mesma faixa.  Unica diferenca a favor do vendor: ~0,1% de
quadros descartados (falha apos todos os retries) contra 0.

Do lado de RX/RF: BBP 66 (ganho, o link tuner reescreve), BBP 84,
GLRT 131/134/156/157, DCOC 31-89, RF bancos 5 e 7 (regs 4, 39-51) e
RF 0/34-35.

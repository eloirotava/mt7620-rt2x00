# mt7620-rt2x00

Melhorias no driver `rt2x00` (`rt2800soc`) para o wifi 2.4 GHz interno do
MediaTek MT7620 (RT6352/RF7620), usando o driver da MediaTek (`rt2860v2`)
como referencia.  Alvo de teste: TP-Link Archer C5 v4, OpenWrt 25.12.2.

- `patches/` — patches do `package/kernel/mac80211/patches/rt2x00`.
- `.github/workflows/kmod.yml` — compila os modulos com o SDK oficial
  25.12.2 ramips/mt7620 (mesmo kernel da imagem).
- `scripts/deploy-test.sh` — troca os modulos no roteador em runtime.

## Linha de base (rt2x00 do 25.12.2)

S22 a -36/-42 dBm, canal 11, HT20, MCS 15 (144 Mbps de PHY):

| direcao | throughput | retries de TX |
|---|---|---|
| AP -> cliente | 28,8 / 34,4 Mbps | ~26% |
| cliente -> AP | 15-21 Mbps | - |

CPU do roteador em 33% durante o teste: o limite e o radio/driver.
Hipotese principal: falta a calibracao DPD do PA interno, que o driver
da MediaTek faz em toda troca de canal e por variacao de temperatura.

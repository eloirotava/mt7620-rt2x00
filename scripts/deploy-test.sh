#!/bin/sh
# Troca os modulos rt2x00 do Archer pelos de um diretorio local (so em
# /tmp: um reboot volta aos originais) e religa o radio 2.4.
# Uso: deploy-test.sh <dir com os .ko> [ssh destino]
set -e
dir=$1
dst=${2:-root@192.168.1.2}
jump=${JUMP:-root@10.8.0.4}
ssh_() { sshpass -e ssh -o ConnectTimeout=10 -J "$jump" "$dst" "$@"; }

ssh_ 'rm -rf /tmp/rt2x00 && mkdir -p /tmp/rt2x00'
for k in "$dir"/*.ko; do
	ssh_ "cat > /tmp/rt2x00/$(basename "$k")" < "$k"
done
ssh_ 'cd /tmp/rt2x00 &&
	rmmod rt2800soc rt2800mmio rt2800lib rt2x00mmio rt2x00lib &&
	insmod ./rt2x00lib.ko && insmod ./rt2x00mmio.ko &&
	insmod ./rt2800lib.ko && insmod ./rt2800mmio.ko && insmod ./rt2800soc.ko &&
	sleep 3 && wifi up radio1 && sleep 10 &&
	dmesg | grep -iE "rt2800|rt2x00|phy1" | tail -15 &&
	iw dev | grep -A2 phy1'

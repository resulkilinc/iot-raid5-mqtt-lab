# Fault-tolerant IoT + RAID5 Lab

Proxmox üzerinde Debian 12 sanal makinede kurulmuş laboratuvar çalışması: **mdadm RAID5** depolama, **C** ile simüle edilmiş IoT sensör düğümü, **Mosquitto MQTT** (anonim kapalı), **systemd** servisleri, RAID izleme, RAID dışı yedek + SHA256 doğrulama, **fio** ile normal/degraded performans karşılaştırması ve temel **iptables** + tcpdump.

> Bu depo staj defteri, şirket belgeleri veya gizli arşiv içermez. Ortam adları ve adresler anonimleştirilmiştir.

## Mimari

```
[sensor_node (C)] --MQTT:1883--> [Mosquitto]
        | writes CSV
        v
   /mnt/raid5  (/dev/md0 RAID5)
        ^
        | monitored by
   raid_monitor.sh (timer)
```

## Bileşenler

| Parça | Rol |
|-------|-----|
| `src/sensor_node.c` | Sahte ADC (sıcaklık/nem/voltaj), CSV kayıt, opsiyonel MQTT |
| `scripts/raid_monitor.sh` | `mdstat` kontrolü, degraded uyarısı |
| `scripts/backup_restore_demo.sh` | RAID dışı kopya + SHA256 + restore demosu |
| `systemd/*` | `sensor-node.service`, `raid-monitor.service/.timer` |
| `docs/architecture.md` | Kurulum ve test özeti |
| `evidence/` | Anonimleştirilmiş komut çıktı örnekleri |

## Hızlı demolar (lab VM içinde)

```bash
# RAID5 örneği (4 disk, 1 parity) — kendi disk adlarına uyarlayın
sudo mdadm --create /dev/md0 --level=5 --raid-devices=4 /dev/sd{b,c,d,e}
sudo mkfs.ext4 /dev/md0
sudo mkdir -p /mnt/raid5 && sudo mount /dev/md0 /mnt/raid5

# Sensör düğümü
gcc -O2 -o sensor_node src/sensor_node.c -lm -lmosquitto   # MQTT ile
# veya MQTT olmadan derleme için kaynak içindeki USE_MQTT=0

./sensor_node --samples 5
./sensor_node --daemon --interval-ms 2000 --mqtt
```

## Ne öğrendim (özet)

- RAID5 tek disk arızasında erişilebilirliği sürdürür; silme / tüm host kaybına karşı yedek değildir.
- Degraded durumda sıralı yazma hızı “beklenen yavaşlama”ya her zaman uymaz; ölçüm koşullarıyla birlikte yorumlanmalı.
- MQTT’de `allow_anonymous false` + kullanıcı/parola, tcpdump ile kanıtlanabilir trafik yakalama.
- İzleme (timer) + RAID dışı yedek + snapshot, birlikte felaket kurtarma hikâyesini tamamlar.

## Kasıtlı olarak eklenmeyenler

- Staj defteri / PDF / fotoğraflar
- Kurumsal IP, parola, anahtar
- Şirket araştırma notları ve gizli arşiv

## Lisans

Eğitim / portföy amaçlı örnek. İstediğin gibi kullan, uyarla.

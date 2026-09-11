# Mimari ve test akışı

## Ortam (anonim)

- Hipervizör: Proxmox VE
- Misafir: Debian 12 (`raid-lab`)
- Depolama: `mdadm` RAID5 → `/mnt/raid5`
- Ağ: lab köprüsü; servisler SSH (22) ve MQTT (1883)

## Kurulum sırası (özet)

1. VM’ye ek sanal diskler ekle, `lsblk` ile doğrula
2. RAID5 oluştur, `ext4` + `fstab`
3. Mosquitto kur, anonim erişimi kapat, lab kullanıcısı tanımla
4. `sensor_node` derle; CSV’yi RAID üzerine yaz; systemd ile daemon
5. `raid_monitor` timer’ı etkinleştir
6. RAID dışı dizine yedek + SHA256; silme/geri yükleme deneyi
7. `fio` ile normal ve degraded yazma/okuma
8. iptables zinciri (22/1883) + mqtt pcap örneği
9. Snapshot al (geri dönüş noktası)

## Kanıt tipi

`evidence/` altındaki dosyalar gerçek lab çıktılarından **IP ve kurumsal isimler temizlenerek** üretilmiş örneklerdir. Üretim sistemlerine ait yapılandırma yoktur.

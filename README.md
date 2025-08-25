# ESP32-C3 Massage Device

Skema dasar:
- Baterai Li-ion 7,4V masuk ke modul step-down 5V untuk memasok ESP32-C3 Super Mini dan driver.
- Driver H-bridge (misal DRV8833/L298N) diberi suplai langsung dari baterai untuk menggerakkan motor DC 7,4V.

- Pin GPIO3 dan GPIO4 mengatur arah motor, GPIO5 sebagai PWM untuk empat tingkat kecepatan (100%, 75%, 50%, 25%).
- Modul pemanas dikendalikan melalui MOSFET N-channel pada GPIO6.
- Tombol power pada GPIO7, tombol pengatur kecepatan pada GPIO8, dan tombol on/off pemanas pada GPIO9 (semua ke GND dengan INPUT_PULLUP).

- Untuk pengisian dan proteksi baterai gunakan rangkaian BMS Li-ion 2S.

Fitur perangkat lunak (lihat `massager.ino`):
- Kontrol on/off dengan deep sleep untuk hemat daya.
- Motor dua arah dengan empat level kecepatan.
- Kontrol modul penghangat.
- Tiga tombol fisik: power (tekan 3 detik untuk off), pengatur kecepatan, dan on/off pemanas.
- Web manager: login admin, pengaturan WiFi, ganti/reset password, OTA update via file, pengaturan menit auto-off.
- Desain UI web responsif dengan dominan warna biru tosca.



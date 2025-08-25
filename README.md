# ESP32-C3 Massage Device

Skema dasar:
- Baterai Li-ion 7,4V masuk ke modul step-down 5V untuk memasok ESP32-C3 Super Mini dan driver.
- Driver H-bridge L298N (versi tanpa pin ENA/ENB) diberi suplai langsung dari baterai untuk menggerakkan motor DC 7,4V.
- Pin GPIO3 dan GPIO4 menjadi IN1 dan IN2. Kecepatan diatur lewat PWM pada salah satu pin IN, sedangkan pin IN lain diberi level LOW untuk menentukan arah.
- Modul pemanas dikendalikan melalui MOSFET N-channel pada GPIO5.
- Tombol power pada GPIO6, tombol pengatur kecepatan pada GPIO7, dan tombol on/off pemanas pada GPIO8 (semua ke GND dengan INPUT_PULLUP).
- Untuk pengisian dan proteksi baterai gunakan rangkaian BMS Li-ion 2S.

Fitur perangkat lunak (lihat `massager.ino`):
- Kontrol on/off dengan deep sleep untuk hemat daya.
- Motor dua arah dengan empat level kecepatan.
- Kontrol modul penghangat.
- Tiga tombol fisik: power (tekan 3 detik untuk off), pengatur kecepatan, dan on/off pemanas.
- Web manager: login admin, pengaturan WiFi, ganti/reset password, OTA update via file, pengaturan menit auto-off.
- Desain UI web responsif dengan dominan warna biru tosca.


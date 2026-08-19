# Panduan Build Btop WebGUI Teroptimasi untuk Amlogic s905x

Dokumen ini berisi panduan cara meng-compile `btop` dengan fitur WebGUI yang sudah **dioptimasi khusus** untuk perangkat dengan spesifikasi rendah seperti Amlogic s905x (Cortex-A53).

Optimasi yang telah diterapkan pada *source code* ini meliputi:
- Pengurangan jumlah payload daftar proses (maksimal 100 proses teratas) untuk meringankan kerja CPU saat pembuatan string JSON (`nlohmann::json`) di *backend*.
- Penyesuaian siklus *polling* API (`setInterval`) dari sisi Web UI, dari yang sebelumnya 1 detik (1000ms) menjadi 2 detik (2000ms), menyesuaikan dengan kecepatan update metrik bawaan `btop`.

Ada dua cara untuk meng-compile program ini ke device Anda:

---

## Metode 1: Cross-Compiler di PC Linux x86_64 (Sangat Disarankan)
Metode ini paling direkomendasikan karena perangkat s905x umumnya memiliki RAM yang terbatas (1-2GB). Proses kompilasi kode bahasa C++ yang berat di dalam perangkat Amlogic sangat berisiko membuat sistem *freeze*, gagal *(Out-of-Memory)*, dan memakan waktu lama.

### Langkah-langkah:
1. **Install Toolchain Cross-Compiler** (di PC Ubuntu/Debian Anda):
   ```bash
   sudo apt update
   sudo apt install g++-aarch64-linux-gnu binutils-aarch64-linux-gnu
   ```
2. **Bersihkan Sisa Kompilasi Sebelumnya**:
   Pastikan tidak ada sisa file *binary* berarsitektur PC dengan cara menjalankan:
   ```bash
   make clean
   ```
3. **Compile dengan Flag Spesifik ARM64 & Cortex-A53**:
   Jalankan perintah kompilasi menggunakan variabel *cross-compiler* beserta *flag* optimasi kinerjanya:
   ```bash
   make CXX=aarch64-linux-gnu-g++ STRIP=aarch64-linux-gnu-strip OPTFLAGS="-O3 -mcpu=cortex-a53" -j4
   ```
4. **Verifikasi Binary**:
   Pastikan hasil file biner-nya berformat aarch64:
   ```bash
   file bin/btop
   ```
   *(Teks output harus menyertakan kata `ARM aarch64`)*
5. **Kirim ke Perangkat Amlogic Anda**:
   Gunakan perintah `scp` (atau aplikasi FTP seperti FileZilla) untuk memindahkan file ke device s905x Anda:
   ```bash
   scp bin/btop user@ip-device-amlogic:/home/user/
   ```

---

## Metode 2: Compile Langsung di Device Amlogic s905x (Native)
Metode ini dijalankan langsung lewat terminal (SSH) di device s905x Anda. **PENTING:** Pastikan perangkat Anda memiliki file *Swap Space* aktif minimal sebesar 1GB - 2GB agar tidak kehabisan RAM.

### Langkah-langkah:
1. **Install Build Dependencies** (di perangkat Amlogic Anda):
   ```bash
   sudo apt update
   sudo apt install build-essential git make g++
   ```
2. **Bersihkan Sisa Kompilasi Sebelumnya**:
   ```bash
   make clean
   ```
3. **Compile dengan Optimasi Native**:
   Karena dijalankan langsung di perangkat sasarannya, cukup tambahkan atribut `-march=native` agar *compiler* yang beradaptasi menyesuaikan fitur Cortex-A53 itu sendiri:
   ```bash
   make OPTFLAGS="-O3 -march=native"
   ```
   *(Catatan: Sebaiknya **jangan** menambahkan akhiran `-j4` atau melakukan *multithreaded compile* agar RAM tidak langsung ludes).*

---

## Cara Menjalankan Btop WebGUI
Setelah file executable `btop` sudah jadi dan tersimpan di Amlogic s905x Anda, jalankan programnya dengan argumen `--web`:

```bash
./btop --web
```

Jika Anda ingin menentukannya di *port* spesifik secara manual (contoh: port 8080):
```bash
./btop --web=8080
```

Terakhir, buka browser Anda dari PC/HP dan akses IP Amlogic tersebut:
```
http://<IP-DEVICE-AMLOGIC>:<PORT>
```
Nikmati btop dengan *Dashboard* Web yang sudah jauh lebih enteng!

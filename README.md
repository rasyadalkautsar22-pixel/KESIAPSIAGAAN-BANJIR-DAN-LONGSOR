# KESIAPSIAGAAN-BANJIR-DAN-LONGSOR
# Sistem Pakar Kesiapsiagaan Banjir dan Longsor (Forward Chaining, C++)

## 1. Identitas

- Nama: Muhamad Rasyad Al-Kautsar
- NIM: F1D02510143

## 2. Topik dan cara menjalankan

Topik: tindakan kesiapsiagaan yang tepat saat ada tanda banjir atau longsor. Program menanyakan 12 kondisi (jawab `y` atau `t`), lalu menyimpulkan tindakan yang perlu diambil beserta alasannya (aturan mana yang dipakai).

- Bahasa: C++ (standar C++11 atau lebih baru). Diuji dengan g++ 13.3 memakai `-std=c++17`. Tanpa library eksternal; pembaca JSON dan mesin inferensi ditulis sendiri.
- File: `fc_banjir.cpp` (program), `kb_banjir_longsor.json` (aturan).

Perintah (terminal):

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o fc_banjir fc_banjir.cpp
./fc_banjir kb_banjir_longsor.json
```

Di Windows (CMD): `fc_banjir.exe kb_banjir_longsor.json`. File JSON harus berada di folder yang sama dengan program, atau tulis path lengkapnya. Di Dev-C++, tekan F11 setelah file JSON disalin ke folder `.exe`.

Menjalankan 3 kasus uji otomatis (Linux/macOS): `./run_tests.sh`. Keluarannya tersimpan di `hasil_uji.txt`.

## 3. Sumber dan tabel aturan

Semua sumber adalah situs pemerintah, diakses 11 November 2026.

| Kode | Judul | Penerbit | URL | Tanggal akses |
|---|---|---|---|---|
| S1 | Tanggap, Tangkas, Tangguh Menghadapi Bencana (bagian Banjir dan Tanah Longsor) | BNPB | https://www.bnpb.go.id/storage/app/media/uploads/migration/pubs/478.pdf | 11 Nov 2026 |
| S2 | PVMBG: Longsor di Solok Akibat Penggalian Lereng dan Curah Hujan Tinggi (28 Sep 2024) | Kementerian ESDM (PVMBG, Badan Geologi) | https://www.esdm.go.id/en/media-center/news-archives/pvmbg-longsor-di-solok-akibat-penggalian-lereng-dan-curah-hujan-tinggi | 11 Nov 2026 |
| S3 | Prosedur Peringatan Dini dan Evakuasi Darurat Banjir | PPID Pemerintah Provinsi DI Yogyakarta | https://ppid.jogjaprov.go.id/informasi/unduh/f61a0df4-e67b-4bc1-a441-c2d98246c3fc | 11 Nov 2026 |

Hal yang diambil dari sumber:
- S1: hujan di atas normal dan air laut pasang sebagai penyebab banjir, larangan membangun di bantaran sungai, tanda longsor (retakan vertikal, air tanah muncul tiba-tiba, air sumur keruh, longsoran batu kecil), mengungsi sedini mungkin, mematikan listrik.
- S2: hujan deras yang lama dan air yang meresap ke lereng memicu longsor, waspada saat hujan berlangsung lama, ikuti arahan BPBD.
- S3: cek zona rawan banjir, tas siaga dan bekal, pantau informasi resmi, pindah ke tempat lebih tinggi, jangan menyentuh alat listrik saat berdiri di air.

Ada 10 aturan dan 5 kesimpulan akhir (cetak tebal). Mode `dan` berarti semua syarat harus ada, `atau` berarti salah satu cukup.

| Aturan | IF | THEN | Sumber |
|---|---|---|---|
| R1 | hujan deras berlangsung lama **OR** air laut pasang | curah air tinggi | S1, S2 |
| R2 | curah air tinggi **AND** tinggal di bantaran sungai atau dataran banjir | risiko banjir tinggi | S1, S3 |
| R3 | risiko banjir tinggi **AND** air menggenang di sekitar rumah | **segera mengungsi ke tempat yang lebih tinggi atau posko banjir** | S1, S3 |
| R4 | ada retakan pada tanah atau tebing **OR** air sumur di sekitar tebing keruh **OR** air tanah muncul tiba-tiba **OR** ada longsoran batu kecil | tanda longsor muncul | S1 |
| R5 | curah air tinggi **AND** tinggal di dekat lereng atau tebing | lereng berisiko jenuh air | S1, S2 |
| R6 | tanda longsor muncul **AND** lereng berisiko jenuh air | bahaya longsor tinggi | S1, S2 |
| R7 | bahaya longsor tinggi | **segera menjauh dari lereng dan mengungsi ke tempat aman** | S1, S2 |
| R8 | air sudah masuk ke rumah **AND** listrik rumah masih menyala | **matikan listrik rumah dan jangan menyentuh alat listrik saat berdiri di air** | S1, S3 |
| R9 | risiko banjir tinggi **OR** bahaya longsor tinggi | **siapkan tas siaga bencana dan pantau informasi dari sumber resmi** | S3 |
| R10 | ada perintah mengungsi dari BPBD atau aparat desa | **ikuti arahan BPBD dan mengungsi ke tempat aman** | S2, S3 |

Rantai aturan (satu aturan memakai hasil aturan lain): R1 → R2 → R3 untuk banjir; R1 → R5 → R6 → R7 untuk longsor (R4 menjadi syarat kedua R6); R2 atau R6 → R9.

## 4. Metode: Forward Chaining

Metode yang dipilih adalah **forward chaining (FC)**. Alasannya:
- Pengguna biasanya hanya tahu kondisi yang terlihat (hujan lama, retakan, air masuk rumah), bukan kesimpulan yang dicari. FC bergerak dari fakta menuju kesimpulan, sesuai kebutuhan ini.
- Satu rangkaian fakta bisa menghasilkan beberapa kesimpulan sekaligus (misalnya mengungsi dan mematikan listrik), dan FC menemukan semuanya.
- Aturan berantai (hujan → curah air tinggi → risiko banjir → mengungsi) terlihat jelas sebagai siklus dan mudah dijelaskan sebagai alasan.

Cara kerja:

```
memori <- fakta yang dijawab "ya" oleh user
ULANGI:
    cocok <- aturan yang syaratnya terpenuhi oleh memori DAN hasilnya belum ada di memori
    JIKA cocok kosong: BERHENTI
    r <- cocok[0]                       # aturan pertama
    tambahkan r.maka ke memori
    catat bahwa r.maka berasal dari r   # bahan "alasan"
```

Aturan yang hasilnya sudah ada tidak dijalankan lagi, sehingga program pasti berhenti. Kesimpulan akhir ditampilkan bersama alasannya, yaitu aturan yang menghasilkannya dan aturan-aturan sebelumnya yang menjadi syaratnya.

## 5. Kasus uji dan screenshot

| # | Jawaban "ya" | Kesimpulan |
|---|---|---|
| 1 | hujan deras berlangsung lama, tinggal di bantaran sungai atau dataran banjir, air menggenang di sekitar rumah | segera mengungsi ke tempat yang lebih tinggi atau posko banjir; siapkan tas siaga bencana dan pantau informasi dari sumber resmi |
| 2 | tinggal di dekat lereng atau tebing saja | **tidak ada** (kasus negatif) |
| 3 | hujan deras berlangsung lama, ada retakan pada tanah atau tebing, tinggal di dekat lereng atau tebing, air sudah masuk ke rumah, listrik rumah masih menyala | segera menjauh dari lereng dan mengungsi ke tempat aman; matikan listrik rumah dan jangan menyentuh alat listrik saat berdiri di air; siapkan tas siaga bencana dan pantau informasi dari sumber resmi |

Urutan jawaban untuk 12 pertanyaan ada di `kasus1.txt`, `kasus2.txt`, dan `kasus3.txt`. Keluaran lengkap ketiga kasus ada di `hasil_uji.txt`.

Kasus 1 (menghasilkan kesimpulan):

![Kasus 1](![alt text](c:\Users\Pongo\Pictures\Screenshots\Screenshot 2026-11-13 203839.png?raw=true))

Kasus 2 (tidak menghasilkan kesimpulan):

![Kasus 2](screenshot/kasus2.png)

Kasus 3 (menghasilkan kesimpulan):

![Kasus 3](screenshot/kasus3.png)

## 6. Batas keberlakuan

Pengetahuan dalam program ini berasal dari tiga dokumen resmi: BNPB, PVMBG (Kementerian ESDM), dan Pemprov DI Yogyakarta, diakses 11 November 2026. Aturan berlaku untuk keadaan umum saat ada tanda hujan lebat, banjir, atau longsor di Indonesia, tanpa ambang angka (curah hujan, tinggi muka air) dan tanpa data lokasi tertentu. Hasilnya berupa saran, bukan keputusan akhir; selalu utamakan peringatan resmi BMKG dan arahan BPBD setempat. Catatan yang sama ditampilkan di akhir keluaran program.

## 7. Penggunaan alat bantu AI

Alat bantu AI (Claude, Anthropic) dipakai untuk mencari dan merangkum sumber, menyusun aturan IF-THEN, menulis kode C++ dan skrip uji, serta menyusun draf README dan laporan. **ISI BAGIAN INI SESUAI PEMAKAIAN SEBENARNYA**, misalnya apa yang Anda periksa atau ubah sendiri.

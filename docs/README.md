# Dokumentasi Arunimesta

Folder ini berisi dokumentasi teknis proyek Arunimesta. Dokumen ditulis berdasarkan kondisi kode saat ini; jika implementasi berubah, perbarui dokumen terkait.

## Daftar Dokumen

| Dokumen | Deskripsi |
| :--- | :--- |
| [architecture.md](architecture.md) | Arsitektur modul, pola pimpl, ownership, dan alur inisialisasi. |
| [building.md](building.md) | Prasyarat, cara build dengan preset maupun manual, dan troubleshooting. |
| [coding-style.md](coding-style.md) | Konvensi penamaan, layout file `.cppm`, dan pola factory/pimpl. |
| [api/application.md](api/application.md) | Referensi API `aru::Application`. |
| [roadmap.md](roadmap.md) | Status, milestone, dan rencana pengembangan. |

## Prinsip Dokumentasi

- **Sinkron dengan kode**: contoh dan nama API harus sesuai source di `modules/`.
- **Jujur soal status**: bagian yang masih kerangka ditandai eksplisit.
- **Praktis**: setiap dokumen menjawab pertanyaan yang muncul saat membangun atau mengembangkan engine.
- **Bahasa**: Indonesia, dengan istilah teknis tetap dalam bahasa Inggris bila lebih jelas.

## Mulai Dari Mana?

- Baru pertama kali build? Baca [building.md](building.md).
- Ingin memahami struktur kode? Baca [architecture.md](architecture.md).
- Ingin menambah modul atau mengubah API? Baca [coding-style.md](coding-style.md).
- Ingin tahu rencana berikutnya? Baca [roadmap.md](roadmap.md).

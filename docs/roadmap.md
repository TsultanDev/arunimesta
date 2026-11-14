# Roadmap

Dokumen ini berisi status, milestone, dan rencana pengembangan Arunimesta. Roadmap bersifat arahan, bukan kontrak; prioritas dapat berubah.

## Status Saat Ini

| Area | Status | Keterangan |
| :--- | :--- | :--- |
| Build system | ✅ Selesai | CMake ≥ 3.28, Clang, Ninja, preset Debug/Release |
| Named modules | ✅ Selesai | `aru` + partition `aru:application`, facade `lib.cppm` |
| `Application` | ✅ Dasar | Factory `std::expected`, pimpl `unique_ptr`, move-only |
| Windowing | 🚧 Berjalan | `aru.window` masih kerangka; belum masuk root build |
| Renderer Vulkan | ⬜ Belum | Belum ada source RHI |
| Testing | ⬜ Belum | Folder `test/` masih kosong |
| Dokumentasi | ✅ Awal | README + `docs/` |

## Milestone

### M0 — Fondasi (selesai)

- [x] Struktur workspace modular di `modules/`
- [x] C++23 named modules dan facade
- [x] Build preset Clang x64 Debug/Release
- [x] `Application` dengan `std::expected` dan pimpl
- [x] Logging debug-only
- [x] Dokumentasi awal

**Kriteria selesai:** proyek dapat di-configure dan di-build, executable `aru` berjalan dan mencetak informasi aplikasi.

### M1 — Windowing (sedang dikerjakan)

- [ ] Menyelesaikan class `Window` di `aru.window:window`
- [ ] Mengisi facade `aru.window` (`lib.cppm`)
- [ ] Menambahkan `modules/aru_window` ke root `CMakeLists.txt`
- [ ] Integrasi event loop dasar (polling)
- [ ] Membuat dan menghancurkan window dengan aman (RAII)
- [ ] Menambahkan contoh pemakaian di executable

**Kriteria selesai:** `aru` dapat membuat window, memproses event, dan keluar dengan bersih.

### M2 — Fondasi Vulkan RHI

- [ ] Modul `aru_rhi` (static library) dan facade `aru.rhi`
- [ ] Loader/instance Vulkan
- [ ] Debug messenger pada build debug
- [ ] Surface dari window
- [ ] Pemilihan physical device dan queue family
- [ ] Logical device dan queue

**Kriteria selesai:** instance, surface, dan device berhasil dibuat serta dihancurkan tanpa validation error.

### M3 — Presentasi

- [ ] Swapchain, image views, dan framebuffer
- [ ] Render pass
- [ ] Pemilihan format dan present mode
- [ ] Penanganan resize (recreate swapchain)

**Kriteria selesai:** frame kosong berhasil di-present ke window.

### M4 — Rendering Frame

- [ ] Command pool dan command buffer
- [ ] Sinkronisasi (fence/semaphore) dan frame in-flight
- [ ] Graphics pipeline dan shader
- [ ] Vertex/index buffer
- [ ] Render loop per-frame

**Kriteria selesai:** objek sederhana (misalnya segitiga) tampil dan beranimasi.

### M5 — Sistem Engine

- [ ] Resource management
- [ ] Matematika (misalnya GLM)
- [ ] ECS
- [ ] Asset pipeline
- [ ] Scene dan kamera
- [ ] PBR pipeline

## Arah Desain

- **Module per subsistem**: `aru`, `aru.window`, `aru.rhi`, dan modul pendukung lain.
- **Facade + partition**: konsumen mengimpor satu nama modul; implementasi dipecah internal.
- **`std::expected` di API publik**: kegagalan eksplisit, bukan exception.
- **Pimpl + `unique_ptr`**: detail implementasi tersembunyi, ownership jelas.
- **RAII**: semua resource dibersihkan oleh destructor.
- **Windows-first**: preset dan target saat ini Windows x64; dukungan platform lain menyusul.

## Utang Teknis

Prioritas pembersihan sebelum menambah subsistem besar:

1. Lengkapi atau hapus sementara `modules/aru_window` yang masih setengah jadi.
2. Daftarkan `aru_window` ke root `CMakeLists.txt` setelah selesai.
3. Ubah copy assignment `Application` menjadi `= delete`.
4. Ganti tipe error `bool` dengan enum/struct error.
5. Tambahkan unit test di `test/`.
6. Rapikan log duplikat pada `Application::init()`.
7. Tambahkan `operator new[] = delete` agar konsisten.
8. Tambahkan `.clang-format` bila formatting ingin diotomatiskan.

## Non-Goals (Sementara)

- Dukungan platform non-Windows.
- Multi-threaded renderer.
- Editor visual.
- Format asset kustom.

## Cara Memperbarui Roadmap

- Tandai item `[x]` hanya setelah implementasi masuk dan build berhasil.
- Pindahkan item yang berubah prioritas ke milestone yang sesuai.
- Catat utang teknis baru di bagian **Utang Teknis**.
- Perbarui [README](../README.md) bila status ringkas berubah.

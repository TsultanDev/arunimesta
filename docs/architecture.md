# Arsitektur

Dokumen ini menjelaskan struktur kode Arunimesta saat ini, keputusan desain utama, dan batas-batas modul.

## Ringkasan

Arunimesta adalah proyek C++23 yang menggunakan **named modules**. Kode dipecah per subsistem di bawah `modules/`, dan setiap modul memiliki target CMake sendiri.

Saat ini hanya dua modul yang ada:

```text
modules/
├── aru/            # executable + modul aru
└── aru_window/     # static library aru.window (masih kerangka)
```

Renderer Vulkan **belum ada** di repositori. Rencana arsitekturnya dibahas di [roadmap.md](roadmap.md).

## Peta Modul

```text
                ┌──────────────────────────┐
                │        main.cpp          │
                │  import aru;             │
                └────────────┬─────────────┘
                             │
                ┌────────────▼─────────────┐
                │      module aru          │
                │      (lib.cppm)          │
                │  export import           │
                │      :application        │
                └────────────┬─────────────┘
                             │
                ┌────────────▼─────────────┐
                │  module aru:application  │
                │  Application +           │
                │  Application_T           │
                └──────────────────────────┘

     (rencana) module aru.window  ←── belum dipakai oleh aru
```

### Modul `aru`

- `modules/aru/src/lib.cppm` adalah **facade**:
  ```cpp
  export module aru;
  export import :application;
  ```
- `modules/aru/src/application.cppm` adalah **partition** `aru:application`.
- `main.cpp` cukup melakukan `import aru;` dan tidak perlu mengetahui partition internal.

Pemisahan facade/partition berguna ketika modul bertambah: konsumen tetap mengimpor satu nama (`aru`), sementara implementasi dipecah menjadi beberapa partition.

### Modul `aru.window` (work in progress)

- Target static library `aru_window` sudah punya `CMakeLists.txt`.
- `src/window.cppm` baru berisi kerangka `export module aru.window:window;` dan deklarasi class yang belum lengkap.
- `src/lib.cppm` masih kosong.
- Modul ini **belum** ditambahkan ke `add_subdirectory()` di root `CMakeLists.txt`, jadi tidak ikut dibangun.

## Pola Desain `Application`

### Pimpl dengan `unique_ptr`

Detail implementasi `Application` dipisahkan ke struct `Application_T`:

```cpp
namespace aru {
    struct Application_T;
    export class Application;
}

struct aru::Application_T {
    std::string name = "";
    uint32_t version = 0;
};

class aru::Application {
    // ...
private:
    Application(std::unique_ptr<Application_T> app);
    std::unique_ptr<Application_T> app;
};
```

Konsekuensi:

- Layout publik `Application` stabil meskipun field internal berubah.
- `Application_T` tidak diekspos ke konsumen modul.
- Penghancuran `Application_T` terjadi otomatis lewat `unique_ptr` (RAII).

### Factory dan `std::expected`

Objek tidak dibuat dengan constructor publik, melainkan lewat factory:

```cpp
static std::expected<Application, bool> init(std::string name, uint32_t version);
```

- Tidak ada exception yang dilempar untuk kegagalan inisialisasi.
- Pemanggil wajib memeriksa hasil (`if (!app)` atau `.transform(...)`).
- Parameter kedua `bool` saat ini adalah placeholder tipe error. Belum ada nilai kegagalan yang dikembalikan; `init()` selalu sukses. Tipe ini dapat diganti dengan enum/struct error di masa depan.

### Move-only dan larangan alokasi heap

```cpp
Application(const Application&) = delete;
Application& operator=(const Application&);   // dideklarasikan, belum didefinisikan
Application(Application&&) noexcept = default;
Application& operator=(Application&&) noexcept = default;
void* operator new(size_t) = delete;
```

- Objek dapat dipindahkan, tetapi tidak disalin.
- `operator new` dihapus agar `Application` tidak dibuat dengan `new` biasa; pembuatan hanya lewat `Application::init()`.
- Catatan: copy assignment masih **dideklarasikan tanpa definisi**. Selama tidak dipakai, build tetap berhasil; ini perlu dibersihkan (dihapus eksplisit) pada pembersihan API berikutnya.

### Alur hidup

```text
main()
  └─ Application::init(name, version)
       ├─ make_unique<Application_T>
       ├─ (debug) println log
       └─ return std::expected<Application, bool>
            └─ printInfo()
                 └─ destructor Application → unique_ptr → ~Application_T
```

Belum ada `Application::run()` pada iterasi kode ini. `main.cpp` hanya memanggil `printInfo()` di dalam `transform`.

## Error Handling

- API publik menggunakan `std::expected`.
- Belum ada exception yang dilempar oleh kode engine.
- Tipe error saat ini `bool`, sehingga tidak ada informasi penyebab kegagalan.
- Rekomendasi ke depan: ganti dengan `enum class Error` atau struct error yang bisa memberi konteks.

## Logging

- Log memakai `std::println` dan hanya aktif pada build debug dengan `#ifndef NDEBUG`.
- Prefix yang dipakai saat ini: `Aru: Application(...)`.
- Target konvensi: `Aru: <Area>(<event>)`, misalnya `Aru: Window(created!!)`.
- Saat ini `init()` mencetak dua baris log yang isinya mirip; dapat dirapikan nanti.

## Build dan Batas Modul

- Root `CMakeLists.txt` hanya menambahkan `modules/aru`.
- Setiap modul mendaftarkan file `.cppm` melalui `FILE_SET CXX_MODULES`.
- Include header standar diletakkan di **global module fragment** (`module;` ... `export module ...;`) agar tidak bocor ke konsumen.
- `aru` tidak bergantung pada `aru.window` saat ini.

## Menambah Modul Baru

Langkah umum:

1. Buat direktori `modules/<nama>/src`.
2. Tambahkan file partition, misalnya `foo.cppm` dengan `export module aru:foo;`.
3. Tambahkan facade `lib.cppm` dengan `export module aru; export import :foo;` (atau modul terpisah bila bukan bagian dari `aru`).
4. Buat `CMakeLists.txt` modul dengan `add_library(...)` dan `target_sources(... FILE_SET CXX_MODULES ...)`.
5. Daftarkan modul di root `CMakeLists.txt` dengan `add_subdirectory(...)`.
6. Jika modul dipakai executable, tambahkan dependency target di `target_link_libraries`.

## Utang Teknis yang Diketahui

| Item | Dampak | Rencana |
| :--- | :--- | :--- |
| `aru.window` belum lengkap | Windowing belum bisa dipakai | Selesaikan class `Window`, isi facade, tambahkan ke root CMake |
| `aru_window` belum di-`add_subdirectory` | Modul tidak ikut build | Tambahkan setelah source lengkap |
| Copy assignment `Application` tanpa definisi | Berpotensi link error bila dipakai | Hapus eksplisit (`= delete`) |
| Tipe error `bool` | Tidak ada detail kegagalan | Ganti dengan enum/struct error |
| Belum ada test | Regresi tidak terdeteksi | Isi folder `test/` dengan unit test |
| Log duplikat di `init()` | Output debug berulang | Rapikan saat refactor |

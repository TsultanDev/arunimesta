<div align="center">

# Arunimesta Engine

<img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23">
<img src="https://img.shields.io/badge/CMake-3.28%2B-064F8C.svg" alt="CMake 3.28+">
<img src="https://img.shields.io/badge/Clang-LLVM-black.svg" alt="Clang">
<img src="https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg" alt="Windows x64">
<img src="https://img.shields.io/badge/Status-Experimental-orange.svg" alt="Experimental">

</div>

**Arunimesta** adalah game engine 3D yang dibangun dari nol dengan **C++23** dan *named modules*. Proyek ini menekankan arsitektur modular, error handling eksplisit, dan manajemen resource berbasis RAII.

> **Status: early development.** Fondasi build system dan modul aplikasi sudah berjalan. Modul windowing masih berupa kerangka, dan renderer Vulkan belum diimplementasikan.

---

## Status Proyek

| Bagian | Status |
| :--- | :--- |
| Build system (CMake + Clang + Ninja) | ✅ Berjalan |
| Modul `aru` (facade + `Application`) | ✅ Berjalan |
| Modul `aru.window` | 🚧 Kerangka / belum selesai |
| Renderer Vulkan (RHI) | ⬜ Belum dimulai |

Saat ini yang dapat dibangun dan dijalankan adalah executable `aru`, yang membuat objek `Application`, mencetak informasi aplikasi, lalu keluar.

---

## Fitur Saat Ini

- **C++23 Named Modules**: Modul utama `aru` dengan *module partition* `aru:application` dan facade `lib.cppm`.
- **Factory + `std::expected`**: `Application::init()` mengembalikan `std::expected<Application, bool>` tanpa melempar exception.
- **Pimpl dengan `std::unique_ptr`**: Detail implementasi disembunyikan di `Application_T`; objek bersifat *move-only*.
- **Heap allocation diblokir**: `operator new` dihapus agar objek hanya dibuat melalui factory dan tidak dialokasikan bebas di heap.
- **RAII**: `Application_T` dibersihkan otomatis oleh `std::unique_ptr`.
- **Debug logging**: Output `std::println` hanya aktif pada build debug (`#ifndef NDEBUG`).
- **CMake Presets**: Preset Clang LLVM x64 untuk Debug dan Release, dilengkapi `compile_commands.json` untuk clangd.
- **Struktur modular**: Setiap modul berada di `modules/<nama_modul>` dengan `CMakeLists.txt` sendiri.

---

## Tech Stack

| Komponen | Teknologi |
| :--- | :--- |
| **Bahasa** | C++23 (named modules) |
| **Compiler** | Clang (`clang++`) |
| **Build System** | CMake ≥ 3.28 + Ninja |
| **Fitur Standar** | `std::expected`, `std::print` / `std::println`, `std::unique_ptr` |
| **Platform Target** | Windows x64 (sesuai preset yang tersedia) |
| **Windowing** | `aru.window` — *work in progress* (target Win32) |
| **Rendering** | Vulkan — *planned* |

---

## Prasyarat

1. **Clang** dengan dukungan C++23 modules; `clang++` harus tersedia di `PATH`.
2. **CMake ≥ 3.28** — diperlukan untuk `FILE_SET ... TYPE CXX_MODULES`.
3. **Ninja** sebagai generator.
4. **Windows x64** — preset yang disediakan hanya aktif di Windows.

> Vulkan SDK belum diperlukan sampai modul RHI mulai diimplementasikan.

---

## Build

### 1. Clone Repository

```bash
git clone https://github.com/TsultanDev/arunimesta.git
cd arunimesta
```

### 2. Configure

Debug:

```bash
cmake --preset clang-x64-debug
```

Release:

```bash
cmake --preset clang-x64-release
```

### 3. Build

```bash
cmake --build --preset build-debug
```

atau:

```bash
cmake --build --preset build-release
```

### 4. Jalankan

```bash
./build/clang-x64-debug/modules/aru/aru.exe
```

Output yang diharapkan (build debug):

```text
Aru: Application(created!!)
Aru: Application(created!!), name: Hello, version: 1
Aru: Application(created!!), name: Hello, version: 1
```

### Build Manual

Jika tidak ingin memakai preset:

```bash
cmake -S . -B build/clang-x64-debug -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/clang-x64-debug
```

Detail dan troubleshooting ada di [`docs/building.md`](docs/building.md).

---

## Struktur Proyek

```text
arunimesta/
├── .clangd                          # Konfigurasi clangd (compile_commands.json)
├── .gitignore
├── CMakeLists.txt                   # Root build script
├── CMakePresets.json                # Preset Clang/Ninja (Debug & Release)
├── README.md
│
├── docs/                            # Dokumentasi proyek
│   ├── README.md                    # Indeks dokumentasi
│   ├── architecture.md              # Arsitektur & desain modul
│   ├── building.md                  # Panduan build & troubleshooting
│   ├── coding-style.md              # Konvensi kode
│   ├── roadmap.md                   # Rencana pengembangan
│   └── api/
│       └── application.md           # Referensi API Application
│
├── modules/                         # Semua target build
│   ├── aru/                         # Executable — entry point
│   │   ├── CMakeLists.txt
│   │   └── src/
│   │       ├── lib.cppm             # export module aru; (facade)
│   │       ├── application.cppm     # export module aru:application;
│   │       └── main.cpp             # int main()
│   │
│   └── aru_window/                  # Static library — windowing (WIP)
│       ├── CMakeLists.txt
│       └── src/
│           ├── lib.cppm             # facade (belum diisi)
│           └── window.cppm          # kerangka aru.window:window
│
├── assets/                          # Aset (saat ini kosong)
├── test/                            # Area pengujian (saat ini kosong)
└── build/                           # Output build (di-ignore)
```

### Deskripsi Modul

| Modul | Tipe | Status | Deskripsi |
| :--- | :--- | :--- | :--- |
| `aru` | Executable | ✅ | Entry point; memakai modul `aru` dan menjalankan `Application`. |
| `aru:application` | Module partition | ✅ | Implementasi `Application` beserta detail internal `Application_T`. |
| `aru.window` | Static library | 🚧 | Rencana abstraksi windowing; source masih berupa kerangka. |

> Catatan: `modules/aru_window` belum ditambahkan ke `add_subdirectory()` di root `CMakeLists.txt`, sehingga belum ikut dibangun.

---

## Contoh Penggunaan

Kode yang saat ini ada di `modules/aru/src/main.cpp`:

```cpp
#include <expected>
#include <utility>
import aru;

int main(int argc, char** argv) {
    auto result = aru::Application::init("Hello", 1)
        .transform([](aru::Application&& app) -> void {
            app.printInfo();
        });

    return 0;
}
```

Penanganan error yang lebih eksplisit:

```cpp
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        // init() gagal
        return 1;
    }

    app->printInfo();
    return 0;
}
```

Referensi lengkap: [`docs/api/application.md`](docs/api/application.md).

---

## Dokumentasi

| Dokumen | Isi |
| :--- | :--- |
| [docs/architecture.md](docs/architecture.md) | Struktur modul, pimpl, ownership, alur inisialisasi |
| [docs/building.md](docs/building.md) | Prasyarat, preset, build manual, troubleshooting |
| [docs/coding-style.md](docs/coding-style.md) | Penamaan, layout file `.cppm`, pola factory/pimpl |
| [docs/api/application.md](docs/api/application.md) | API `aru::Application` |
| [docs/roadmap.md](docs/roadmap.md) | Milestone dan rencana ke depan |

---

## Roadmap Singkat

- [x] Build system CMake + Clang + Ninja
- [x] Named modules C++23 dan module partition
- [x] `Application` dengan factory `std::expected` dan pimpl `unique_ptr`
- [ ] Menyelesaikan modul `aru.window` dan memasukkannya ke root build
- [ ] Vulkan RHI: instance, surface, adapter/device, swapchain
- [ ] Render pass, pipeline, command buffer, dan render loop
- [ ] Event handling dan resize
- [ ] Resource management dan ECS

Detail: [`docs/roadmap.md`](docs/roadmap.md).

---

## Catatan

- Proyek ini **dalam tahap pengembangan aktif**; API dan struktur dapat berubah tanpa kompatibilitas mundur.
- Preset build saat ini dikhususkan untuk **Windows x64 + Clang**.
- Modul `aru_window` masih belum lengkap dan belum ikut dibangun.
- Belum ada lisensi yang ditetapkan.

---

## Referensi

- [C++23 Modules — cppreference](https://en.cppreference.com/w/cpp/language/modules.html)
- [std::expected — cppreference](https://en.cppreference.com/w/cpp/utility/expected)
- [CMake: C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html)
- [Clang C++ Status](https://clang.llvm.org/cxx_status.html)
- [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)

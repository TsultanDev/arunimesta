# Panduan Build

Dokumen ini menjelaskan cara membangun Arunimesta dari source, baik memakai preset maupun manual.

## Prasyarat

| Kebutuhan | Versi / Catatan |
| :--- | :--- |
| Clang | Harus mendukung C++23 modules; `clang++` tersedia di `PATH` |
| CMake | ≥ 3.28 (dibutuhkan untuk `FILE_SET ... TYPE CXX_MODULES`) |
| Ninja | Generator yang dipakai preset |
| OS | Windows x64 — preset yang tersedia hanya aktif di Windows |

Anda dapat memeriksa tool yang terpasang:

```bash
clang++ --version
cmake --version
ninja --version
```

> Vulkan SDK belum dibutuhkan sampai modul RHI diimplementasikan.

## Preset yang Tersedia

| Configure Preset | Build Preset | Build Type | Direktori Output |
| :--- | :--- | :--- | :--- |
| `clang-x64-debug` | `build-debug` | Debug | `build/clang-x64-debug` |
| `clang-x64-release` | `build-release` | Release | `build/clang-x64-release` |

Preset menggunakan:

- Generator **Ninja**
- Compiler **clang++**
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` (untuk clangd)
- Kondisi `hostSystemName == Windows`

## Build dengan Preset

### Debug

```bash
cmake --preset clang-x64-debug
cmake --build --preset build-debug
```

### Release

```bash
cmake --preset clang-x64-release
cmake --build --preset build-release
```

## Menjalankan

Executable Debug:

```bash
./build/clang-x64-debug/modules/aru/aru.exe
```

Executable Release:

```bash
./build/clang-x64-release/modules/aru/aru.exe
```

## Build Manual

Jika preset tidak cocok dengan environment Anda:

```bash
cmake -S . -B build/manual -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build/manual
```

Hasilnya berada di `build/manual/modules/aru/aru.exe`.

## Build Ulang dari Nol

Jika ada error aneh akibat cache atau modul lama:

```bash
rm -rf build/clang-x64-debug      # Bash / Git Bash
```

```powershell
Remove-Item -Recurse -Force build\clang-x64-debug   # PowerShell
```

Lalu configure dan build ulang.

## Integrasi clangd

- `CMakePresets.json` mengekspor `compile_commands.json`.
- `.clangd` mengarahkan clangd ke `build/clang-x64-debug`.
- Jadi, lakukan configure preset Debug terlebih dahulu agar IDE/clangd mengenali modul dan include.

## Modul `aru_window`

Saat ini `modules/aru_window` **belum** ditambahkan ke root `CMakeLists.txt`, sehingga tidak ikut dibangun. Setelah source modul selesai, tambahkan:

```cmake
add_subdirectory("modules/aru_window")
```

Lalu hubungkan ke target yang membutuhkannya:

```cmake
target_link_libraries(aru PRIVATE aru_window)
```

## Troubleshooting

| Gejala | Kemungkinan Penyebab | Solusi |
| :--- | :--- | :--- |
| `clang++: command not found` | Clang belum terpasang / tidak di `PATH` | Install LLVM dan pastikan `clang++` bisa dipanggil |
| `CMake ... does not support CXX_MODULES` | CMake terlalu lama | Gunakan CMake ≥ 3.28 |
| Preset tidak muncul / error kondisi | Preset hanya aktif di Windows | Gunakan Windows, atau buat preset/manual configure sendiri |
| `ninja: command not found` | Ninja belum terpasang | Install Ninja atau gunakan generator lain |
| Error saat compile `.cppm` | Compiler tidak mendukung modules | Gunakan Clang versi yang mendukung C++23 modules |
| Perubahan header tidak terdeteksi | Cache build/modul lama | Hapus direktori build lalu configure ulang |
| `import aru;` gagal | File `.cppm` tidak terdaftar di `FILE_SET CXX_MODULES` atau target tidak di-link | Periksa `target_sources` dan `target_link_libraries` |

## Catatan

- Semua file modul harus terdaftar di `target_sources(... FILE_SET CXX_MODULES ...)` agar ikut dikompilasi.
- Include library eksternal diletakkan di global module fragment, bukan setelah `export module`.
- Build Debug mengaktifkan log `std::println`; build Release tidak.

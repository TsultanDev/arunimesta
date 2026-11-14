# Konvensi Kode

Dokumen ini merangkum konvensi yang dipakai (dan dituju) oleh kode Arunimesta. Belum ada file `.clang-format` di repositori, jadi konsistensi dijaga secara manual.

## Penamaan

| Elemen | Konvensi | Contoh |
| :--- | :--- | :--- |
| Class / struct publik | `PascalCase` | `Application`, `Window` |
| Struct implementasi internal | `PascalCase` + `_T` | `Application_T` |
| Fungsi / method | `camelCase` | `init`, `printInfo` |
| Variabel / parameter / member | `camelCase` | `app`, `name`, `version` |
| Enum type | `PascalCase` | `Flags` |
| Enum value | `PascalCase` | `None`, `Resizable` |
| Namespace | `lowercase` | `aru`, `aru::window` |
| Module | `lowercase.titik` | `aru`, `aru.window`, `aru:application` |
| Module file | `lowercase.cppm` | `application.cppm` |
| Macro | `UPPER_SNAKE_CASE` | *(belum ada di engine)* |

Catatan:

- Member diakses eksplisit dengan `this->` pada implementasi class.
- Parameter yang menyimpan nilai ke member boleh memakai nama yang sama, misalnya `version(version)`.

## Ekstensi dan Lokasi File

- Modul C++ memakai ekstensi `.cppm`.
- Entry point memakai `.cpp`.
- Setiap modul berada di `modules/<nama_modul>/src/`.
- Header/source non-module (bila nanti ada) diletakkan di `modules/<nama_modul>/include/`.

## Layout File `.cppm`

```cpp
module;                              // global module fragment

#include <cstdint>                   // include standar / library eksternal
#include <memory>

export module aru:widget;            // deklarasi modul / partition

namespace aru {
    struct Widget_T;                 // forward declaration impl
    export class Widget;             // deklarasi class publik
}

struct aru::Widget_T {               // definisi impl (tidak diekspor)
    // ...
};

class aru::Widget {                  // definisi class publik
    // ...
};
```

Aturan:

1. Include **wajib** berada di global module fragment (`module;` sampai `export module ...;`).
2. Jangan menaruh `#include` setelah `export module`, karena akan ikut terekspos ke konsumen.
3. Definisikan struct `_T` sebelum member `unique_ptr<_T>` dihancurkan; urutan ini membuat compiler melihat tipe lengkap.
4. Deklarasikan semua yang perlu dipakai konsumen dengan `export`.

## Facade dan Partition

Untuk modul yang dipecah:

```cpp
// application.cppm
export module aru:application;
```

```cpp
// lib.cppm
export module aru;
export import :application;
```

- Konsumen cukup `import aru;`.
- Partisi internal tidak diimpor langsung oleh kode luar.
- Nama partition mengikuti nama file tanpa ekstensi.

## Pola Factory + Pimpl

Gunakan pola berikut untuk class yang memiliki resource/internal state:

```cpp
namespace aru {
    struct Thing_T;
    export class Thing;
}

struct aru::Thing_T {
    // state internal
};

class aru::Thing {
    public:
        static std::expected<Thing, bool> init(/* parameter */);
        // method publik lain

        Thing(const Thing&) = delete;
        Thing& operator=(const Thing&) = delete;
        Thing(Thing&&) noexcept = default;
        Thing& operator=(Thing&&) noexcept = default;

        void* operator new(size_t) = delete;
        void* operator new[](size_t) = delete;

    private:
        Thing(std::unique_ptr<Thing_T> impl);
        std::unique_ptr<Thing_T> impl;
};
```

Ketentuan:

- Constructor privat; pembuatan lewat factory statis.
- Factory mengembalikan `std::expected<T, Error>`.
- Copy dihapus; move di-default.
- `operator new` dihapus agar tidak bisa dialokasikan di heap secara bebas.
- Implementasi internal tidak diekspor.

> Pada kode saat ini `Application` hanya menghapus `operator new` skalar. Untuk konsistensi, tambahkan juga `operator new[]` saat membersihkan API.

## Error Handling

- Gunakan `std::expected` untuk kegagalan yang dapat dipulihkan.
- Hindari `throw` pada batas API publik.
- Berikan tipe error yang bermakna; jangan gunakan `bool` untuk jangka panjang.
- Jangan mengabaikan hasil `expected`; periksa atau teruskan dengan `transform` / `and_then`.

Contoh:

```cpp
auto app = aru::Application::init("Arunimesta", 1);
if (!app) {
    // tangani kegagalan
    return 1;
}
app->printInfo();
```

## Logging

- Gunakan `std::println` / `std::print`.
- Bungkus dengan `#ifndef NDEBUG` agar hanya aktif di build debug.
- Format prefix: `Aru: <Area>(<event>)`.

Contoh:

```cpp
#ifndef NDEBUG
std::println(stdout, "Aru: Window(created!!), title: {}", title);
#endif
```

## Formatting

Belum ada formatter otomatis. Ikuti gaya yang sudah ada:

- Indentasi 4 spasi.
- Kurung kurawal gaya Allman pada deklarasi class/function utama (baris baru).
- Satu statement per baris.
- Hindari baris terlalu panjang; pecah parameter atau chained call.
- Gunakan `std::uint32_t`/`uint32_t` secara konsisten sesuai file (saat ini `uint32_t` dengan `<cstdint>`).

## Checklist Modul Baru

- [ ] File `.cppm` dengan `export module` yang jelas.
- [ ] Include di global module fragment.
- [ ] Tipe internal dinamai `<Type>_T` dan tidak diekspor.
- [ ] Factory mengembalikan `std::expected`.
- [ ] Copy dihapus, move di-default.
- [ ] `operator new` dihapus bila objek hanya boleh lewat factory.
- [ ] Target CMake dengan `FILE_SET CXX_MODULES`.
- [ ] Ditambahkan ke root `CMakeLists.txt`.
- [ ] Facade `export import` diperbarui.
- [ ] Dokumentasi di `docs/` diperbarui.

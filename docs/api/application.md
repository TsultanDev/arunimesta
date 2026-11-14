# API: `aru::Application`

Referensi untuk class `Application` yang diekspor oleh modul `aru`.

```cpp
import aru;

aru::Application
```

## Ringkasan

`Application` merepresentasikan konfigurasi dasar aplikasi: nama dan versi. Objek dibuat melalui factory `init()` dan bersifat *move-only*.

Detail implementasi disimpan pada `Application_T` (tidak diekspor) dan dimiliki melalui `std::unique_ptr`.

## API

### `static std::expected<Application, bool> init(std::string name, uint32_t version)`

Membuat instance `Application`.

| Parameter | Tipe | Keterangan |
| :--- | :--- | :--- |
| `name` | `std::string` | Nama aplikasi. Disimpan dengan move. |
| `version` | `uint32_t` | Nilai versi apa adanya; belum ada encoding khusus. |

Mengembalikan:

- `std::expected` yang berisi `Application` bila berhasil.
- Saat ini implementasi **selalu mengembalikan sukses**; belum ada jalur kegagalan.
- Tipe error `bool` adalah placeholder dan belum memberi informasi penyebab.

Efek samping (hanya build debug):

- Mencetak log pembuatan aplikasi ke `stdout`.

Contoh:

```cpp
auto app = aru::Application::init("Arunimesta", 1);
if (!app) {
    return 1;
}
```

### `void printInfo() const`

Mencetak nama dan versi aplikasi ke `stdout`.

Tidak memiliki guard debug; pemanggilan tetap menghasilkan output pada build Release.

Contoh output:

```text
Aru: Application(created!!), name: Hello, version: 1
```

## Lifetime dan Ownership

| Operasi | Status | Catatan |
| :--- | :--- | :--- |
| Constructor | Privat | Gunakan `init()` |
| Copy constructor | `= delete` | Tidak dapat disalin |
| Copy assignment | Dideklarasikan, belum didefinisikan | Jangan dipakai; akan diperbaiki menjadi `= delete` |
| Move constructor | `noexcept = default` | Dapat dipindahkan |
| Move assignment | `noexcept = default` | Dapat dipindahkan |
| `operator new` | `= delete` | Tidak bisa `new Application(...)` |
| Destructor | Implisit | `Application_T` dihancurkan via `unique_ptr` |

Urutan penghancuran:

```text
~Application()
  └─ ~unique_ptr<Application_T>()
       └─ ~Application_T()
```

## Contoh Penggunaan

### Gaya `transform` (seperti `main.cpp`)

```cpp
#include <expected>
#include <utility>
import aru;

int main() {
    auto result = aru::Application::init("Hello", 1)
        .transform([](aru::Application&& app) -> void {
            app.printInfo();
        });

    return 0;
}
```

Pada gaya ini, kegagalan tidak ditangani secara eksplisit. Cocok untuk contoh, tetapi tidak disarankan untuk kode produksi.

### Gaya pemeriksaan eksplisit

```cpp
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        return 1;
    }

    app->printInfo();
    return 0;
}
```

### Memindahkan objek

```cpp
#include <utility>
import aru;

int main() {
    auto app = aru::Application::init("Arunimesta", 1);
    if (!app) {
        return 1;
    }

    aru::Application moved = std::move(*app);
    moved.printInfo();
    return 0;
}
```

## Keterbatasan Saat Ini

- Belum ada `Application::run()` atau callback bootstrap.
- `version` masih berupa angka mentah; belum ada helper encoding `major.minor.patch`.
- Tipe error `bool` belum deskriptif.
- Copy assignment belum didefinisikan.
- `printInfo()` tidak dibungkus `#ifndef NDEBUG`, sehingga selalu mencetak.
- Tidak ada jaminan thread-safety.

## Kompatibilitas

- Memerlukan C++23 (`std::expected`, `std::print`, named modules).
- Diimpor melalui `import aru;`.
- Belum ada jaminan stabilitas API; lihat [roadmap](../roadmap.md).

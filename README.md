# Mini-Notepad

---

## English Version

A lightweight console-based text editor written in C, featuring essential text editing capabilities with undo/redo functionality.

### Features

- **Text Editing**: Full support for inserting and deleting characters
- **Word Navigation**: Move through text word by word
- **Copy/Cut/Paste**: Clipboard operations for text manipulation
- **Undo/Redo**: Unlimited undo functionality with stack-based state management
- **Text Selection**: Select and manipulate text ranges
- **Find**: Search for specific words or patterns in the text
- **File Operations**: Save and load text files
- **Windows Console**: Optimized for Windows console display with cursor positioning

### Requirements

- C compiler (GCC or MSVC)
- Windows operating system (uses Windows-specific features)
- CMake 4.0 or higher

### Building

#### Using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

#### Using GCC directly

```bash
gcc -o prolab1 main.c
```

### Usage

Run the executable:

```bash
./prolab1
```

### Keyboard Shortcuts

- **Arrow Keys**: Navigate through the text
- **Ctrl+C**: Copy selected text
- **Ctrl+X**: Cut selected text
- **Ctrl+V**: Paste from clipboard
- **Ctrl+Z**: Undo last action
- **Ctrl+F**: Find text
- **Ctrl+S**: Save file
- **Delete/Backspace**: Delete characters
- **Ctrl+Left/Right**: Move by word

### Project Structure

- `main.c` - Main editor implementation
- `CMakeLists.txt` - CMake build configuration
- `test/` - Test files directory

### Technical Details

The editor uses:
- **Doubly-linked list** for efficient text storage and navigation
- **Stack** for undo functionality (up to 20 states)
- **Windows Console API** for cursor positioning and control

### Data Structures

- `Node`: Individual character in the linked list
- `Editor`: Main editor state containing head, tail, cursor, and other properties

### Limitations

- Maximum text size: 8192 characters
- Maximum undo states: 20
- Windows-only (uses Windows.h)

### License

This project is provided as-is for educational purposes.

### Author

Created as part of a programming laboratory project.

---

## Türkçe Versiyonu

C dilinde yazılmış, temel metin düzenleme özelliklerine ve geri alma / yineleme işlevlerine sahip hafif bir konsol tabanlı metin editörü.

## Özellikler

- **Metin Düzenleme**: Karakterleri ekleme ve silme desteği
- **Kelime Gezintisi**: Metinde kelime kelime gezinme
- **Kopyala/Kes/Yapıştır**: Metin manipülasyonu için pano işlemleri
- **Geri Al/Yinele**: Yığın tabanlı durum yönetimiyle sınırsız geri alma işlevi
- **Metin Seçimi**: Metin aralıklarını seçme ve düzenleme
- **Ara**: Metinde belirli sözcükleri veya desenleri arama
- **Dosya İşlemleri**: Metin dosyalarını kaydetme ve açma
- **Windows Konsolu**: İmleç konumlandırması ile Windows konsolu görüntüsü için optimize edilmiş

## Gereksinimler

- C derleyicisi (GCC veya MSVC)
- Windows işletim sistemi (Windows'a özgü özellikler kullanır)
- CMake 4.0 veya daha yüksek

## Derleme

### CMake Kullanarak

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### GCC Kullanarak Doğrudan

```bash
gcc -o prolab1 main.c
```

## Kullanım

Yürütülebilir dosyayı çalıştırın:

```bash
./prolab1
```

### Klavye Kısayolları

- **Ok Tuşları**: Metinde gezinme
- **Ctrl+C**: Seçili metni kopyala
- **Ctrl+X**: Seçili metni kes
- **Ctrl+V**: Panodan yapıştır
- **Ctrl+Z**: Son işlemi geri al
- **Ctrl+F**: Metin ara
- **Ctrl+S**: Dosyayı kaydet
- **Delete/Backspace**: Karakterleri sil
- **Ctrl+Sol/Sağ Ok**: Kelime kelime gezin

## Proje Yapısı

- `main.c` - Ana editör uygulaması
- `CMakeLists.txt` - CMake derleme yapılandırması
- `test/` - Test dosyaları dizini

## Teknik Detaylar

Editör aşağıdakileri kullanır:
- **Çift yönlü bağlı liste** - Verimli metin depolaması ve gezintisi
- **Yığın (Stack)** - Geri alma işlevi için (en fazla 20 durum)
- **Windows Konsol API** - İmleç konumlandırması ve kontrolü

## Veri Yapıları

- `Node`: Bağlı listedeki bireysel karakter
- `Editor`: İmleç, baş, kuyruk ve diğer özellikleri içeren ana editör durumu

## Sınırlamalar

- Maksimum metin boyutu: 8192 karakter
- Maksimum geri alma durumları: 20
- Yalnızca Windows (Windows.h kullanır)

## Lisans

Bu proje eğitim amaçlı olarak olduğu gibi sağlanmaktadır.

## Yazarı

Programlama laboratuvarı proje kapsamında oluşturulmuştur.

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h> // _getch() için gerekli
#include <string.h> // strcpy fonksiyonu için gerekli
#include <direct.h>   // _getcwd ve _chdir fonksiyonları için
#include <sys/stat.h> // S_ISDIR makrosu ve stat fonksiyonu için

int isUndoing = 0; // Geri alma işlemi yapılıp yapılmadığını takip eden bayrak

// Her bir karakteri tutacak olan Çift Yönlü Bağlı Liste Düğümü
typedef struct Node {
    char data;
    struct Node* prev;
    struct Node* next;
} Node;

// Undo (Geri Al) işlemi için Stack (Yığın) Yapısı
// Yapılan işlemi ve o anki durumu tutmalı
typedef struct UndoAction {
    int actionType; // 1: Karakter Ekleme, 2: Karakter Silme vb.
    char character;
    // İmleç konumu gibi ekstra bilgiler eklenebilir
    struct UndoAction* next;
} UndoAction;

// Metin editörünün genel durumunu tutacak ana yapı
typedef struct Editor {
    Node* head;               // Metnin başlangıcı
    Node* tail;               // Metnin sonu
    Node* cursor;             // İmlecin şu anki konumu
    UndoAction* undoStackTop; // Undo yığınının tepesi
    char filename[256];       // dosyanin adi ve yolu
    char searchWord[100];     // aranan kelimeyi tutacak
    Node* selectStart;        // secili metnin baslangic dugumu
    Node* selectEnd;          // secili metinin bitis dugumu
    char clipboard[1024];     // kopyalanan metini tutacak gecici pano
} Editor;


// Yapılan işlemi Undo Yığınına (Stack) ekler
// type 1: Karakter Ekleme, type 2: Karakter Silme
void pushUndo(Editor* editor, int type, char c) {
    if (isUndoing) return; // Eğer şu an CTRL+Z çalışıyorsa, bu işlemi yığına kaydetme!

    UndoAction* newAction = (UndoAction*)malloc(sizeof(UndoAction));
    newAction->actionType = type;
    newAction->character = c;

    // Yığının (Stack) en üstüne ekle (Push işlemi)
    newAction->next = editor->undoStackTop;
    editor->undoStackTop = newAction;
}

// İmlecin solundaki karakteri silme (Normal Backspace)
void deleteChar(Editor* editor) {
    // İmleç en baştaysa silinecek bir şey yoktur
    if (editor->cursor == NULL) return;

    Node* toDelete = editor->cursor;

    // Silinecek düğümün solundaki bağları güncelle
    if (toDelete->prev != NULL) {
        toDelete->prev->next = toDelete->next;
    } else {
        // Eğer silinen düğüm 'head' ise, head'i bir sağa kaydır
        editor->head = toDelete->next;
    }

    // Silinecek düğümün sağındaki bağları güncelle
    if (toDelete->next != NULL) {
        toDelete->next->prev = toDelete->prev;
    } else {
        // Eğer silinen düğüm 'tail' ise, tail'i bir sola kaydır
        editor->tail = toDelete->prev;
    }

    // İmleci silinen harfin solundakine geçir
    editor->cursor = toDelete->prev;

    free(toDelete); // Hafızayı serbest bırak (Memory leak önlemek için)

    // Karakter henüz silinmeden, ne silineceğini yığına haber ver (2 = Silme yapıldı)
    pushUndo(editor, 2, editor->cursor->data);
}

// Yeni bir düğüm (harf) oluşturmak için yardımcı fonksiyon
Node* createNode(char c) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Hafiza tahsis hatasi!\n");
        return NULL; // Ram dolduysa veya hata varsa
    }
    newNode->data = c;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

// İmlecin (cursor) bulunduğu konuma karakter ekleme
void insertChar(Editor* editor, char c) {
    Node* newNode = createNode(c);
    if (newNode == NULL) return;

    // Durum 1: Dosya tamamen boşsa
    if (editor->head == NULL) {
        editor->head = newNode;
        editor->tail = newNode;
        editor->cursor = newNode;
    }
    // Durum 2: İmleç en baştaysa (Karakterler var ama imleç ilk harfin solunda)
    else if (editor->cursor == NULL) {
        newNode->next = editor->head;
        editor->head->prev = newNode;
        editor->head = newNode;
        editor->cursor = newNode; // İmleç yeni eklenen harfe geçer
    }
    // Durum 3: İmleç ortada veya sondayken ekleme
    else {
        newNode->next = editor->cursor->next;
        newNode->prev = editor->cursor;

        // Eğer imleç cümlenin ortasındaysa, sağdaki elemanın 'prev' değerini güncelle
        if (editor->cursor->next != NULL) {
            editor->cursor->next->prev = newNode;
        } else {
            // Eğer imleç sondayken ekliyorsak, 'tail' artık yeni düğüm olur
            editor->tail = newNode;
        }

        editor->cursor->next = newNode;
        editor->cursor = newNode; // İmleci yazdığımız harfin üzerine kaydır
    }

    // İşlem başarıyla bittikten sonra yığına haber ver (1 = Ekleme yapıldı)
    pushUndo(editor, 1, c);
}

// Geri Al (CTRL + Z) İşlemi
void undo(Editor* editor) {
    if (editor->undoStackTop == NULL) return; // Yığın boşsa, geri alınacak bir şey yoktur

    // Yığının en üstündeki işlemi al
    UndoAction* action = editor->undoStackTop;
    editor->undoStackTop = action->next; // Yığının tepesini bir altındakine kaydır (Pop)

    isUndoing = 1; // Döngüye girmemek için bayrağı kaldır

    if (action->actionType == 1) {
        // Son işlem EKLEME ise, geri almak için o harfi SİLMELİYİZ
        deleteChar(editor);
    }
    else if (action->actionType == 2) {
        // Son işlem SİLME ise, geri almak için o harfi tekrar EKLEMELİYİZ
        insertChar(editor, action->character);
    }

    isUndoing = 0; // İşlem bitti, bayrağı indir
    free(action); // Yığından çıkardığımız düğümü RAM'den sil
}


// Kelime Bazlı Silme (CTRL + Backspace)
void deleteWord(Editor* editor) {
    if (editor->cursor == NULL) return; // İmleç en baştaysa silinecek bir şey yoktur

    // 1. Önce imlecin hemen solunda boşluklar varsa onları sil
    while (editor->cursor != NULL && (editor->cursor->data == ' ' || editor->cursor->data == '\n')) {
        deleteChar(editor);
    }

    // 2. Sonra kelimenin harflerini, yeni bir boşluk veya satır başı görene kadar sil
    while (editor->cursor != NULL && editor->cursor->data != ' ' && editor->cursor->data != '\n') {
        deleteChar(editor);
    }
}

// Başlangıç düğümünden itibaren kelime eşleşiyor mu kontrol eden algoritma
int isMatch(Node* startNode, const char* word) {
    Node* temp = startNode;
    int i = 0;

    // Kelimenin sonuna ('\0') gelene kadar harfleri karşılaştır
    while (word[i] != '\0') {
        // Eğer liste bittiyse veya harfler uyuşmuyorsa eşleşme başarısızdır
        if (temp == NULL || temp->data != word[i]) {
            return 0; // Yanlış (0) döndür
        }
        temp = temp->next;
        i++;
    }
    return 1; // Tüm harfler uyduysa Doğru (1) döndür
}

// İmleci bir kelime sola kaydır (Boşluk görene kadar)
void moveWordLeft(Editor* editor) {
    while (editor->cursor != NULL && editor->cursor->data != ' ' && editor->cursor->data != '\n') {
        editor->cursor = editor->cursor->prev;
    }
    // Boşluğu da atla
    if (editor->cursor != NULL) editor->cursor = editor->cursor->prev;
}

// İmleci bir kelime sağa kaydır
void moveWordRight(Editor* editor) {
    // Önce şu anki boşluktaysak onu atla
    if (editor->cursor != NULL && (editor->cursor->data == ' ' || editor->cursor->data == '\n')) {
        editor->cursor = editor->cursor->next;
    }
    // Sonraki boşluğa kadar git
    while (editor->cursor != NULL && editor->cursor->next != NULL &&
           editor->cursor->next->data != ' ' && editor->cursor->next->data != '\n') {
        editor->cursor = editor->cursor->next;
           }
}

void printText(Editor* editor) {
    // 1. Ekranı Temizle
    system("cls");

    // 2. Toolbox (Şerit) Tasarımı
    printf(" Dosya Ac(CTRL+O) | Kaydet(CTRL+S) | Farkli Kaydet(CTRL+SHIFT+S) | Bul(CTRL+F) | Degistir(CTRL+H) \n");
    printf(" Kopyala(CTRL+C) | Eslesmeleri Bul(CTRL+G) | Kes(CTRL+X) | Yapistir(CTRL+V) | Geri Al(CTRL+Z) | Cikis(ESC) \n");
    printf("-------------------------------------------------------------------------------------------------------------\n");

    // 3. Metin ve Satır Numaralarını Yazdırma
    int lineNumber = 1;

    //satir basi numaralari
    printf("%3d | ", lineNumber);

    Node* current = editor->head;

    int inSelection = 0; // Seçim aralığında mıyız kontrolü

    while (current != NULL) {
        // Eğer arama (CTRL+F) kelimesi varsa mavi yapma kodlarımız burada durmaya devam edecek...
        // (Daha önce yazdığımız mavi arka plan kodlarını buraya dahil ettiğini varsayıyorum)

        // SEÇİM (SARI ARKA PLAN) KONTROLÜ
        int isCurrentNodeSelected = 0;

        // Eğer bir seçim varsa ve sınırlar belliyse
        if (editor->selectStart != NULL && editor->selectEnd != NULL) {
            // Başlangıç veya bitiş düğümüne geldiysek durumu değiştir
            if (current == editor->selectStart || current == editor->selectEnd) {
                // Eğer sadece 1 karakter seçiliyse
                if (editor->selectStart == editor->selectEnd) {
                    isCurrentNodeSelected = 1;
                } else {
                    inSelection = !inSelection; // Seçim alanına girdik veya çıkıyoruz
                    isCurrentNodeSelected = 1;  // Sınır düğümleri de sarı olmalı
                }
            } else if (inSelection) {
                isCurrentNodeSelected = 1; // Aradaki düğümler de sarı olmalı
            }
        }

        // Rengi Ayarla
        if (isCurrentNodeSelected) {
            // Arka planı SARI, yazıyı SİYAH yap
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_RED | BACKGROUND_GREEN | FOREGROUND_INTENSITY);
        } else {
            // Normal Renk (Siyah arka plan, Beyaz yazı)
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }

        putchar(current->data); // Karakteri ekrana bas

        // Renkleri sıfırla (eğer sarıysa diğer karakterleri etkilemesin)
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        if (current->data == '\n') {
            lineNumber++;
            printf("%3d | ", lineNumber);
        }

        // Eğer seçim alanından çıkıyorsak (sınır düğümünü az önce bastık) ve tek karakter değilse inSelection'ı kapat
        if ((current == editor->selectStart || current == editor->selectEnd) && editor->selectStart != editor->selectEnd && isCurrentNodeSelected) {
           // Zaten inSelection toggle ile değiştiği için ek bir şeye gerek yok.
        }

        current = current->next;
    }
}

// 1. Ekrandaki fiziksel imleci (X, Y) koordinatına taşıyan fonksiyon
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 2. İmlecin görünümünü ayarlayan fonksiyon
void setCursorAppearance() {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100; // İmlecin boyutunu büyütür (Terminal ayarlarına göre dikey çizgi veya blok yapar)
    info.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

// 3. Bağlı listedeki konuma göre X ve Y'yi hesaplayıp imleci oraya taşıyan ana fonksiyon
void calculateAndMoveCursor(Editor* editor) {
    // X Offset (X Başlangıcı): 6
    // Neden 6? Çünkü printText fonksiyonunda satır numaralarını "%3d | " formatında bastık.
    // 3 hane rakam + 1 boşluk + 1 dikey çizgi + 1 boşluk = Toplam 6 karakter yer kaplıyor.
    int x = 6;

    // Y Offset (Y Başlangıcı): 2
    // Neden 2? Çünkü Toolbox (menü) 0. satırda, altındaki tire (----) çizgisi 1. satırda.
    // Yazı alanı 2. satırda (index olarak) başlıyor.
    int y = 3;

    Node* temp = editor->head;

    // Eğer imleç NULL ise (yani dosya tamamen boşsa veya imleç en baştaysa)
    // Direkt (6, 2) koordinatına git ve çık.
    if (editor->cursor == NULL) {
        gotoxy(x, y);
        return;
    }

    // Listeyi head'den başlayıp 'cursor' işaretçisinin olduğu yere kadar gez
    while (temp != NULL) {
        // Eğer alt satıra geçilmişse
        if (temp->data == '\n') {
            x = 6; // X'i satır başı hizasına geri çek
            y++;   // Y koordinatını 1 satır aşağı kaydır
        } else {
            x++;   // Normal bir karakterse X'i 1 birim sağa kaydır
        }

        // Eğer aradığımız cursor düğümüne ulaştıysak döngüyü kır
        if (temp == editor->cursor) {
            break;
        }
        temp = temp->next;
    }

    // Hesaplanmış olan (X, Y) konumuna konsol imlecini taşı
    gotoxy(x, y);
}

// İmleci bir karakter sola kaydır
void moveCursorLeft(Editor* editor) {
    // Eğer imleç zaten en baştaysa (NULL) daha fazla sola gidemeyiz
    if (editor->cursor != NULL) {
        editor->cursor = editor->cursor->prev;
    }
}

// İmleci bir karakter sağa kaydır
void moveCursorRight(Editor* editor) {
    // Durum 1: İmleç en baştaysa (NULL) ve listede eleman varsa, ilk elemana git
    if (editor->cursor == NULL && editor->head != NULL) {
        editor->cursor = editor->head;
    }
    // Durum 2: İmleç zaten bir harfin üzerindeyse ve sağında harf varsa sağa kay
    else if (editor->cursor != NULL && editor->cursor->next != NULL) {
        editor->cursor = editor->cursor->next;
    }
}

// İmlecin mevcut satırdaki sütun (X) pozisyonunu hesaplar (Yardımcı Fonksiyon)
int getCurrentColumn(Editor* editor) {
    int col = 0;
    Node* temp = editor->cursor;

    // Geriye doğru giderek satır başına (\n veya NULL) kadar say
    while (temp != NULL && temp->data != '\n') {
        col++;
        temp = temp->prev;
    }
    return col;
}

// YUKARI OK TUŞU MANTIĞI (DÜZELTİLMİŞ)
void moveCursorUp(Editor* editor) {
    if (editor->cursor == NULL) return; // Zaten 1. satırdayız, yukarı çıkamayız

    int targetCol = getCurrentColumn(editor);

    // 1. Bulunduğumuz satırın başındaki \n karakterini bul (Bu, bir üst satırın sonudur)
    Node* currentLinePrevNewline = editor->cursor;
    while (currentLinePrevNewline != NULL && currentLinePrevNewline->data != '\n') {
        currentLinePrevNewline = currentLinePrevNewline->prev;
    }

    if (currentLinePrevNewline == NULL) {
        // Eğer \n yoksa 1. satırdayız demektir, en başa git
        editor->cursor = NULL;
        return;
    }

    // 2. Bir üst satırın başındaki \n karakterini bul (Bu, hedef satırımızın başlangıç noktasıdır)
    Node* targetLinePrevNewline = currentLinePrevNewline->prev;
    while (targetLinePrevNewline != NULL && targetLinePrevNewline->data != '\n') {
        targetLinePrevNewline = targetLinePrevNewline->prev;
    }

    // İmleci bir üst satırın en başına (0. sütuna) yerleştir
    editor->cursor = targetLinePrevNewline;

    // 3. Hedef sütuna (targetCol) kadar sağa doğru ilerle
    int col = 0;
    Node* nextNode = (editor->cursor == NULL) ? editor->head : editor->cursor->next;

    // Eğer alt satırda daha fazla sağdaysak ama üst satır kısaysa veya boşsa, \n görene kadar ilerle
    while (col < targetCol && nextNode != NULL && nextNode->data != '\n') {
        editor->cursor = nextNode;
        nextNode = nextNode->next;
        col++;
    }
}

// AŞAĞI OK TUŞU MANTIĞI (DÜZELTİLMİŞ)
void moveCursorDown(Editor* editor) {
    int targetCol = getCurrentColumn(editor);

    // 1. Bulunduğumuz satırın sonundaki \n karakterini bul
    Node* currentLineEnd = (editor->cursor == NULL) ? editor->head : editor->cursor->next;
    while (currentLineEnd != NULL && currentLineEnd->data != '\n') {
        currentLineEnd = currentLineEnd->next;
    }

    if (currentLineEnd == NULL) return; // Eğer \n yoksa son satırdayız, aşağı inemeyiz

    // İmleci bir alt satırın en başına (0. sütuna) yerleştir
    editor->cursor = currentLineEnd;

    // 2. Hedef sütuna (targetCol) kadar sağa doğru ilerle
    int col = 0;
    Node* nextNode = editor->cursor->next;

    while (col < targetCol && nextNode != NULL && nextNode->data != '\n') {
        editor->cursor = nextNode;
        nextNode = nextNode->next;
        col++;
    }
}

// Dosya Yöneticisi Fonksiyonu
// resultPath: Seçilen dosyanın adının kopyalanacağı dizi
// mode: 0 ise "Dosya Aç", 1 ise "Dosya Kaydet" başlığı gösterir
void openFileManager(char* resultPath, int mode) {
    char currentPath[512];
    char input[256];
    struct stat fileStat;
    WIN32_FIND_DATAA findFileData;
    HANDLE findHandle;

    int fmRunning = 1;
    while(fmRunning) {
        system("cls"); // Ekranı temizle
        _getcwd(currentPath, sizeof(currentPath)); // Bulunulan dizinin yolunu al

        // Başlık kısmı
        if (mode == 0) printf("=== DOSYA AC (Secim Ekrani) ===\n");
        else printf("=== DOSYA KAYDET (Secim Ekrani) ===\n");

        printf("Mevcut Dizin: %s\n", currentPath);
        printf("--------------------------------------------------\n");

        // Klasörleri ve dosyaları listeleme kısmı
        findHandle = FindFirstFileA("*", &findFileData);
        if (findHandle != INVALID_HANDLE_VALUE) {
            do {
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    printf(" [KLASOR] %s\n", findFileData.cFileName);
                } else {
                    printf(" [DOSYA]  %s\n", findFileData.cFileName);
                }
            } while (FindNextFileA(findHandle, &findFileData));
            FindClose(findHandle);
        } else {
            printf("Dizin okunamadi!\n");
        }

        // Kullanıcı Yönlendirmesi
        printf("--------------------------------------------------\n");
        printf(" -> Ileri gitmek icin bir [KLASOR] adi yazin.\n");
        printf(" -> Geri (ust dizine) donmek icin '..' yazin.\n");
        printf(" -> Ismlemi tamamlamak icin bir [DOSYA] adi yazin.\n");
        printf(" -> Iptal etmek icin 'iptal' yazin.\n");
        printf("\nSeciminiz veya Dosya Adi: ");
        scanf("%255s", input);

        if (strcmp(input, "iptal") == 0) {
            resultPath[0] = '\0'; // İptal edildiyse boş string döndür
            return;
        }

        // Girilen ismin bir klasör mü yoksa dosya mı olduğunu kontrol et
        stat(input, &fileStat);
        if (S_ISDIR(fileStat.st_mode)) {
            // Eğer klasörse, o klasörün içine gir (_chdir) ve döngü baştan başlasın
            _chdir(input);
        } else {
            // Klasör değilse (yani bir dosyaysa veya yeni yazılmış bir isimse)
            // ismi resultPath'e kopyala ve dosya yöneticisini kapat
            strcpy(resultPath, input);
            fmRunning = 0;
        }
    }
}

// Bağlı listedeki metni belirtilen dosyaya yazan fonksiyon
void saveToFile(Editor* editor) {
    // Eğer dosya adı boşsa (daha önce hiç kaydedilmediyse)
    if (editor->filename[0] == '\0') {
        // PROJE İSTERİ: Burada aslında "Dosya Yöneticisi" açılması gerekiyor.
        // Şimdilik test edebilmek için sabit bir isim veriyoruz veya
        // ekranın en altından basitçe isim alabiliriz.

        // Ekranın en altına gidip kullanıcıdan isim isteyelim
        // Dosya adı yoksa Dosya Yöneticisini aç (mode 1: Kaydet)
        openFileManager(editor->filename, 1);

        // Eğer kullanıcı iptal dediyse işlemi durdur
        if (editor->filename[0] == '\0') return;
    }

    // Dosyayı yazma (w - write) modunda aç
    FILE *file = fopen(editor->filename, "w");

    if (file == NULL) {
        gotoxy(0, 21);
        printf("HATA: Dosya olusturulamadi!\n");
        return;
    }

    // Bağlı listeyi baştan sona gez ve dosyaya yaz
    Node* temp = editor->head;
    while (temp != NULL) {
        fputc(temp->data, file); // Karakteri dosyaya yaz
        temp = temp->next;
    }

    fclose(file); // İş bitince dosyayı mutlaka kapat

    // Kullanıcıya bilgi ver
    gotoxy(0, 21);
    printf("Basariyla kaydedildi: %s              \n", editor->filename);
    Sleep(1500); // Mesajı 1.5 saniye ekranda tut (windows.h gerektirir)
}

// Mevcut metni temizleme fonksiyonu (Yeni dosya açmadan önce RAM'i boşaltmak için)
void clearEditor(Editor* editor) {
    Node* current = editor->head;
    // Listeyi baştan sona gez ve her bir düğümü (harfi) RAM'den sil
    while (current != NULL) {
        Node* nextNode = current->next;
        free(current);
        current = nextNode;
    }
    // Editörü sıfırla
    editor->head = NULL;
    editor->tail = NULL;
    editor->cursor = NULL;
}

// Dosyadan metin okuyup editöre yükleyen fonksiyon (CTRL + O)
void openFile(Editor* editor) {
    char tempFilename[256] = "";
    // Dosya Yöneticisini aç (mode 0: Aç)
    openFileManager(tempFilename, 0);

    // Eğer kullanıcı iptal dediyse işlemi durdur
    if (tempFilename[0] == '\0') return;

    // Dosyayı okuma (r - read) modunda aç
    FILE *file = fopen(tempFilename, "r");

    if (file == NULL) {
        gotoxy(0, 21);
        printf("HATA: %s dosyasi bulunamadi!\n", tempFilename);
        Sleep(1500); // Mesajı 1.5 saniye ekranda tut
        return;
    }

    // 1. Önce ekrandaki mevcut metni sil (RAM'i temizle)
    clearEditor(editor);

    // 2. Yeni dosya adını editör yapısına kaydet
    strcpy(editor->filename, tempFilename);

    // 3. Dosyadaki karakterleri tek tek oku ve bağlı listeye ekle
    int ch;
    // EOF (End of File) yani dosya sonuna gelene kadar karakter oku
    while ((ch = fgetc(file)) != EOF) {
        insertChar(editor, (char)ch); // Daha önce yazdığımız ekleme fonksiyonunu kullanıyoruz!
    }

    fclose(file); // İşlem bitince dosyayı kapat
}

// Bul ve Değiştir Fonksiyonu (CTRL + H)
void replaceText(Editor* editor) {
    char oldWord[100];
    char newWord[100];

    // Konsolun altına inip kullanıcıdan kelimeleri al
    gotoxy(0, 20);
    printf("Degistirilecek kelime (iptal icin '-'): ");
    scanf("%99s", oldWord);

    // Proje isterinde ESC ile iptal isteniyor. scanf fonksiyonu Enter beklediği için,
    // ESC davranışını simüle etmek adına '-' girilirse iptal edilecek şekilde ayarlıyoruz.
    if (strcmp(oldWord, "-") == 0) return;

    gotoxy(0, 21);
    printf("Yeni kelime (Eski kelimeyi tamamen silmek icin '-' yazin): ");
    scanf("%99s", newWord);

    // Eğer yeni kelimeye '-' girilirse, aslında "eski kelimeyi sil" işlemi yapmak istiyoruz demektir
    if (strcmp(newWord, "-") == 0) {
        newWord[0] = '\0'; // Yeni kelimeyi boş string yap
    }

    int oldLen = strlen(oldWord);
    int newLen = strlen(newWord);
    Node* current = editor->head;

    while (current != NULL) {
        // Eğer kelime eşleşiyorsa (CTRL+F'te yazdığımız isMatch fonksiyonu)
        if (isMatch(current, oldWord)) {
            Node* nodeBeforeMatch = current->prev; // Eşleşen kelimenin hemen solundaki düğüm

            // 1. AŞAMA: Eski kelimeyi bağlı listeden kopar ve RAM'den sil
            for (int i = 0; i < oldLen; i++) {
                Node* toDelete = current;
                current = current->next; // current işaretçisini bir sağa kaydır

                // Eğer imleç silinen harfin üzerindeyse, onu güvenli bir yere (sola) çek
                if (editor->cursor == toDelete) {
                    editor->cursor = nodeBeforeMatch;
                }
                free(toDelete); // Hafızayı boşalt (Memory Leak önlemi)
            }

            Node* nodeAfterMatch = current; // Eşleşen kelimenin hemen sağında kalan düğüm

            // 2. AŞAMA: Yeni kelimenin harflerini kopan aralığa tek tek ekle
            Node* lastInserted = nodeBeforeMatch;

            for (int i = 0; i < newLen; i++) {
                Node* newNode = createNode(newWord[i]); // createNode fonksiyonumuzu kullanıyoruz

                if (lastInserted == NULL) { // Eğer metnin en başına ekliyorsak
                    newNode->next = editor->head;
                    if (editor->head) editor->head->prev = newNode;
                    editor->head = newNode;
                } else { // Eğer araya veya sona ekliyorsak
                    newNode->next = lastInserted->next;
                    newNode->prev = lastInserted;
                    lastInserted->next = newNode;
                    if (newNode->next) {
                        newNode->next->prev = newNode;
                    }
                }
                lastInserted = newNode;
            }

            // 3. AŞAMA: Kopan sağ tarafı (nodeAfterMatch) yeni kelimenin sonuna bağla
            if (nodeAfterMatch != NULL) {
                nodeAfterMatch->prev = lastInserted;
            }
            if (lastInserted != NULL) {
                lastInserted->next = nodeAfterMatch;
            }

            // Kuyruk ve Baş (Tail & Head) güncellemeleri
            if (nodeAfterMatch == NULL) editor->tail = lastInserted;
            if (nodeBeforeMatch == NULL && newLen == 0) editor->head = nodeAfterMatch;

            // Aramaya kaldığımız yerden devam et (Tüm eşleşmeleri değiştirmek için)
            current = nodeAfterMatch;
        } else {
            // Eşleşme yoksa bir sonraki harfe geçerek aramaya devam et
            current = current->next;
        }
    }
}

// KOPYALA (CTRL + C)
void copyText(Editor* editor) {
    // Eğer hiçbir şey seçili değilse işlemi iptal et
    if (editor->selectStart == NULL || editor->selectEnd == NULL) return;

    Node* temp = editor->selectStart;
    int i = 0;

    // Başlangıçtan bitişe kadar olan harfleri panoya (clipboard) kopyala
    // Panonun taşmaması için 1023 karakter sınırı koyuyoruz
    while (temp != NULL && i < 1023) {
        editor->clipboard[i] = temp->data;
        i++;
        if (temp == editor->selectEnd) break; // Bitiş düğümüne ulaştıysak dur
        temp = temp->next;
    }
    editor->clipboard[i] = '\0'; // String'in sonuna bitiş karakteri ekle
}

// YAPıŞTıR (CTRL + V)
void pasteText(Editor* editor) {
    int i = 0;
    // Panodaki her bir karakteri sırayla imlecin olduğu yere ekle
    // Daha önce yazdığımız insertChar fonksiyonu tüm bağlama işlerini bizim için yapacak!
    while (editor->clipboard[i] != '\0') {
        insertChar(editor, editor->clipboard[i]);
        i++;
    }
}

// KES (CTRL + X)
void cutText(Editor* editor) {
    if (editor->selectStart == NULL || editor->selectEnd == NULL) return;

    // 1. Önce metni panoya kopyala
    copyText(editor);

    // 2. Şimdi seçili alanı RAM'den ve ekrandan sil
    Node* current = editor->selectStart;
    Node* endNode = editor->selectEnd->next; // Bitişin bir sağına kadar sileceğiz

    // İmleci silinecek alanın soluna güvenli bir yere taşı
    editor->cursor = editor->selectStart->prev;

    while (current != endNode && current != NULL) {
        Node* toDelete = current;
        current = current->next;

        // Bağları kopar
        if (toDelete->prev != NULL) toDelete->prev->next = toDelete->next;
        else editor->head = toDelete->next; // Eğer head siliniyorsa kaydır

        if (toDelete->next != NULL) toDelete->next->prev = toDelete->prev;
        else editor->tail = toDelete->prev; // Eğer tail siliniyorsa kaydır

        free(toDelete); // Hafızayı boşalt
    }

    // Kesme işlemi bittiğine göre seçimi sıfırla
    editor->selectStart = NULL;
    editor->selectEnd = NULL;
}

int main() {
    Editor myEditor;
    myEditor.head = NULL;
    myEditor.tail = NULL;
    myEditor.cursor = NULL;
    myEditor.undoStackTop = NULL;
    myEditor.filename[0] = '\0';
    myEditor.searchWord[0] = '\0';
    myEditor.selectStart = NULL;
    myEditor.selectEnd = NULL;
    myEditor.clipboard[0] = '\0';

    setCursorAppearance();

    int running = 1;

    // Ana Döngü
    while(running) {
        // 1. Ekranı ve metni çiz
        printText(&myEditor);

        // 2. İmleci doğru yere oturt
        calculateAndMoveCursor(&myEditor);

        // 3. Kullanıcıdan tuş bekle (Program burada tuşa basılana kadar duraklar)
        int ch = _getch();
        // EĞER ÖZEL BİR TUŞA BASILDIYSA (Yön tuşları vb.)
        if (ch == 224 || ch == 0) {
            int specialKey = _getch();

            // O an CTRL veya SHIFT tuşuna basılı tutuluyor mu kontrol et
            int isCtrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
            int isShift = GetAsyncKeyState(VK_SHIFT) & 0x8000;

            // Eğer seçim yapılıyorsa ve selectStart henüz atanmamışsa, başlangıcı o anki imleç yap
            if (isCtrl && myEditor.selectStart == NULL) {
                myEditor.selectStart = myEditor.cursor;
            }
            // Eğer CTRL'ye basılmıyorsa seçimi iptal et (Normal yön tuşu kullanımı)
            else if (!isCtrl) {
                myEditor.selectStart = NULL;
                myEditor.selectEnd = NULL;
            }

            switch(specialKey) {
                case 75: // SOL OK TUŞU
                    if (isCtrl && isShift) moveWordLeft(&myEditor);
                    else moveCursorLeft(&myEditor);
                    break;
                case 77: // SAĞ OK TUŞU
                    if (isCtrl && isShift) moveWordRight(&myEditor);
                    else moveCursorRight(&myEditor);
                    break;
                case 72: // YUKARI OK TUŞU
                    moveCursorUp(&myEditor);
                    break;
                case 80: // AŞAĞI OK TUŞU
                    moveCursorDown(&myEditor);
                    break;
            }

            // Seçim işlemi devam ediyorsa, bitiş noktasını imlecin yeni yeri olarak güncelle
            if (isCtrl) {
                myEditor.selectEnd = myEditor.cursor;
            }
        }
        // EĞER NORMAL KARAKTER VEYA CTRL KISAYOLUYSA
        else {
            switch(ch) {
                case 27: // ESC Tuşu (Çıkış ve Kayıt Onayı)
                    gotoxy(0, 21); // Ekranın en altına in
                    // Satırı temizle ve soruyu sor
                    printf("Degisiklikleri kaydetmek istiyor musunuz? (y/n) [Iptal icin ESC]: ");

                    int promptRunning = 1;
                    while (promptRunning) {
                        int confirm = _getch(); // Kullanıcıdan tek bir harf bekle

                        if (confirm == 'y' || confirm == 'Y') {
                            saveToFile(&myEditor); // Dosyayı kaydet
                            running = 0; // Ana döngüyü kır (Programı kapat)
                            promptRunning = 0;
                        }
                        else if (confirm == 'n' || confirm == 'N') {
                            running = 0; // Kaydetmeden doğrudan programı kapat
                            promptRunning = 0;
                        }
                        else if (confirm == 27) {
                            // Eğer ESC'ye tekrar basarsa çıkış işlemini iptal et, editöre dön
                            promptRunning = 0;
                        }
                    }
                    break;

                    // BACKSPACE ve CTRL+H ÇAKIŞMASI ÇÖZÜMÜ
                case 8:
                    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        // CTRL + H işlemi (Bul ve Değiştir)
                        replaceText(&myEditor);
                    } else {
                        // Normal Backspace işlemi (Karakter silme)
                        deleteChar(&myEditor);
                    }
                    break;

                case 13: // Enter Tuşu
                    insertChar(&myEditor, '\n');
                    break;

                case 3: // CTRL + C
                    copyText(&myEditor);
                    break;
                case 6: // CTRL + F
                    gotoxy(0, 20); // Ekranın altına in
                    printf("Aranacak kelimeyi girin (Aramayi temizlemek icin '-' yazin): ");
                    scanf("%99s", myEditor.searchWord);

                    // Eğer kullanıcı '-' girdiyse aramayı iptal et/temizle
                    if (strcmp(myEditor.searchWord, "-") == 0) {
                        myEditor.searchWord[0] = '\0';
                    }
                    break;
                case 15: // CTRL + O
                    openFile(&myEditor);
                    break;
                case 19: // CTRL + S
                    saveToFile(&myEditor);
                    break;
                case 22: // CTRL + V
                    pasteText(&myEditor);
                    break;
                case 24: // CTRL + X
                    cutText(&myEditor);
                    break;
                case 26: // CTRL + Z
                    undo(&myEditor);
                    break;
                case 127: // CTRL + Backspace
                    deleteWord(&myEditor);
                    break;
                    //NORMAL YAZI YAZMA
                default:
                    // Klavyeden girilen karakter ekranda gösterilebilir bir karakterse
                    if (ch >= 32 && ch <= 126) {
                        insertChar(&myEditor, (char)ch);
                    }
                    break;
            }
        }
    }
    return 0;
}

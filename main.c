#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h> // _getch() için gerekli
#include <string.h> // strcpy fonksiyonu için gerekli
#include <direct.h>   // _getcwd ve _chdir fonksiyonları için
#include <sys/stat.h> // S_ISDIR makrosu ve stat fonksiyonu için

int isUndoing = 0; // Geri alma işlemi yapılıp yapılmadığını takip eden bayrak
char clipboard[8192] = ""; // Kopyalanan metinleri tutacak pano
int isNavigatingMatch = 0; //Gezinme modunda olup olmadığımızı tutar

// ZAMAN MAKİNESİ (UNDO STACK) DEĞİŞKENLERİ
#define MAX_UNDO_STATES 20 // Geriye dönük en fazla 20 işlemi hafızada tutar
#define MAX_TEXT_SIZE 8192 // Bir metnin alabileceği maksimum harf sayısı

char undoStack[MAX_UNDO_STATES][MAX_TEXT_SIZE];
int undoTop = -1; // Yığının en üstünü gösteren işaretçi

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
// BACKSPACE (SİLME) İŞLEMİ
void deleteChar(Editor* editor) {
    // 1. Durum: Dosya tamamen boşsa zaten silinecek bir şey yoktur.
    if (editor->head == NULL) return;

    Node* toDelete = NULL;

    // 2. Durum: İmleç metnin en sonundaysa (NULL ise), en sondaki harfi (tail) sil.
    if (editor->cursor == NULL) {
        toDelete = editor->tail;
    }
    // 3. DURUM (ÇÖKMEYİ ENGELLEYEN YER): İmleç en baştaysa (head), silinecek harf YOKTUR!
    else if (editor->cursor == editor->head) {
        return; // Hiçbir şey yapmadan sessizce fonksiyondan çık. Programı kurtar.
    }
    // 4. Durum: İmleç aralarda bir yerdeyse, imlecin bir gerisindeki harfi sil.
    else {
        toDelete = editor->cursor->prev;
    }

    // Güvenlik teyidi
    if (toDelete == NULL) return;

    // --- DÜĞÜMÜ LİSTEDEN GÜVENLİCE KOPARMA ---
    if (toDelete->prev != NULL) {
        toDelete->prev->next = toDelete->next;
    } else {
        editor->head = toDelete->next; // Eğer ilk düğüm silindiyse head'i kaydır
    }

    if (toDelete->next != NULL) {
        toDelete->next->prev = toDelete->prev;
    } else {
        editor->tail = toDelete->prev; // Eğer son düğüm silindiyse tail'i kaydır
    }

    free(toDelete); // RAM'i (Belleği) temizle
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

// Düğümün (harfin) metindeki sırasını (indeksini) bulur.
// İmleç en baştaysa (NULL) 0 döner.
int getNodeIndex(Editor* editor, Node* target) {
    if (target == NULL) return 0;
    int idx = 1;
    Node* temp = editor->head;
    while (temp != NULL) {
        if (temp == target) return idx;
        idx++;
        temp = temp->next;
    }
    return 0;
}
// 1. Ekrandaki fiziksel imleci (X, Y) koordinatına taşıyan fonksiyon
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
// KOPYALA (CTRL+C)
void copySelected(Editor* editor) {
    if (editor->selectStart == NULL || editor->selectEnd == NULL) return;

    int startIdx = getNodeIndex(editor, editor->selectStart);
    int endIdx = getNodeIndex(editor, editor->selectEnd);
    int minIdx = (startIdx < endIdx) ? startIdx : endIdx;
    int maxIdx = (startIdx > endIdx) ? startIdx : endIdx;

    Node* current = editor->head;
    int currentIdx = 1;
    int clipIdx = 0;

    while (current != NULL) {
        if (currentIdx > minIdx && currentIdx <= maxIdx) {
            clipboard[clipIdx++] = current->data;
        }
        current = current->next;
        currentIdx++;
    }
    clipboard[clipIdx] = '\0'; // Panodaki metni sonlandır

    // Kopyalama bitince seçimi iptal et
    editor->selectStart = NULL;
    editor->selectEnd = NULL;
}

// KES (CTRL+X)
void cutSelected(Editor* editor) {
    if (editor->selectStart == NULL || editor->selectEnd == NULL) return;

    // 1. Önce Kopyala (Ama seçimi iptal etmemesi için kopyalama işlemini manuel yapıyoruz)
    int startIdx = getNodeIndex(editor, editor->selectStart);
    int endIdx = getNodeIndex(editor, editor->selectEnd);
    int minIdx = (startIdx < endIdx) ? startIdx : endIdx;
    int maxIdx = (startIdx > endIdx) ? startIdx : endIdx;

    Node* current = editor->head;
    int currentIdx = 1;
    int clipIdx = 0;

    // Kopyalama Döngüsü
    while (current != NULL) {
        if (currentIdx > minIdx && currentIdx <= maxIdx) {
            clipboard[clipIdx++] = current->data;
        }
        current = current->next;
        currentIdx++;
    }
    clipboard[clipIdx] = '\0';

    // 2. Silme Döngüsü
    current = editor->head;
    currentIdx = 1;
    while (current != NULL) {
        Node* nextNode = current->next;

        if (currentIdx > minIdx && currentIdx <= maxIdx) {
            // Düğümü listeden kopar
            if (current->prev) current->prev->next = current->next;
            else editor->head = current->next; // Baştaysa head'i güncelle

            if (current->next) current->next->prev = current->prev;
            else editor->tail = current->prev; // Sondayysa tail'i güncelle

            // Eğer imleç silinen harfteyse, imleci bir öncekine kaydır
            if (editor->cursor == current) {
                editor->cursor = current->prev;
            }
            free(current); // Bellekten sil
        }
        current = nextNode;
        currentIdx++;
    }

    editor->selectStart = NULL;
    editor->selectEnd = NULL;
}

// YAPIŞTIR (CTRL+V)
void pasteClipboard(Editor* editor) {
    if (strlen(clipboard) == 0) return; // Pano boşsa hiçbir şey yapma

    // Panodaki her harfi sırayla editöre ekle
    for (int i = 0; clipboard[i] != '\0'; i++) {
        insertChar(editor, clipboard[i]); // Sende zaten var olan harf ekleme fonksiyonu
    }
}

// METNİN FOTOĞRAFINI ÇEKİP YIĞINA ATAR (Değişiklik yapmadan hemen önce çağrılır)
void saveState(Editor* editor) {
    // Eğer yığın doluysa, en eski kayıtları silip yer aç (Kaydırma işlemi)
    if (undoTop >= MAX_UNDO_STATES - 1) {
        for (int i = 0; i < MAX_UNDO_STATES - 1; i++) {
            strcpy(undoStack[i], undoStack[i + 1]);
        }
        undoTop = MAX_UNDO_STATES - 2;
    }

    undoTop++; // Yığının tepesini bir artır

    // Bağlı listedeki tüm harfleri okuyup string olarak yığına kaydet
    Node* temp = editor->head;
    int i = 0;
    while (temp != NULL && i < MAX_TEXT_SIZE - 1) {
        undoStack[undoTop][i++] = temp->data;
        temp = temp->next;
    }
    undoStack[undoTop][i] = '\0'; // Stringi bitir
}

// ZAMANI GERİ ALIR (CTRL + Z)
void undoAction(Editor* editor) {
    if (undoTop < 0) {
        // Yığın boşsa (geri alınacak bir şey kalmadıysa) uyar
        gotoxy(0, 21);
        printf("Geri alinacak bir islem yok!       ");
        Sleep(1000);
        return;
    }

    // 1. MEVCUT METNİ TAMAMEN TEMİZLE
    Node* temp = editor->head;
    while (temp != NULL) {
        Node* toDelete = temp;
        temp = temp->next;
        free(toDelete);
    }
    editor->head = NULL;
    editor->tail = NULL;
    editor->cursor = NULL;
    editor->selectStart = NULL;
    editor->selectEnd = NULL;

    // 2. YIĞINDAKİ (STACK) SON METNİ GERİ YÜKLE
    char* previousState = undoStack[undoTop];
    for (int i = 0; previousState[i] != '\0'; i++) {
        insertChar(editor, previousState[i]); // Sende zaten var olan harf ekleme fonksiyonu
    }

    undoTop--; // Yığından bir adım geri git (fotoğrafı çöpe at)
}


// Ekranı ve Metni Çizdirme Fonksiyonu
void printText(Editor* editor) {
    //system("cls");
    //printf("\x1b[2J\x1b[H");
    printf("\x1b[H\x1b[J");

    printf(" Dosya Ac(CTRL+O) | Kaydet(CTRL+S) | Farkli Kaydet(CTRL+SHIFT+S) | Bul(CTRL+F) | Degistir(CTRL+H)\n");
    printf(" Kopyala(CTRL+C)  | Sec(CTRL+YON)  | Kes(CTRL+X) | Yapistir(CTRL+V) | Geri Al(CTRL+Z) | Cikis(ESC)\n");
    printf("--------------------------------------------------------------------------------------------------\n");

    int lineNumber = 1;
    printf("%3d | ", lineNumber);

    int startIdx = getNodeIndex(editor, editor->selectStart);
    int endIdx = getNodeIndex(editor, editor->selectEnd);
    int minIdx = (startIdx < endIdx) ? startIdx : endIdx;
    int maxIdx = (startIdx > endIdx) ? startIdx : endIdx;

    Node* current = editor->head;
    int currentIdx = 1;

    int searchLen = strlen(editor->searchWord);
    int blueRemaining = 0;

    while (current != NULL) {
        // --- ARAMA (MAVİ) KONTROLÜ ---
        if (searchLen > 0 && blueRemaining == 0) {
            if (isMatch(current, editor->searchWord)) {
                // EĞER CTRL+G İLE GEZİNİYORSAK (1) SADECE İMLECİN ÜZERİNDE OLDUĞU KELİMEYİ BOYARIZ.
                // EĞER NORMAL CTRL+F İSE (0) HEPSİNİ BOYARIZ.
                if (isNavigatingMatch == 0 || current == editor->cursor) {
                    blueRemaining = searchLen;
                }
            }
        }

        // --- SEÇİM (SARI) KONTROLÜ ---
        int isSelected = 0;
        if ((editor->selectStart != NULL || editor->selectEnd != NULL) && startIdx != endIdx) {
            if (currentIdx > minIdx && currentIdx <= maxIdx) {
                isSelected = 1;
            }
        }

        // --- RENKLENDİRME ---
        if (blueRemaining > 0) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_BLUE | BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            blueRemaining--;
        }
        else if (isSelected) {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_RED | BACKGROUND_GREEN | FOREGROUND_INTENSITY);
        }
        else {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }

        putchar(current->data);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        if (current->data == '\n') {
            lineNumber++;
            printf("%3d | ", lineNumber);
        }

        current = current->next;
        currentIdx++;
    }
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

    // 1. AŞAMA: Eski Kelimeyi Al
    gotoxy(0, 20);
    printf("Degistirilecek kelime [Iptal icin ESC]: ");

    int i = 0;
    while (1) {
        int ch = _getch();
        if (ch == 27) return; // ESC: İşlemi anında iptal et ve çık
        else if (ch == 13) { // ENTER: Kelime girişini tamamla
            oldWord[i] = '\0';
            break;
        }
        else if (ch == 8) { // BACKSPACE: Karakter sil
            if (i > 0) {
                i--;
                printf("\b \b"); // Ekrandaki son harfi sil
            }
        }
        else if (ch >= 32 && ch <= 126 && i < 99) { // NORMAL HARFLER
            oldWord[i++] = (char)ch;
            putchar(ch); // Harfi ekrana bas
        }
    }

    if (i == 0) return; // Hiçbir şey yazmadan Enter'a basıldıysa çık

    // 2. AŞAMA: Yeni Kelimeyi Al
    gotoxy(0, 21);
    printf("Yeni kelime [Iptal icin ESC]: ");

    int j = 0;
    while (1) {
        int ch = _getch();
        if (ch == 27) return; // ESC: İşlemi anında iptal et ve çık
        else if (ch == 13) { // ENTER: Kelime girişini tamamla
            newWord[j] = '\0';
            break;
        }
        else if (ch == 8) { // BACKSPACE: Karakter sil
            if (j > 0) {
                j--;
                printf("\b \b"); // Ekrandaki son harfi sil
            }
        }
        else if (ch >= 32 && ch <= 126 && j < 99) { // NORMAL HARFLER
            newWord[j++] = (char)ch;
            putchar(ch); // Harfi ekrana bas
        }
    }

    // 3. AŞAMA: Değiştirme Algoritması (Burası aynı kalıyor)
    int oldLen = strlen(oldWord);
    int newLen = strlen(newWord);
    Node* current = editor->head;

    while (current != NULL) {
        if (isMatch(current, oldWord)) {
            Node* nodeBeforeMatch = current->prev;

            for (int k = 0; k < oldLen; k++) {
                Node* toDelete = current;
                current = current->next;

                if (editor->cursor == toDelete) {
                    editor->cursor = nodeBeforeMatch;
                }
                free(toDelete);
            }

            Node* nodeAfterMatch = current;
            Node* lastInserted = nodeBeforeMatch;

            for (int k = 0; k < newLen; k++) {
                Node* newNode = createNode(newWord[k]);

                if (lastInserted == NULL) {
                    newNode->next = editor->head;
                    if (editor->head) editor->head->prev = newNode;
                    editor->head = newNode;
                } else {
                    newNode->next = lastInserted->next;
                    newNode->prev = lastInserted;
                    lastInserted->next = newNode;
                    if (newNode->next) {
                        newNode->next->prev = newNode;
                    }
                }
                lastInserted = newNode;
            }

            if (nodeAfterMatch != NULL) {
                nodeAfterMatch->prev = lastInserted;
            }
            if (lastInserted != NULL) {
                lastInserted->next = nodeAfterMatch;
            }

            if (nodeAfterMatch == NULL) editor->tail = lastInserted;
            if (nodeBeforeMatch == NULL && newLen == 0) editor->head = nodeAfterMatch;

            current = nodeAfterMatch;
        } else {
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

// CTRL + G: Eşleşmeleri Bul ve Aralarında Gezin
void findAndNavigateMatches(Editor* editor) {
    char word[100];
    gotoxy(0, 20);
    printf("Aranacak kelimeyi girin (CTRL+G): ");
    scanf("%99s", word);

    strcpy(editor->searchWord, word);

    Node* matches[1000];
    int matchCount = 0;

    Node* current = editor->head;
    while (current != NULL) {
        if (isMatch(current, word)) {
            if (matchCount < 1000) {
                matches[matchCount] = current;
                matchCount++;
            }
        }
        current = current->next;
    }

    if (matchCount == 0) {
        gotoxy(0, 21);
        printf("Sonuc: '%s' kelimesi metinde bulunamadi.\n", word);
        editor->searchWord[0] = '\0';
        Sleep(1500);
        return;
    }

    int currentIndex = 0;
    int navigating = 1;

    isNavigatingMatch = 1; // EKLENDİ: Tekli mavi boyama modunu aç

    while (navigating) {
        editor->cursor = matches[currentIndex];

        printText(editor);

        gotoxy(0, 21);
        printf("--> Eslesme: %d / %d | [SOL] Onceki | [SAG] Sonraki | [ESC] Cikis", currentIndex + 1, matchCount);

        calculateAndMoveCursor(editor);

        int ch = _getch();
        if (ch == 224 || ch == 0) {
            int special = _getch();
            if (special == 75) {
                if (currentIndex > 0) currentIndex--;
                else currentIndex = matchCount - 1;
            }
            else if (special == 77) {
                if (currentIndex < matchCount - 1) currentIndex++;
                else currentIndex = 0;
            }
        }
        else if (ch == 27) {
            navigating = 0;
            editor->searchWord[0] = '\0';
            isNavigatingMatch = 0; // EKLENDİ: Tekli mavi boyama modunu kapat
        }
    }
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

            // Eğer CTRL'ye basılıysa ve seçim yeni başlıyorsa başlangıç noktasını al
            if (isCtrl && myEditor.selectStart == NULL) {
                myEditor.selectStart = myEditor.cursor;
            }
            // Eğer CTRL'ye basılmıyorsa seçimi iptal et (Normal yön tuşu kullanımı)
            else if (!isCtrl) {
                myEditor.selectStart = NULL;
                myEditor.selectEnd = NULL;
            }

            // --- YÖN TUŞLARI MANTIĞI ---
            // C dilinde CTRL + Ok tuşları farklı (115, 116, 141, 145) değerler döndürür!
            if (specialKey == 75 || specialKey == 115) { // SOL OK veya CTRL+SOL OK
                if (isCtrl && isShift) moveWordLeft(&myEditor); // Kelime Kelime Seç (İster 2)
                else moveCursorLeft(&myEditor);                 // Karakter Karakter Seç (İster 1)
            }
            else if (specialKey == 77 || specialKey == 116) { // SAĞ OK veya CTRL+SAĞ OK
                if (isCtrl && isShift) moveWordRight(&myEditor);
                else moveCursorRight(&myEditor);
            }
            else if (specialKey == 72 || specialKey == 141) { // YUKARI OK
                moveCursorUp(&myEditor);
            }
            else if (specialKey == 80 || specialKey == 145) { // AŞAĞI OK
                moveCursorDown(&myEditor);
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
                case 7: // CTRL + G (Eşleşmeleri Bul ve Gezin)
                    findAndNavigateMatches(&myEditor);
                    break;
                    // BACKSPACE ve CTRL+H ÇAKIŞMASI ÇÖZÜMÜ
                case 8:
                    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                        // CTRL + H işlemi (Bul ve Değiştir)
                        saveState(&myEditor);
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
                    copySelected(&myEditor);
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
                    saveState(&myEditor);
                    pasteClipboard(&myEditor);
                    break;
                case 24: // CTRL + X
                    saveState(&myEditor);
                    cutSelected(&myEditor);
                    break;
                case 26: // CTRL + Z
                    undoAction(&myEditor);
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

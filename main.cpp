#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <stack>
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>
#include <list>

using namespace std;

struct Book {
    string isbn;
    string title;
    string author;
    string genre;
    int stock;
};

enum RequestType {
    BORROW,
    RETURN
};

struct Request {
    string user_id;
    string book_isbn;
    RequestType type;
};

enum ActionType {
    ACTION_BORROWED,
    ACTION_RETURNED
};

struct Action {
    string user_id;
    string book_isbn;
    ActionType type;
};

class LibrarySystem {
private:
    map<string, list<Book>> booksByGenre;
    map<string, Book*> booksByIsbn;
    map<string, Book*> booksByTitle;
    queue<Request> transactionQueue;
    stack<Action> actionHistory;

    void loadDatabaseFromFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Tidak dapat membuka file database: " << filename << endl;
            return;
        }

        string line;
        while (getline(file, line)) {
            if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos) {
                continue;
            }

            stringstream ss(line);
            string isbn, title, author, genre, stock_str;
            int stock;

            if (getline(ss, isbn, ',') &&
                getline(ss, title, ',') &&
                getline(ss, author, ',') &&
                getline(ss, genre, ',') &&
                getline(ss, stock_str)) {
                
                try {
                    stock = stoi(stock_str);
                } catch (const invalid_argument& e) {
                    continue;
                }
                addBook(isbn, title, author, genre, stock);
            }
        }
        file.close();
    }

public:
    LibrarySystem(const string& dbFilename) {
        loadDatabaseFromFile(dbFilename);
    }

    void saveDatabaseToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Tidak dapat menyimpan perubahan ke file database: " << filename << endl;
            return;
        }
        for (const auto& pair : booksByGenre) {
            for (const auto& book : pair.second) {
                file << book.isbn << "," << book.title << "," << book.author << "," << book.genre << "," << book.stock << "\n";
            }
        }
        file.close();
    }

    void addBook(const string& isbn, const string& title, const string& author, const string& genre, int stock) {
        Book newBook = {isbn, title, author, genre, stock};
        booksByGenre[genre].push_back(newBook);
        Book* bookPtr = &booksByGenre[genre].back();
        booksByIsbn[isbn] = bookPtr;
        booksByTitle[title] = bookPtr;
    }

    void displayAllBooks() {
        cout << "========================================\n";
        cout << "           DAFTAR SEMUA BUKU\n";
        cout << "========================================\n";
        if (booksByGenre.empty()) {
            cout << "Perpustakaan masih kosong atau gagal memuat database.\n";
            return;
        }

        for (const auto& entry : booksByGenre) {
            const string& genre = entry.first;
            const list<Book>& bookList = entry.second;

            cout << "\n--- Genre: " << genre << " ---\n";
            for (const auto& book : bookList) {
                cout << "  Judul    : " << book.title << endl;
                cout << "  Author   : " << book.author << endl;
                cout << "  ISBN     : " << book.isbn << endl;
                cout << "  Stok     : " << book.stock << endl;
                cout << "  --------------------------------------\n";
            }
        }
    }

    Book* findBookByIsbn(const string& isbn) {
        if (booksByIsbn.count(isbn)) {
            return booksByIsbn[isbn];
        }
        return nullptr;
    }

    void createBorrowRequest(const string& user, const string& isbn) {
        Book* book = findBookByIsbn(isbn);
        if (!book) {
            cout << "Error: Buku dengan ISBN tersebut tidak ditemukan.\n";
            return;
        }
        if (book->stock <= 0) {
            cout << "Info: Stok buku sedang habis, permintaan Anda tetap dimasukkan ke antrian.\n";
        }
        transactionQueue.push({user, isbn, BORROW});
        cout << "Sukses: Permintaan peminjaman buku \"" << book->title << "\" telah ditambahkan ke antrian.\n";
    }

    void createReturnRequest(const string& user, const string& isbn) {
        Book* book = findBookByIsbn(isbn);
        if (!book) {
            cout << "Error: Buku dengan ISBN tersebut tidak ditemukan.\n";
            return;
        }
        transactionQueue.push({user, isbn, RETURN});
        cout << "Sukses: Permintaan pengembalian buku \"" << book->title << "\" telah ditambahkan ke antrian.\n";
    }

    void processNextRequest() {
        if (transactionQueue.empty()) {
            cout << "Info: Tidak ada permintaan di dalam antrian.\n";
            return;
        }

        Request req = transactionQueue.front();
        transactionQueue.pop();
        Book* book = findBookByIsbn(req.book_isbn);
        if(!book) return;

        cout << "Memproses permintaan dari User '" << req.user_id << "' untuk buku '" << book->title << "'...\n";

        if (req.type == BORROW) {
            if (book->stock > 0) {
                book->stock--;
                actionHistory.push({req.user_id, req.book_isbn, ACTION_BORROWED});
                cout << "Status: SUKSES. Buku berhasil dipinjam. Sisa stok: " << book->stock << "\n";
            } else {
                cout << "Status: GAGAL. Stok buku habis saat permintaan diproses.\n";
            }
        } else {
            book->stock++;
            actionHistory.push({req.user_id, req.book_isbn, ACTION_RETURNED});
            cout << "Status: SUKSES. Buku berhasil dikembalikan. Stok sekarang: " << book->stock << "\n";
        }
    }

    void undoLastAction() {
        if (actionHistory.empty()) {
            cout << "Info: Tidak ada aksi yang bisa di-undo.\n";
            return;
        }

        Action lastAction = actionHistory.top();
        actionHistory.pop();
        Book* book = findBookByIsbn(lastAction.book_isbn);
        if(!book) return;

        cout << "Membatalkan aksi terakhir untuk buku '" << book->title << "'...\n";

        if (lastAction.type == ACTION_BORROWED) {
            book->stock++;
            cout << "Status: SUKSES. Peminjaman dibatalkan. Stok sekarang: " << book->stock << "\n";
        } else {
            book->stock--;
            cout << "Status: SUKSES. Pengembalian dibatalkan. Stok sekarang: " << book->stock << "\n";
        }
    }

    void getRecommendations(const string& isbn) {
        Book* sourceBook = findBookByIsbn(isbn);
        if (!sourceBook) {
            cout << "Error: Buku dengan ISBN tersebut tidak ditemukan.\n";
            return;
        }

        cout << "\n========================================\n";
        cout << "   REKOMENDASI BUKU SERUPA\n";
        cout << "Berdasarkan buku: \"" << sourceBook->title << "\"\n";
        cout << "Genre: " << sourceBook->genre << "\n";
        cout << "========================================\n";

        int count = 0;
        const auto& bookList = booksByGenre.at(sourceBook->genre);
        for (const auto& book : bookList) {
            if (book.isbn != sourceBook->isbn && book.stock > 0) {
                cout << "  - " << book.title << " oleh " << book.author << endl;
                count++;
            }
        }

        if (count == 0) {
            cout << "Tidak ada buku lain yang tersedia di genre ini saat ini.\n";
        }
    }
};

void displayMenu() {
    cout << "\n\n===== Strukdat Library System =====\n";
    cout << "1. Lihat Semua Buku\n";
    cout << "2. Cari Buku (by ISBN)\n";
    cout << "3. Buat Permintaan Pinjam Buku\n";
    cout << "4. Buat Permintaan Kembali Buku\n";
    cout << "5. Proses Permintaan Berikutnya\n";
    cout << "6. Undo Aksi Terakhir\n";
    cout << "7. Dapatkan Rekomendasi Buku\n";
    cout << "0. Keluar\n";
    cout << "=================================\n";
    cout << "Pilih opsi: ";
}

void clearInputBuffer() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {
    string dbFilename = "database.txt";
    LibrarySystem library(dbFilename);
    int choice;
    string isbn, user;

    do {
        displayMenu();
        cin >> choice;

        if (cin.fail()) {
            cout << "Error: Input tidak valid. Harap masukkan angka.\n";
            cin.clear();
            clearInputBuffer();
            choice = -1;
            continue;
        }

        clearInputBuffer();

        switch (choice) {
            case 1:
                library.displayAllBooks();
                break;
            case 2:
                cout << "Masukkan ISBN buku yang dicari: ";
                getline(cin, isbn);
                {
                    Book* book = library.findBookByIsbn(isbn);
                    if (book) {
                        cout << "\n--- Buku Ditemukan ---\n";
                        cout << "  Judul    : " << book->title << endl;
                        cout << "  Author   : " << book->author << endl;
                        cout << "  ISBN     : " << book->isbn << endl;
                        cout << "  Genre    : " << book->genre << endl;
                        cout << "  Stok     : " << book->stock << endl;
                    } else {
                        cout << "Buku dengan ISBN '" << isbn << "' tidak ditemukan.\n";
                    }
                }
                break;
            case 3:
                cout << "Masukkan ID User Anda: ";
                getline(cin, user);
                cout << "Masukkan ISBN buku yang akan dipinjam: ";
                getline(cin, isbn);
                library.createBorrowRequest(user, isbn);
                break;
            case 4:
                cout << "Masukkan ID User Anda: ";
                getline(cin, user);
                cout << "Masukkan ISBN buku yang akan dikembalikan: ";
                getline(cin, isbn);
                library.createReturnRequest(user, isbn);
                break;
            case 5:
                library.processNextRequest();
                break;
            case 6:
                library.undoLastAction();
                break;
            case 7:
                cout << "Masukkan ISBN buku yang Anda suka untuk mendapatkan rekomendasi: ";
                getline(cin, isbn);
                library.getRecommendations(isbn);
                break;
            case 0:
                cout << "Menyimpan perubahan ke database...\n";
                library.saveDatabaseToFile(dbFilename);
                cout << "Terima kasih telah menggunakan Strukdat Library System. Sampai jumpa!\n";
                break;
            default:
                cout << "Error: Pilihan tidak valid. Silakan coba lagi.\n";
        }
    } while (choice != 0);

    return 0;
}

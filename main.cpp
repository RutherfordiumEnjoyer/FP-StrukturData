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

// Struktur data untuk menyimpan informasi buku
struct Book {
    string isbn;
    string title;
    string author;
    string genre;
    int stock;
};

// Jenis permintaan transaksi
enum RequestType {
    BORROW,
    RETURN
};

// Struktur permintaan user
struct Request {
    string user_id;
    string book_isbn;
    RequestType type;
};

// Jenis aksi untuk fitur undo
enum ActionType {
    ACTION_BORROWED,
    ACTION_RETURNED
};

// Struktur aksi yang disimpan ke stack
struct Action {
    string user_id;
    string book_isbn;
    ActionType type;
};

class LibrarySystem {
private:
    map<string, list<Book>> booksByGenre;     // Buku dikelompokkan berdasarkan genre
    map<string, Book*> booksByIsbn;           // Pointer buku berdasarkan ISBN (untuk pencarian cepat)
    map<string, Book*> booksByTitle;          // Pointer buku berdasarkan judul (untuk pencarian cepat)
    queue<Request> transactionQueue;          // Antrian permintaan pinjam/kembali (FIFO)
    stack<Action> actionHistory;              // Stack riwayat aksi (untuk undo) (LIFO)

    // Fungsi untuk load data dari file database
    void loadDatabaseFromFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Tidak dapat membuka file database: " << filename << endl;
            return;
        }

        string line;
        // Looping setiap baris dalam file
        while (getline(file, line)) {
            // Skip baris kosong atau yang hanya berisi whitespace
            if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos)
                continue;

            stringstream ss(line);
            string isbn, title, author, genre, stock_str;
            int stock;

            // Parsing data dari format CSV (comma-separated values)
            if (getline(ss, isbn, ',') &&
                getline(ss, title, ',') &&
                getline(ss, author, ',') &&
                getline(ss, genre, ',') &&
                getline(ss, stock_str)) {
                
                try {
                    stock = stoi(stock_str);  // Konversi string ke integer
                } catch (...) {
                    continue; // Skip baris jika konversi gagal
                }
                addBook(isbn, title, author, genre, stock);
            }
        }
        file.close();
    }

public:
    // Constructor: load database dari file saat object dibuat
    LibrarySystem(const string& dbFilename) {
        loadDatabaseFromFile(dbFilename);
    }

    // Simpan semua buku kembali ke file
    void saveDatabaseToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Gagal menyimpan ke file: " << filename << endl;
            return;
        }

        // Loop semua buku di setiap genre dan tulis ke file
        for (const auto& pair : booksByGenre) {
            for (const auto& book : pair.second) {
                file << book.isbn << "," << book.title << "," << book.author << "," << book.genre << "," << book.stock << "\n";
            }
        }
        file.close();
    }

    // Tambahkan buku baru ke sistem
    void addBook(const string& isbn, const string& title, const string& author, const string& genre, int stock) {
        // Buat buku baru dan tambahkan ke list berdasarkan genre
        Book newBook = {isbn, title, author, genre, stock};
        booksByGenre[genre].push_back(newBook);

        // Simpan pointer ke buku yang baru ditambahkan untuk pencarian cepat
        Book* bookPtr = &booksByGenre[genre].back();
        booksByIsbn[isbn] = bookPtr;
        booksByTitle[title] = bookPtr;
    }

    // Tampilkan semua buku di perpustakaan
    void displayAllBooks() {
        cout << "========================================\n";
        cout << "           DAFTAR SEMUA BUKU\n";
        cout << "========================================\n";

        if (booksByGenre.empty()) {
            cout << "Perpustakaan kosong.\n";
            return;
        }

        // Loop melalui setiap genre dan tampilkan buku-bukunya
        for (const auto& entry : booksByGenre) {
            cout << "\n--- Genre: " << entry.first << " ---\n";
            for (const auto& book : entry.second) {
                cout << "  Judul    : " << book.title << endl;
                cout << "  Author   : " << book.author << endl;
                cout << "  ISBN     : " << book.isbn << endl;
                cout << "  Stok     : " << book.stock << endl;
                cout << "  --------------------------------------\n";
            }
        }
    }

    // Cari buku berdasarkan ISBN menggunakan map untuk O(1) lookup
    Book* findBookByIsbn(const string& isbn) {
        if (booksByIsbn.count(isbn)) {
            return booksByIsbn[isbn]; // Return pointer buku
        }
        return nullptr;
    }

    // Tambahkan permintaan peminjaman ke antrian
    void createBorrowRequest(const string& user, const string& isbn) {
        Book* book = findBookByIsbn(isbn);
        if (!book) {
            cout << "Error: Buku tidak ditemukan.\n";
            return;
        }
        if (book->stock <= 0) {
            cout << "Info: Stok habis, tetap dimasukkan ke antrian.\n";
        }
        transactionQueue.push({user, isbn, BORROW});
        cout << "Permintaan peminjaman \"" << book->title << "\" ditambahkan.\n";
    }

    // Tambahkan permintaan pengembalian ke antrian
    void createReturnRequest(const string& user, const string& isbn) {
        Book* book = findBookByIsbn(isbn);
        if (!book) {
            cout << "Error: Buku tidak ditemukan.\n";
            return;
        }
        transactionQueue.push({user, isbn, RETURN});
        cout << "Permintaan pengembalian \"" << book->title << "\" ditambahkan.\n";
    }

    // Proses permintaan pertama di antrian (FIFO)
    void processNextRequest() {
        if (transactionQueue.empty()) {
            cout << "Tidak ada permintaan yang menunggu.\n";
            return;
        }

        // Ambil request paling depan
        Request req = transactionQueue.front();
        transactionQueue.pop();
        Book* book = findBookByIsbn(req.book_isbn);
        if (!book) return;

        cout << "Memproses permintaan dari " << req.user_id << " untuk buku \"" << book->title << "\"...\n";

        if (req.type == BORROW) {
            if (book->stock > 0) {
                book->stock--;
                // Simpan aksi ke history untuk undo
                actionHistory.push({req.user_id, req.book_isbn, ACTION_BORROWED});
                cout << "Dipinjam. Sisa stok: " << book->stock << endl;
            } else {
                cout << "Stok habis saat diproses.\n";
            }
        } else {
            book->stock++;
            // Simpan aksi ke history untuk undo
            actionHistory.push({req.user_id, req.book_isbn, ACTION_RETURNED});
            cout << "Dikembalikan. Stok sekarang: " << book->stock << endl;
        }
    }

    // Undo aksi terakhir (LIFO)
    void undoLastAction() {
        if (actionHistory.empty()) {
            cout << "Tidak ada aksi untuk dibatalkan.\n";
            return;
        }

        // Ambil aksi teratas dari stack
        Action last = actionHistory.top();
        actionHistory.pop();
        Book* book = findBookByIsbn(last.book_isbn);
        if (!book) return;

        cout << "Undo aksi pada buku \"" << book->title << "\"...\n";
        if (last.type == ACTION_BORROWED) {
            // Kembalikan stok yang dikurangi sebelumnya
            book->stock++;
            cout << "Peminjaman dibatalkan. Stok: " << book->stock << endl;
        } else {
            // Kurangi stok yang ditambah sebelumnya
            book->stock--;
            cout << "Pengembalian dibatalkan. Stok: " << book->stock << endl;
        }
    }

    // Berikan rekomendasi buku berdasarkan genre buku yang diberikan
    void getRecommendations(const string& isbn) {
        Book* source = findBookByIsbn(isbn);
        if (!source) {
            cout << "Buku tidak ditemukan.\n";
            return;
        }

        cout << "\n=== Rekomendasi Buku Genre \"" << source->genre << "\" ===\n";

        int count = 0;
        // Loop semua buku dalam genre yang sama kecuali buku itu sendiri
        for (const auto& book : booksByGenre[source->genre]) {
            if (book.isbn != source->isbn && book.stock > 0) {
                cout << "  - " << book.title << " oleh " << book.author << endl;
                count++;
            }
        }

        if (count == 0) {
            cout << "Tidak ada rekomendasi saat ini.\n";
        }
    }
};

// Tampilan menu utama
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

// Bersihkan input buffer saat cin error
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

        // Handle input error
        if (cin.fail()) {
            cout << "Input tidak valid.\n";
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
                cout << "Masukkan ISBN: ";
                getline(cin, isbn);
                {
                    Book* book = library.findBookByIsbn(isbn);
                    if (book) {
                        cout << "Ditemukan: " << book->title << " oleh " << book->author << ", stok: " << book->stock << endl;
                    } else {
                        cout << "Buku tidak ditemukan.\n";
                    }
                }
                break;
            case 3:
                cout << "Masukkan ID User: ";
                getline(cin, user);
                cout << "Masukkan ISBN buku: ";
                getline(cin, isbn);
                library.createBorrowRequest(user, isbn);
                break;
            case 4:
                cout << "Masukkan ID User: ";
                getline(cin, user);
                cout << "Masukkan ISBN buku: ";
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
                cout << "Masukkan ISBN buku yang Anda suka: ";
                getline(cin, isbn);
                library.getRecommendations(isbn);
                break;
            case 0:
                cout << "Menyimpan data...\n";
                library.saveDatabaseToFile(dbFilename);
                cout << "Sampai jumpa!\n";
                break;
            default:
                cout << "Pilihan tidak valid.\n";
        }
    } while (choice != 0);

    return 0;
}

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;


struct J {
    enum Tipe { KOSONG, STR, ARR, OBJ } tipe = KOSONG;
    string s;
    vector<J> a;          
    vector<string> kunci; 
    vector<J> nilai;      

    const J* ambil(const string& k) const {
        for (size_t i = 0; i < kunci.size(); i++)
            if (kunci[i] == k) return &nilai[i];
        return nullptr;
    }
};

struct PembacaJSON {
    const string& x;
    size_t i = 0;
    explicit PembacaJSON(const string& teks) : x(teks) {}

    [[noreturn]] void salah(const string& pesan) {
        throw runtime_error("JSON salah di posisi " + to_string(i) + ": " + pesan);
    }
    void lewati() {
        while (i < x.size() && isspace(static_cast<unsigned char>(x[i]))) i++;
    }
    string teks() {
        if (x[i] != '"') salah("diharapkan tanda kutip");
        i++;
        string r;
        while (i < x.size() && x[i] != '"') {
            if (x[i] == '\\') {
                i++;
                if (i >= x.size()) salah("escape terputus");
                char c = x[i];
                r += (c == 'n') ? '\n' : (c == 't') ? '\t' : c;
            } else {
                r += x[i];
            }
            i++;
        }
        if (i >= x.size()) salah("string tidak ditutup");
        i++;
        return r;
    }
    J nilaiJSON() {
        lewati();
        if (i >= x.size()) salah("data berakhir terlalu cepat");
        J v;
        if (x[i] == '"') {
            v.tipe = J::STR;
            v.s = teks();
        } else if (x[i] == '[') {
            v.tipe = J::ARR;
            i++;
            lewati();
            if (x[i] == ']') { i++; return v; }
            for (;;) {
                v.a.push_back(nilaiJSON());
                lewati();
                if (x[i] == ',') { i++; continue; }
                if (x[i] == ']') { i++; break; }
                salah("diharapkan , atau ]");
            }
        } else if (x[i] == '{') {
            v.tipe = J::OBJ;
            i++;
            lewati();
            if (x[i] == '}') { i++; return v; }
            for (;;) {
                lewati();
                string k = teks();
                lewati();
                if (x[i] != ':') salah("diharapkan :");
                i++;
                v.kunci.push_back(k);
                v.nilai.push_back(nilaiJSON());
                lewati();
                if (x[i] == ',') { i++; continue; }
                if (x[i] == '}') { i++; break; }
                salah("diharapkan , atau }");
            }
        } else {
            salah("nilai tidak dikenal (hanya string, array, object)");
        }
        return v;
    }
};


struct Aturan {
    string id, mode, maka;
    vector<string> jika, sumber;
};

struct BasisPengetahuan {
    string judul, batas;
    map<string, string> sumber;
    vector<string> kesimpulanAkhir;
    vector<Aturan> aturan;
};

static vector<string> daftarTeks(const J* j, const string& nama) {
    if (!j || j->tipe != J::ARR) throw runtime_error("'" + nama + "' harus berupa array");
    vector<string> r;
    for (const J& e : j->a) {
        if (e.tipe != J::STR) throw runtime_error("isi '" + nama + "' harus teks");
        r.push_back(e.s);
    }
    return r;
}

static string teksWajib(const J& o, const string& k) {
    const J* j = o.ambil(k);
    if (!j || j->tipe != J::STR) throw runtime_error("field teks '" + k + "' tidak ada");
    return j->s;
}

BasisPengetahuan muatBasis(const string& berkas) {
    ifstream f(berkas);
    if (!f) throw runtime_error("tidak bisa membuka file: " + berkas);
    stringstream ss;
    ss << f.rdbuf();
    string isi = ss.str();

    J akar = PembacaJSON(isi).nilaiJSON();
    if (akar.tipe != J::OBJ) throw runtime_error("isi file harus berupa object JSON");

    BasisPengetahuan kb;
    kb.judul = teksWajib(akar, "judul");
    kb.batas = teksWajib(akar, "batas");

    const J* sm = akar.ambil("sumber");
    if (!sm || sm->tipe != J::OBJ) throw runtime_error("field 'sumber' harus object");
    for (size_t i = 0; i < sm->kunci.size(); i++) kb.sumber[sm->kunci[i]] = sm->nilai[i].s;

    kb.kesimpulanAkhir = daftarTeks(akar.ambil("kesimpulan_akhir"), "kesimpulan_akhir");

    const J* ar = akar.ambil("aturan");
    if (!ar || ar->tipe != J::ARR) throw runtime_error("field 'aturan' harus array");
    for (const J& o : ar->a) {
        Aturan r;
        
        
        r.id = teksWajib(o, "id");
        r.mode = teksWajib(o, "mode");
        r.maka = teksWajib(o, "maka");
        r.jika = daftarTeks(o.ambil("jika"), "jika");
        r.sumber = daftarTeks(o.ambil("sumber"), "sumber");
        if (r.mode != "dan" && r.mode != "atau")
            throw runtime_error("mode aturan " + r.id + " harus 'dan' atau 'atau'");
        if (r.jika.empty()) throw runtime_error("aturan " + r.id + " tidak punya syarat");
        for (const string& s : r.sumber)
            if (!kb.sumber.count(s))
                throw runtime_error("aturan " + r.id + " memakai sumber tak dikenal: " + s);
        kb.aturan.push_back(r);
    }
    return kb;
}


static string gabung(const vector<string>& v, const string& pemisah) {
    string r;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) r += pemisah;
        r += v[i];
    }
    return r;
}

static string sumberTeks(const Aturan& r) { return "[" + gabung(r.sumber, ", ") + "]"; }


static bool tanya(const string& fakta, int no) {
    bool interaktif = isatty(0);
    for (;;) {
        cout << no << ". Apakah " << fakta << "? (y/t) > " << flush;
        string jawab;
        if (!getline(cin, jawab)) {
            if (!interaktif) cout << "t";
            cout << "\n";
            return false;
        }
        size_t a = jawab.find_first_not_of(" \t\r");
        size_t b = jawab.find_last_not_of(" \t\r");
        jawab = (a == string::npos) ? "" : jawab.substr(a, b - a + 1);
        transform(jawab.begin(), jawab.end(), jawab.begin(), ::tolower);
        if (!interaktif) cout << jawab << "\n";
        if (jawab == "y" || jawab == "ya") return true;
        if (jawab == "t" || jawab == "tidak") return false;
        cout << "   Jawab dengan y atau t.\n";
    }
}


int main(int argc, char** argv) {
    string berkas = (argc > 1) ? argv[1] : "kb_banjir_longsor.json";

    BasisPengetahuan kb;
    try {
        kb = muatBasis(berkas);
    } catch (const exception& e) {
        cerr << "Gagal membaca basis pengetahuan: " << e.what() << "\n";
        return 1;
    }

   
    set<string> hasilAturan;
    for (const Aturan& r : kb.aturan) hasilAturan.insert(r.maka);
    vector<string> pertanyaan;
    set<string> sudah;
    for (const Aturan& r : kb.aturan)
        for (const string& c : r.jika)
            if (!hasilAturan.count(c) && !sudah.count(c)) {
                sudah.insert(c);
                pertanyaan.push_back(c);
            }

    cout << kb.judul << "\n";
    cout << "Jawab y/t.\n";

    set<string> memori;           
    vector<string> fakta_awal;
    int no = 1;
    for (const string& f : pertanyaan)
        if (tanya(f, no++)) {
            memori.insert(f);
            fakta_awal.push_back(f);
        }

    cout << "\nFakta awal: " << (fakta_awal.empty() ? "(tidak ada)" : gabung(fakta_awal, ", ")) << "\n\n";

    auto terpenuhi = [&](const Aturan& r) {
        if (r.mode == "atau") {
            for (const string& c : r.jika)
                if (memori.count(c)) return true;
            return false;
        }
        for (const string& c : r.jika)
            if (!memori.count(c)) return false;
        return true;
    };

    map<string, int> asal;     
    vector<string> turunan;     
    for (int siklus = 1;; siklus++) {
        int pilih = -1;
        for (size_t i = 0; i < kb.aturan.size(); i++)
            if (!memori.count(kb.aturan[i].maka) && terpenuhi(kb.aturan[i])) {
                pilih = static_cast<int>(i);
                break; 
            }
        if (pilih < 0) {
            cout << "Siklus " << siklus << ": tidak ada aturan baru, berhenti\n";
            break;
        }
        const Aturan& r = kb.aturan[pilih];
        memori.insert(r.maka);
        asal[r.maka] = pilih;
        turunan.push_back(r.maka);
        cout << "Siklus " << siklus << ": " << r.id << " -> " << r.maka << "\n";
    }

   
    set<string> akhir(kb.kesimpulanAkhir.begin(), kb.kesimpulanAkhir.end());
    vector<string> kesimpulan, antara;
    for (const string& t : turunan) (akhir.count(t) ? kesimpulan : antara).push_back(t);

    
    function<void(const string&, int)> alasan = [&](const string& f, int dalam) {
        const Aturan& r = kb.aturan[asal[f]];
        vector<string> benar;
        for (const string& c : r.jika)
            if (memori.count(c)) benar.push_back(c);
        cout << string(2 + dalam * 2, ' ') << f << " <- " << r.id << " (" << gabung(benar, ", ") << ") "
             << sumberTeks(r) << "\n";
        for (const string& c : benar)
            if (asal.count(c)) alasan(c, dalam + 1);
    };

    cout << "\n";
    if (kesimpulan.empty()) {
        cout << "Kesimpulan: tidak ada kesimpulan yang dapat ditarik dari jawaban tersebut.\n";
        if (!antara.empty()) {
            cout << "Hasil antara yang sempat diperoleh:\n";
            for (const string& t : antara) cout << "  " << t << " <- " << kb.aturan[asal[t]].id << "\n";
        }
    } else {
        cout << "Kesimpulan:\n";
        for (const string& k : kesimpulan) cout << "  - " << k << "\n";
        cout << "\nAlasan:\n";
        for (const string& k : kesimpulan) {
            alasan(k, 0);
            cout << "\n";
        }
    }

    set<string> dipakai;
    for (const string& t : turunan)
        for (const string& s : kb.aturan[asal[t]].sumber) dipakai.insert(s);
    if (!dipakai.empty()) {
        cout << "Sumber yang dipakai:\n";
        for (const string& s : dipakai) cout << "  " << s << ": " << kb.sumber[s] << "\n";
        cout << "\n";
    }
    cout << "Catatan: " << kb.batas << "\n";
    return 0;
}

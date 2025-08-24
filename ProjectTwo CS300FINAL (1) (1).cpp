// ProjectTwo.cpp
// ABCU Advising file C++)
// - Kayden Sysum
// - What the system does- 
// - loads a csv of courses
// - store them in a BST which is keyed by course number
// - prints a sorted list
// - show details for a single course (with prereq numbers + titles)
//
// note: no external CSV lib.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// -------- helpers --------

static inline std::string trim(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && std::isspace((unsigned char)s[i])) ++i;
    while (j > i && std::isspace((unsigned char)s[j - 1])) --j;
    return s.substr(i, j - i);
}

static inline std::string up(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return (char)std::toupper(c); });
    return s;
}

// comma split (simple, works fine for our input)
static inline std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) out.push_back(trim(cell));
    return out;
}

// -------- data --------

struct Course {
    std::string num;
    std::string title;
    std::vector<std::string> prereqs;
};

struct Node {
    Course course;
    Node* left  = nullptr;
    Node* right = nullptr;
    explicit Node(const Course& c) : course(c) {}
};

class CourseBST {
public:
    ~CourseBST() { clear(root); }

    void insert(const Course& c) { root = insertRec(root, c); }

    const Course* find(const std::string& key) const {
        Node* cur = root;
        while (cur) {
            if (key == cur->course.num) return &cur->course;
            cur = (key < cur->course.num) ? cur->left : cur->right;
        }
        return nullptr;
    }

    template <typename F>
    void inorder(F&& f) const { walk(root, f); }

    void reset() { clear(root); root = nullptr; }

private:
    Node* root = nullptr;

    static Node* insertRec(Node* n, const Course& c) {
        if (!n) return new Node(c);
        if (c.num == n->course.num) {
            n->course = c;              // update on duplicate key
        } else if (c.num < n->course.num) {
            n->left = insertRec(n->left, c);
        } else {
            n->right = insertRec(n->right, c);
        }
        return n;
    }

    template <typename F>
    static void walk(Node* n, F&& f) {
        if (!n) return;
        walk(n->left, f);
        f(n->course);
        walk(n->right, f);
    }

    static void clear(Node* n) {
        if (!n) return;
        clear(n->left);
        clear(n->right);
        delete n;
    }
};

struct Catalog {
    CourseBST bst;
    std::unordered_map<std::string, const Course*> ix; // num -> ptr (for prereq title lookup)
    bool loaded = false;
};

// -------- io & features --------

static bool loadCatalog(const std::string& path, Catalog& cat, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "couldn't open file: " + path; return false; }

    cat.bst.reset();
    cat.ix.clear();

    struct Row { std::string num, title; std::vector<std::string> prereqs; };
    std::vector<Row> rows;
    std::string line;
    size_t lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        auto trimmed = trim(line);
        if (trimmed.empty()) continue;

        auto parts = splitCSV(trimmed);
        if (parts.size() < 2) { err = "bad line " + std::to_string(lineNo); return false; }

        Row r;
        r.num = up(parts[0]);
        r.title = parts[1];
        for (size_t i = 2; i < parts.size(); ++i) r.prereqs.push_back(up(parts[i]));
        rows.push_back(std::move(r));
    }

    for (const auto& r : rows) {
        Course c;
        c.num = r.num;
        c.title = r.title;
        c.prereqs = r.prereqs;
        cat.bst.insert(c);
    }

    cat.ix.clear();
    cat.bst.inorder([&](const Course& c){ cat.ix[c.num] = &c; });

    cat.loaded = true;
    std::cout << "loaded " << rows.size() << " course(s) from " << path << "\n";
    return true;
}

static void listCourses(const Catalog& cat) {
    if (!cat.loaded) { std::cout << "load data first (option 1)\n"; return; }
    std::cout << "\ncourse list (a→z):\n";
    cat.bst.inorder([&](const Course& c){
        std::cout << c.num << ", " << c.title << "\n";
    });
    std::cout << "\n";
}

static void showCourse(const Catalog& cat, const std::string& raw) {
    if (!cat.loaded) { std::cout << "load data first (option 1)\n"; return; }
    std::string key = up(trim(raw));
    if (key.empty()) { std::cout << "no course entered\n"; return; }

    const Course* c = cat.bst.find(key);
    if (!c) { std::cout << "not found: " << key << "\n"; return; }

    std::cout << c->num << ", " << c->title << "\n";
    if (c->prereqs.empty()) { std::cout << "prerequisites: none\n\n"; return; }

    std::cout << "prerequisites: ";
    for (size_t i = 0; i < c->prereqs.size(); ++i) {
        const auto& p = c->prereqs[i];
        auto it = cat.ix.find(p);
        if (it != cat.ix.end() && it->second) {
            std::cout << p << " (" << it->second->title << ")";
        } else {
            std::cout << p;
        }
        if (i + 1 < c->prereqs.size()) std::cout << ", ";
    }
    std::cout << "\n\n";
}

// -------- menu --------

static void menu() {
    std::cout << "\n1) load data\n"
                 "2) print course list\n"
                 "3) print one course\n"
                 "9) exit\n"
                 "choose: ";
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Catalog cat;
    std::cout << "welcome to the course planner.\n";

    bool on = true;
    while (on) {
        menu();
        std::string line;
        if (!std::getline(std::cin, line)) break;
        line = trim(line);

        int opt = 0;
        if (!line.empty() && std::all_of(line.begin(), line.end(),
                                         [](unsigned char c){ return std::isdigit(c); })) {
            opt = std::stoi(line);
        }

        switch (opt) {
            case 1: {
                std::cout << "enter filename: ";
                std::string path;
                if (!std::getline(std::cin, path)) break;
                path = trim(path);
                if (path.empty()) { std::cout << "no filename provided\n"; break; }
                std::string err;
                if (!loadCatalog(path, cat, err)) std::cout << err << "\n";
                break;
            }
            case 2:
                listCourses(cat);
                break;
            case 3: {
                std::cout << "course number: ";
                std::string num;
                if (!std::getline(std::cin, num)) break;
                showCourse(cat, num);
                break;
            }
            case 9:
                std::cout << "goodbye!\n";
                on = false;
                break;
            default:
                std::cout << (line.empty() ? "no option entered" : ("'" + line + "' isn't a valid option")) << "\n";
                break;
        }
    }
    return 0;
}

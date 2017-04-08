#ifndef TYPING_H
#define TYPING_H

#include <locale>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "common.h"

typedef long long LL;

class Typing
{
public:
    Typing(std::locale coding, std::string DATA_PATH, int Q = 2, double lamda = 0.9);
    ~Typing();

    void Init();
    void Check();
    /// return true if find solution
    bool Solve(std::vector<std::wstring> pinyins, std::vector<std::wstring>& solution) const;

private:
    const std::locale coding;
    const std::string DATA_PATH;
    const int Q;
    const double lamda;

    std::set<wchar_t> letters; // 字
    std::map<std::wstring, std::vector<std::wstring>> pinyin2letters;
    std::vector<std::vector<std::pair<std::wstring, LL>>> wordscount;
    LL total_database_words;

    std::set<wchar_t> getLetters();
    std::map<std::wstring, std::vector<std::wstring> > getPinyin2Letters(const std::set<wchar_t>& letters);
    std::vector<std::pair<std::wstring, LL> > getWordCount(const std::set<wchar_t>& letters, int p);
    void saveWordCount(const std::vector<std::pair<std::wstring, LL> >& word_count, const std::string& filename);
    std::vector<std::pair<std::wstring, LL> > loadWordCount(const std::string& filename);
    bool isFileExists(const std::string& filename);
    int C(const std::wstring& X) const;
    double Pw(const std::wstring& A, const std::wstring& B) const;
};

#endif // TYPING_H
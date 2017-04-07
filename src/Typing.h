#ifndef TYPING_H
#define TYPING_H

#include <locale>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "common.h"

class Typing
{
public:
    Typing(std::locale coding, std::string DATA_PATH, int Q = 2, double lamda = 0.9);
    ~Typing();
    const double FILTER = 0.7;

    void Init();
    void Check();
    double Solve(std::vector<std::wstring> pinyins, std::vector<std::wstring>& solution) const;

private:
    std::locale coding;
    const std::string DATA_PATH;
    const int Q;
    const double lamda;
    std::map<std::wstring, std::vector<std::wstring> > pinyin2words;
    std::set<wchar_t> words; // 字
    std::set<std::wstring> ci_set;
    std::vector<std::vector<std::pair<std::wstring, int> > > wordscount;
    long long total_database_words;

    std::set<wchar_t> getWords();
    std::set<std::wstring> getCi(const std::set<wchar_t>& words);
    std::map<std::wstring, std::vector<std::wstring> > getPinyin2Words(const std::set<wchar_t>& words);
    std::vector<std::pair<std::wstring, int> > getWordCount(const std::set<wchar_t>& words, int p);
    void saveWordCount(const std::vector<std::pair<std::wstring, int> >& word_count, const std::string& filename);
    std::vector<std::pair<std::wstring, int> > loadWordCount(const std::string& filename);
    bool isFileExists(const std::string& filename);
    int C(const std::wstring& X) const;
    double Pw(const std::wstring& A, const std::wstring& B) const;
};

#endif // TYPING_H
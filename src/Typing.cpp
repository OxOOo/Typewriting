#include "Typing.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <mutex>
#include <thread>
#include <cstdlib>

using namespace std;

void possibleList(int n, const vector<int>& sizes, vector<vector<int> >& indexes)
{
    indexes.clear();

    if (n == 0) {
        return;
    } if (n == 1) {
        for(int a = 0; a < sizes[0]; a ++)
        {
            vector<int> list = {a};
            indexes.push_back(list);
        }
        return;
    } else if (n == 2) {
        for(int a = 0; a < sizes[0]; a ++)
            for(int b = 0; b < sizes[1]; b ++)
            {
                vector<int> list = {a, b};
                indexes.push_back(list);
            }
        return;
    } else if (n == 3) {
        for(int a = 0; a < sizes[0]; a ++)
            for(int b = 0; b < sizes[1]; b ++)
                for(int c = 0; c < sizes[2]; c ++)
                {
                    vector<int> list = {a, b, c};
                    indexes.push_back(list);
                }
        return;
    }
    assert(false);
}

void splitRange(int S, int T, int n, vector<int>& rangeS, vector<int>& rangeT) // [S, T)
{
    assert(T - S >= n);
    rangeS.clear();
    rangeT.clear();
    for(long long i = 0; i < n; i ++)
    {
        rangeS.push_back(i*(T-S)/n+S);
    }
    for(long long i = 1; i <= n; i ++)
    {
        rangeT.push_back(i*(T-S)/n+S);
    }
}

Typing::Typing(locale coding, string DATA_PATH, int Q, double lamda): coding(coding), DATA_PATH(DATA_PATH), Q(Q), lamda(lamda)
{
    assert(Q > 0);
    assert(0 <= lamda && lamda <= 1);
}

Typing::~Typing()
{

}

void Typing::Init()
{
    letters = getLetters();
    pinyin2letters = getPinyin2Letters(letters);

    wordscount.resize(1);
    for(int q = 1; q <= Q; q ++)
    {
        char buf[BUFFER_SIZE];
        sprintf(buf, "word%dcount.txt", q);
        if (isFileExists(buf)) {
            auto count = loadWordCount(buf);
            wordscount.push_back(count);
        } else {
            auto count = getWordCount(letters, q);
            saveWordCount(count, buf);
            wordscount.push_back(count);
        }
    }

    if (Q >= 2) {
        auto it = copy_if(wordscount[2].begin(), wordscount[2].end(), wordscount[2].begin(), [](pair<wstring, int> x) {
            return x.second > 10;
        });
        wordscount[2].resize(std::distance(wordscount[2].begin(),it));
    }
    if (Q >= 3) {
        auto it = copy_if(wordscount[3].begin(), wordscount[3].end(), wordscount[3].begin(), [](pair<wstring, int> x) {
            return x.second > 10;
        });
        wordscount[3].resize(std::distance(wordscount[3].begin(),it));
    }

    total_database_words = 0;
    for(auto p: wordscount[1])
        total_database_words += p.second;
}

void Typing::Check()
{
    assert(Q >= 1);

    if (Q >= 1) assert(4256692 <= C(L"中"));
    if (Q >= 2) assert(3302 <= C(L"取通"));
    if (Q >= 3) assert(1710 <= C(L"一一个"));
}

bool Typing::Solve(std::vector<std::wstring> pinyins, std::vector<std::wstring>& solution) const
{
    solution.clear();
    for(int i = 0; i < (int)pinyins.size(); i ++)
        transform(pinyins[i].begin(), pinyins[i].end(), pinyins[i].begin(), ::tolower);
    if (pinyins.size() == 0) return false;

    vector<vector<wstring> > choice;
    vector<vector<double> > DP;
    vector<vector<int> > from;

    choice.resize(pinyins.size());
    DP.resize(pinyins.size());
    from.resize(pinyins.size());
    for(int i = 0; i < (int)pinyins.size(); i ++)
    {
        if (pinyin2letters.find(pinyins[i]) == pinyin2letters.end() || pinyin2letters.at(pinyins[i]).size() == 0) return false;
        choice[i] = pinyin2letters.at(pinyins[i]);
        DP[i].resize(choice[i].size());
        from[i].resize(choice[i].size());
    }

    for(int i = 0; i < (int)choice.size(); i ++)
    {
        for(int j = 0; j < (int)choice[i].size(); j ++)
        {
            DP[i][j] = 0;
            from[i][j] = 0;
            if (i == 0) DP[i][j] = C(choice[i][j])*lamda*lamda;
            else {
                for(int S = max(0, i-Q+1), T = i-1; S <= T; S++)
                {
                    vector<int> sizes;
                    vector<vector<int> > indexes;

                    for(int k = S; k <= T; k ++)
                        sizes.push_back(choice[k].size());
                    possibleList(sizes.size(), sizes, indexes);

                    for(int p = 0; p < (int)indexes.size(); p ++)
                    {
                        double sum = 1.0;
                        double P = 0;

                        for(int k1 = S; k1 <= T; k1 ++)
                        {
                            wstring gram;
                            for(int k = k1; k <= T; k ++)
                            {
                                gram += choice[k][indexes[p][k-S]];
                            }
                            P += Pw(gram+choice[i][j], gram)*lamda*sum;
                            sum *= 1-lamda;
                        }
                        P += C(choice[i][j])*sum/double(total_database_words);

                        // FIXME 只取了最后一个
                        for(int k = T; k <= T; k ++)
                        {
                            P *= DP[k][indexes[p][k-S]];
                        }

                        if (DP[i][j] < P)
                        {
                            DP[i][j] = P;
                            from[i][j] = indexes[p][sizes.size()-1];
                        }
                    }

                    break;
                }
            }
        }
    }
    
    double max_score = -1;
    int pos = -1;

    for(int j = 0; j < (int)choice[choice.size()-1].size(); j ++)
        if (DP[choice.size()-1][j] > max_score)
        {
            max_score = DP[choice.size()-1][j];
            pos = j;
        }
    if (pos < 0) return false;
    
    for(int i = choice.size()-1; i >= 0; i --)
    {
        solution.push_back(choice[i][pos]);
        pos = from[i][pos];
    }
    reverse(solution.begin(), solution.end());

    return true;
}

set<wchar_t> Typing::getLetters()
{
    set<wchar_t> letters;
    wifstream file(DATA_PATH + "letter_list.txt");
    file.imbue(coding);

    wstring line;
    while(!file.eof())
    {
        getline(file, line);

        for(int i = 0; i < (int)line.length(); i ++)
            if (!iswspace(line[i]))
                letters.insert(line[i]);
    }

    return letters;
}

map<wstring, vector<wstring> > Typing::getPinyin2Letters(const std::set<wchar_t>& letters)
{
    map<wstring, vector<wstring> > pinyin2letters;
    wifstream file(DATA_PATH + "dict.txt");
    file.imbue(coding);

    wstring line;
    while(!file.eof())
    {
        getline(file, line);

        wistringstream ss(line);
        wstring pinyin, letter;
        vector<wstring> letter_list;

        ss >> pinyin;
        if (pinyin.length() == 0) continue;
        while(ss >> letter) {
            if (letter.length() != 1 || letters.find(letter[0]) == letters.end()) continue;
            letter_list.push_back(letter);
        }
        transform(pinyin.begin(), pinyin.end(), pinyin.begin(), ::tolower);
        pinyin2letters[pinyin] = letter_list;
    }

    return pinyin2letters;
}

std::vector<std::pair<std::wstring, LL> > Typing::getWordCount(const set<wchar_t>& letters, int p)
{
    wifstream file(DATA_PATH + "news.txt");
    file.imbue(coding);

    vector<wstring> lines;
    while(!file.eof())
    {
        wstring line;
        getline(file, line);
        lines.push_back(line);
    }

    thread threads[THREADS];
    map<wstring, LL> temp_rst[THREADS];
    int id = 0;
    mutex mut;
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k] = thread([&](int range_index) {
            while(true)
            {
                mut.lock();
                int index = id ++;
                mut.unlock();
                if (index >= (int)lines.size()) break;

                wstring line = lines[index];
                vector<bool> isok;
                isok.resize(line.length());

                for(int pos = 0; pos < (int)line.length(); pos ++)
                {
                    isok[pos] = (letters.find(line[pos]) != letters.end());
                }

                for(int pos = 0; pos < (int)line.length(); pos ++)
                {
                    wstring gram = L"";
                    bool flag = true;
                    for(int i = 0; i < p && flag; i ++)
                    {
                        if (pos + i >= (int)line.length()) flag = false;
                        else if (!isok[pos+i]) flag = false;
                        else gram += line[pos+i];
                    }
                    if (!flag) continue;

                    if (temp_rst[range_index].find(gram) == temp_rst[range_index].end()) temp_rst[range_index][gram] = 1;
                    else temp_rst[range_index][gram] ++;
                }
            }
        }, k);
    }
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k].join();
    }

    lines = vector<wstring>();

    map<wstring, LL> rst;
    for(int k = 0; k < THREADS; k ++)
    {
        for(auto p: temp_rst[k])
        {
            if (rst.find(p.first) == rst.end())
                rst[p.first] = p.second;
            else rst[p.first] += p.second;
        }
        temp_rst[k] = map<wstring, LL>();
    }

    vector<pair<wstring, LL> > count;
    for(auto p: rst)
    {
        count.push_back(make_pair(p.first, p.second));
    }

    return count;
}

void Typing::saveWordCount(const std::vector<std::pair<std::wstring, LL> >& word_count, const string& filename)
{
    wofstream file(DATA_PATH + filename);
    file.imbue(coding);

    for(auto p: word_count)
        file << p.first << L" " << p.second << endl;
}

std::vector<std::pair<std::wstring, LL> > Typing::loadWordCount(const string& filename)
{
    wifstream file(DATA_PATH + filename);
    file.imbue(coding);

    vector<wstring> lines;
    while(!file.eof())
    {
        wstring line;
        getline(file, line);
        if (line.length() < 2) continue;
        lines.push_back(line);
    }

    thread threads[THREADS];
    vector<pair<wstring, LL> > temp_list[THREADS];
    vector<int> rangeS, rangeT;
    splitRange(0, lines.size(), THREADS, rangeS, rangeT);

    for(int k = 0; k < THREADS; k ++)
    {
        threads[k] = thread([&](int range_index) {
            const int S = rangeS[range_index];
            const int T = rangeT[range_index];

            for(int index = S; index < T; index ++)
            {
                wstring line = lines[index];
                while(line.find(L",") != wstring::npos)
                    line = line.replace(line.find(L","), 1, L"");

                wistringstream ss(line);
                wstring gram;
                LL count;
                ss >> gram >> count;

                temp_list[range_index].push_back(make_pair(gram, count));
            }

        }, k);
    }
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k].join();
    }

    vector<pair<wstring, LL> > list;
    list.reserve(lines.size());
    for(int i = 0; i < THREADS; i ++)
    {
        list.insert(list.end(), temp_list[i].begin(), temp_list[i].end());
    }

    return list;
}

bool Typing::isFileExists(const std::string& filename)
{
    FILE *fd = fopen((DATA_PATH + filename).c_str(), "rb");
    if (fd) {
        fclose(fd);
        return true;
    }
    return false;
}

int Typing::C(const std::wstring& X) const
{
    auto x = lower_bound(wordscount[X.length()].begin(), wordscount[X.length()].end(), make_pair(X, LL(0)));
    if (x->first != X) return 0;
    return x->second;
}

double Typing::Pw(const std::wstring& A, const std::wstring& B) const
{
    int ca = C(A), cb = C(B);
    if (ca == 0) return 0.0;
    return double(ca)/double(cb);
}
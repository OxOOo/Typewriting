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
    words = getWords();
    pinyin2words = getPinyin2Words(words);
    ci_set = getCi(words);
    cout << "ci_set.size() " << ci_set.size() << endl;

    wordscount.resize(1);
    for(int q = 1; q <= Q; q ++)
    {
        char buf[BUFFER_SIZE];
        sprintf(buf, "word%dcount.txt", q);
        if (isFileExists(buf)) {
            auto count = loadWordCount(buf);
            wordscount.push_back(count);
        } else {
            auto count = getWordCount(words, q);
            saveWordCount(count, buf);
            wordscount.push_back(count);
        }
    }

    if (Q >= 2) {
        auto it = copy_if(wordscount[2].begin(), wordscount[2].end(), wordscount[2].begin(), [](pair<wstring, int> x) {
            return x.second > 10;
        });
        wordscount[2].resize(std::distance(wordscount[2].begin(),it));
        cout << "wordscount[2].size() " << wordscount[2].size() << " " << wordscount[2][0].second << endl;
    }
    if (Q >= 3) {
        auto it = copy_if(wordscount[3].begin(), wordscount[3].end(), wordscount[3].begin(), [](pair<wstring, int> x) {
            return x.second > 10;
        });
        wordscount[3].resize(std::distance(wordscount[3].begin(),it));
        cout << "wordscount[3].size() " << wordscount[3].size() << " " << wordscount[3][0].second << endl;
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

double Typing::Solve(std::vector<std::wstring> pinyins, std::vector<std::wstring>& solution) const
{
    solution.clear();
    for(int i = 0; i < (int)pinyins.size(); i ++)
        transform(pinyins[i].begin(), pinyins[i].end(), pinyins[i].begin(), ::tolower);

    vector<vector<wstring> > choice;
    vector<vector<double> > DP;
    vector<vector<int> > from;

    assert(pinyins.size() > 0);
    choice.resize(pinyins.size());
    DP.resize(pinyins.size());
    from.resize(pinyins.size());
    for(int i = 0; i < (int)pinyins.size(); i ++)
    {
        if (pinyin2words.find(pinyins[i]) == pinyin2words.end()) wcout << L"assert fail " << pinyins[i] << endl;
        assert(pinyin2words.find(pinyins[i]) != pinyin2words.end());
        choice[i] = pinyin2words.at(pinyins[i]);
        DP[i].resize(choice[i].size());
        from[i].resize(choice[i].size());
        assert(choice[i].size() > 0);
    }

    for(int i = 0; i < (int)choice.size(); i ++)
    {
        vector<int> sizes;
        vector<vector<int> > indexes;

        const int S = max(0, i-Q+1);
        const int T = i-1;

        for(int k = S; k <= T; k ++)
            sizes.push_back(choice[k].size());
        possibleList(sizes.size(), sizes, indexes);

        for(int j = 0; j < (int)choice[i].size(); j ++)
        {
            DP[i][j] = 0;
            from[i][j] = 0;
            if (i == 0) DP[i][j] = C(choice[i][j]);
            else {
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

                        // if (ci_set.find(gram + choice[i][j]) != ci_set.end()) P += C(gram + choice[i][j])/double();
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
    assert(pos >= 0);
    
    for(int i = choice.size()-1; i >= 0; i --)
    {
        solution.push_back(choice[i][pos]);
        pos = from[i][pos];
    }
    reverse(solution.begin(), solution.end());

    return max_score;
}

map<wstring, vector<wstring> > Typing::getPinyin2Words(const std::set<wchar_t>& words)
{
    map<wstring, vector<wstring> > pinyin2words;
    wifstream file(DATA_PATH + "dict.txt");
    file.imbue(coding);

    wstring line;
    while(!file.eof())
    {
        getline(file, line);

        wistringstream ss(line);
        wstring pinyin, word;
        vector<wstring> word_list;

        ss >> pinyin;
        if (pinyin.length() == 0) continue;
        while(ss >> word) {
            if (word.length() != 1 || words.find(word[0]) == words.end()) continue;
            word_list.push_back(word);
        }
        transform(pinyin.begin(), pinyin.end(), pinyin.begin(), ::tolower);
        pinyin2words[pinyin] = word_list;
    }

    return pinyin2words;
}

set<wchar_t> Typing::getWords()
{
    set<wchar_t> words;
    wifstream file(DATA_PATH + "word_list.txt");
    file.imbue(coding);

    wstring line;
    while(!file.eof())
    {
        getline(file, line);

        for(int i = 0; i < (int)line.length(); i ++)
            if (!iswspace(line[i]))
                words.insert(line[i]);
    }

    return words;
}

std::set<std::wstring> Typing::getCi(const set<wchar_t>& words)
{
    set<wstring> ci;
    wifstream file(DATA_PATH + "ci.txt");
    file.imbue(coding);

    wstring line;
    while(!file.eof())
    {
        getline(file, line);

        wistringstream ss(line);
        wstring entry;
        while(ss >> entry)
        {
            bool flag = true;
            for(int i = 0; i < (int)entry.length() && flag; i ++)
                if(words.find(entry[i]) == words.end())
                    flag = false;
            if (flag) ci.insert(entry);
        }
    }

    return ci;
}

std::vector<std::pair<std::wstring, int> > Typing::getWordCount(const set<wchar_t>& words, int p)
{
    double filter = 1.0;
    for(int x = 1; x < p; x ++) filter *= FILTER;

    wifstream file(DATA_PATH + "news.txt");
    file.imbue(coding);

    cout << "getWordCount " << p << endl;
    vector<wstring> lines;
    while(!file.eof())
    {
        wstring line;
        getline(file, line);
        lines.push_back(line);
    }

    cout << "total lines : " << lines.size() << endl;

    thread threads[THREADS];
    map<wstring, int> temp_rst[THREADS];
    vector<int> rangeS, rangeT;
    splitRange(0, lines.size(), THREADS, rangeS, rangeT);
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k] = thread([&](int range_index) {
            const int S = rangeS[range_index], T = rangeT[range_index];
            cout << "thread " << range_index << " " << S << " " << T << endl;

            for(int index = S; index < T; index ++)
            {
                wstring line = lines[index];
                vector<bool> isok;
                isok.resize(line.length());

                for(int pos = 0; pos < (int)line.length(); pos ++)
                {
                    isok[pos] = (words.find(line[pos]) != words.end());
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
        cout << "start threads " << k << endl;
    }
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k].join();
    }
    cout << "end threads " << endl;

    map<wstring, int> rst;
    for(int k = 0; k < THREADS; k ++)
    {
        for(auto p: temp_rst[k])
        {
            if (rst.find(p.first) == rst.end())
                rst[p.first] = p.second;
            else rst[p.first] += p.second;
        }
    }

    vector<pair<int, wstring> > reversed_list;
    for(auto p: rst)
    {
        reversed_list.push_back(make_pair(p.second, p.first));
    }
    sort(reversed_list.begin(), reversed_list.end(), greater<pair<int, wstring>>());

    vector<pair<wstring, int> > count;
    for(int i = 0, j = reversed_list.size()*filter+1e-5; i < j; i ++)
    {
        count.push_back(make_pair(reversed_list[i].second, reversed_list[i].first));
    }
    sort(count.begin(), count.end());

    return count;
}

void Typing::saveWordCount(const std::vector<std::pair<std::wstring, int> >& word_count, const string& filename)
{
    wofstream file(DATA_PATH + filename);
    file.imbue(coding);

    for(auto p: word_count)
        file << p.first << L" " << p.second << endl;
}

std::vector<std::pair<std::wstring, int> > Typing::loadWordCount(const string& filename)
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
    vector<pair<wstring, int> > temp_list[THREADS];
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
                int count;
                ss >> gram >> count;

                temp_list[range_index].push_back(make_pair(gram, count));
            }

        }, k);
        cout << "start threads " << k << endl;
    }
    for(int k = 0; k < THREADS; k ++)
    {
        threads[k].join();
    }
    cout << "thread end" << endl;

    vector<pair<wstring, int> > list;
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
    auto x = lower_bound(wordscount[X.length()].begin(), wordscount[X.length()].end(), make_pair(X, 0));
    if (x->first != X) return 0;
    return x->second;
}

double Typing::Pw(const std::wstring& A, const std::wstring& B) const
{
    int ca = C(A), cb = C(B);

    if (ca == 0) return 0.0;
    return double(ca)/double(cb);
}
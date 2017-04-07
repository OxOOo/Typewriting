#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <locale>
#include <ctime>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <vector>
#include <mutex>
#include <thread>
#include <sys/time.h>

#include "Typing.h"
#include "common.h"

using namespace std;

#define DATA_PATH "../data/"

locale utf8locale("zh_CN.UTF-8");

map<string, double> clock_points;

void setTimePoint(string tag)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    clock_points[tag] = tv.tv_sec + 1e-6*tv.tv_usec;
}

double logTimePoint(string tag)
{

    if (clock_points.find(tag) != clock_points.end()) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double now = tv.tv_sec + 1e-6*tv.tv_usec;
        cout << "TimePoint(" << tag << "):" << double(now-clock_points[tag]) << " s" << endl;
        return double(now-clock_points[tag]);
    } else {
        cout << "TimePoint(" << tag << "):unknow" << endl;
        return 0.0;
    }
}

Typing typing(utf8locale, DATA_PATH, 3, 0.9);

int main()
{
    ios::sync_with_stdio(false);
    wcout.imbue(utf8locale);

    setTimePoint("typing.Init");
    typing.Init();
    typing.Check();
    double init_time = logTimePoint("typing.Init");

    wifstream input_file(DATA_PATH "input.txt");
    input_file.imbue(utf8locale);
    vector<wstring> input_lines;
    while(!input_file.eof())
    {
        wstring input_line;
        getline(input_file, input_line);
        input_lines.push_back(input_line);
    }
    input_file.close();

    wifstream answer_file(DATA_PATH "answer.txt");
    answer_file.imbue(utf8locale);
    vector<wstring> answer_lines;
    while(!answer_file.eof())
    {
        wstring answer_line;
        getline(answer_file, answer_line);
        answer_lines.push_back(answer_line);
    }
    answer_file.close();
    assert(input_lines.size() == answer_lines.size());

    vector<vector<wstring> > pinyin_list;
    vector<vector<wstring> > output_list;
    pinyin_list.resize(input_lines.size());
    output_list.resize(input_lines.size());
    int id = 0;
    mutex mut;
    thread threads[THREADS];

    setTimePoint("typing.Solve");
    for(auto& t: threads)
    {
        t = thread([&]() {
            while(true)
            {
                mut.lock();
                int index = id ++;
                cout << "processing : " << index << endl;
                mut.unlock();

                if (index >= (int)input_lines.size()) return;

                wistringstream ss(input_lines[index]);
                wstring word;
                pinyin_list[index].clear();
                while(ss >> word)
                {
                    pinyin_list[index].push_back(word);
                }
                if (pinyin_list[index].size() != 0)
                    typing.Solve(pinyin_list[index], output_list[index]);
            }
        });
    }
    for(auto& t: threads)
    {
        t.join();
    }
    double solve_time = logTimePoint("typing.Solve");

    wofstream output_file(DATA_PATH "output.txt");
    output_file.imbue(utf8locale);

    int total_words = 0, correct_words = 0;
    int total_lines = 0, correct_lines = 0;
    for(int index = 0; index < (int)input_lines.size(); index ++)
    {
        auto answer_line = answer_lines[index];
        auto solution = output_list[index];
        auto pinyin = pinyin_list[index];

        if (pinyin.size() == 0) continue;

        if (answer_line.length() != solution.size()) {
            cout << "=========== data error ===============" << endl;
            for(int i = 0; i < (int)pinyin.size(); i ++)
                wcout << pinyin[i] << L" ";
            wcout << endl;
            wcout << answer_line << endl;
        }
        assert(answer_line.length() == solution.size());

        bool flag = true;
        for(int i = 0; i < (int)solution.size(); i ++)
            flag &= solution[i][0] == answer_line[i];
        
        if (!flag)
        {
            cout << "====================" << endl;
            for(int i = 0; i < (int)pinyin.size(); i ++)
                wcout << pinyin[i] << L" ";
            wcout << endl;
            for(wstring word: solution)
                wcout << word << L" ";
            wcout << endl;
            wcout << answer_line << endl;
        }

        // 统计
        for(int i = 0; i < (int)solution.size(); i ++)
        {
            total_words ++;
            if (solution[i][0] == answer_line[i]) correct_words ++;
        }
        total_lines ++;
        if (flag) correct_lines ++;
    }

    cout << "words correct : " << double(correct_words)/double(total_words)*100 << " | " << total_words << endl;
    cout << "lines correct : " << double(correct_lines)/double(total_lines)*100 << " | " << total_lines << endl;
    cout << "typing.Init : " << init_time << " s" << endl;
    cout << "typing.Solve : " << solve_time << " s" << endl;

    return 0;
}
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

#define DATA_PATH "../indexes/"

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
        cout << "耗时(" << tag << "):" << double(now-clock_points[tag]) << " s" << endl;
        return double(now-clock_points[tag]);
    } else {
        cout << "耗时(" << tag << "):未知" << endl;
        return 0.0;
    }
}

wstring strip(wstring input)
{
    int s = 0, t = input.length();
    while(s < t && iswspace(input[s])) s ++;
    while(s < t && iswspace(input[t-1])) t --;
    return input.substr(s, t-s);
}

Typing typing(utf8locale, DATA_PATH, 3, 0.9);

void processFiles(const char* input_filename, const char* output_filename, const char* answer_filename)
{
    wifstream input_file(input_filename);
    if (!input_file) {
        cerr << "无法读取输入文件 : " << input_filename << endl;
        return;
    }

    input_file.imbue(utf8locale);
    vector<wstring> input_lines;
    while(!input_file.eof())
    {
        wstring input_line;
        getline(input_file, input_line);
        input_line = strip(input_line);
        if (input_line.length() == 0) continue;
        input_lines.push_back(input_line);
    }
    input_file.close();

    vector<vector<wstring> > pinyin_list;
    vector<vector<wstring> > output_list;
    vector<bool> output_rst;
    pinyin_list.resize(input_lines.size());
    output_list.resize(input_lines.size());
    output_rst.resize(input_lines.size());
    int id = 0;
    mutex mut;
    thread threads[THREADS];
    cout << "正在尝试搜寻汉字..." << endl;
    setTimePoint("搜寻汉字");
    for(auto& t: threads)
    {
        t = thread([&]() {
            while(true)
            {
                mut.lock();
                int index = id ++;
                mut.unlock();

                if (index >= (int)input_lines.size()) return;

                wistringstream ss(input_lines[index]);
                wstring word;
                pinyin_list[index].clear();
                while(ss >> word)
                {
                    pinyin_list[index].push_back(word);
                }
                output_rst[index] = typing.Solve(pinyin_list[index], output_list[index]);
            }
        });
    }
    for(auto& t: threads)
    {
        t.join();
    }
    logTimePoint("搜寻汉字");

    wofstream output_file(output_filename);
    if (!output_file) {
        cerr << "打开输出文件失败 ：" << output_filename << endl;
        return;
    }

    output_file.imbue(utf8locale);
    for(int index = 0; index < (int)pinyin_list.size(); index ++)
    {
        for(int i = 0; i < (int)pinyin_list[index].size(); i ++)
            output_file << pinyin_list[index][i] << L" ";
        output_file << endl;
        if (output_rst[index]) {
            for(int i = 0; i < (int)output_list[index].size(); i ++)
                output_file << output_list[index][i];
        } else {
            output_file << L"输入法查找失败";
        }
        output_file << endl;
    }
    output_file.close();

    if (!answer_filename) return;

    wifstream answer_file(answer_filename);
    if (!answer_file) {
        cerr << "打开标准文件失败 ：" << answer_filename << endl;
        return;
    }

    answer_file.imbue(utf8locale);
    vector<wstring> answer_lines;
    while(!answer_file.eof())
    {
        wstring answer_line;
        getline(answer_file, answer_line);
        answer_line = strip(answer_line);
        if (answer_line.length() == 0) continue;
        answer_lines.push_back(answer_line);
    }
    answer_file.close();
    if (input_lines.size() != answer_lines.size()) {
        cerr << "输入文件和标准文件的行数不同" << endl;
        return;
    }

    int total_words = 0, correct_words = 0;
    int total_lines = 0, correct_lines = 0;
    for(int index = 0; index < (int)input_lines.size(); index ++)
    {
        auto answer_line = answer_lines[index];
        auto solution = output_list[index];
        auto pinyin = pinyin_list[index];

        if (pinyin.size() == 0 || !output_rst[index]) continue;

        if (answer_line.length() != solution.size()) {
            cerr << "标准答案和得出的汉字数字不符" << endl;
            wcerr << L"拼音：";
            for(int i = 0; i < (int)pinyin.size(); i ++)
                wcerr << pinyin[i] << L" ";
            wcerr << endl;
            wcerr << L"标准答案：" << answer_line << endl;
            wcerr << L"算法得出的汉字：";
            for(int i = 0; i < (int)solution.size(); i ++)
                wcerr << solution[i];
            wcerr << endl;
            continue;
        }

        bool flag = true;
        for(int i = 0; i < (int)solution.size(); i ++)
            flag &= solution[i][0] == answer_line[i];

        // 统计
        for(int i = 0; i < (int)solution.size(); i ++)
        {
            total_words ++;
            if (solution[i][0] == answer_line[i]) correct_words ++;
        }
        total_lines ++;
        if (flag) correct_lines ++;
    }

    cout << "字正确率: " << double(correct_words)/double(total_words)*100 << " 字总数：" << total_words << endl;
    cout << "语句正确率: " << double(correct_lines)/double(total_lines)*100 << " 语句总数：" << total_lines << endl;
}

void processCLI()
{
    cout << "请输入要查询的拼音(exit 表示退出)" << endl;
    while(wcin)
    {
        cout << "> "; cout.flush(); wcout.flush();

        wstring line;
        getline(wcin, line);
        line = strip(line);
        if (line.length() == 0) continue;
        if (line == L"exit") break;

        vector<wstring> pinyin;
        wistringstream ss(line);
        wstring gram;
        while(ss >> gram)
        {
            pinyin.push_back(gram);
        }

        vector<wstring> solution;
        if (typing.Solve(pinyin, solution))
        {
            for(auto p: solution)
                wcout << p << L" ";
            wcout << endl;
        } else {
            cout << "[ERROR]查找失败" << endl;
        }
    }
}

int main(int argc, char* argv[])
{
    ios::sync_with_stdio(false);
    wcin.imbue(utf8locale);
    wcout.imbue(utf8locale);
    wcerr.imbue(utf8locale);

    cout << "初始化中..." << endl;
    setTimePoint("初始化");
    typing.Init();
    typing.Check();
    logTimePoint("初始化");

    if (argc >= 3)
    {
        char* input_filename = argv[1];
        char* output_filename = argv[2];
        char* answer_filename = argc >= 4 ? argv[3] : NULL;

        processFiles(input_filename, output_filename, answer_filename);
    }

    processCLI();

    return 0;
}
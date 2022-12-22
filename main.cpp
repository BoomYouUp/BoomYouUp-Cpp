#include <iostream>
#include <fstream>
#include <windows.h>
#include <charconv>
#include <list>
#include <iomanip>
#include <utility>
#include <filesystem>

#define VERSION "1.0"
#define CONFIG "配置.txt"

#pragma GCC optimize(3, "Ofast", "inline")

using namespace std;

struct Item {
    struct Time {
        int hour;
        int minute;
        int second;

        Time() = default;

        Time(int hour, int minute, int second) : hour(hour), minute(minute), second(second) {}

        bool operator<(const Time &other) const {
            if (hour != other.hour) {
                return hour < other.hour;
            }
            if (minute != other.minute) {
                return minute < other.minute;
            }
            return second < other.second;
        }

        bool operator>(const Time &other) const {
            if (hour != other.hour) {
                return hour > other.hour;
            }
            if (minute != other.minute) {
                return minute > other.minute;
            }
            return second > other.second;
        }

        bool operator==(const Time &other) const {
            return hour == other.hour && minute == other.minute && second == other.second;
        }
    } time{};

    list<string> command;

    Item() = default;

    Item(Time time, list<string> command) : time(time), command(std::move(command)) {}
};

int enter();

int readConfig();

int createConfig();

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);

    SetConsoleOutputCP(936);

    cout << "BoomYouUp 炸你起床 版本 " << VERSION << " 南科大附中 胡睿邈 于 2022-12-22 编写" << endl;

    return enter();
}

void addCommand(list<Item> &config, const Item::Time &time, const std::string &command) {
    auto it = lower_bound(
            config.begin(),
            config.end(),
            time,
            [](const Item &item, const Item::Time &t) {
                return item.time < t;
            }
    );

    if (it != config.end() && it->time == time) {
        it->command.push_back(command);
    } else {
        Item item;
        item.time = time;
        item.command.push_back(command);
        config.insert(it, item);
    }
}

pair<Item::Time, string> parseItem(const string &line) {
    istringstream lineStream(line);
    string hour, minute, second, command;

    bool success = true;
    lineStream >> hour;
    if (stoi(hour) < 0 || stoi(hour) > 23) {
        success = false;
    } else {
        lineStream >> minute;
        if (stoi(minute) < 0 || stoi(minute) > 59) {
            success = false;
        } else {
            lineStream >> second;
            if (stoi(second) < 0 || stoi(second) > 59) {
                success = false;
            }
        }
    }

    if (!success) {
        throw invalid_argument("时间格式错误");
    }

    lineStream >> command;

    return {
            Item::Time(stoi(hour), stoi(minute), stoi(second)),
            command
    };
}

void printConfig(const list<Item> &config) {
    cout << "配置解析中，配置如下：" << endl;
    for (const auto &item: config) {
        cout << setfill('0') << setw(2) << item.time.hour << ":"
             << setfill('0') << setw(2) << item.time.minute << ":"
             << setfill('0') << setw(2) << item.time.second << " ";
        bool shouldOutputSpace = false;
        for (const auto &command: item.command) {
            if (shouldOutputSpace) {
                cout << "         ";
            } else {
                shouldOutputSpace = true;
            }
            cout << command << endl;
        }
    }
}

bool timeEqual(const Item::Time &time1, const SYSTEMTIME &time2) {
    return time1.hour == time2.wHour && time1.minute == time2.wMinute && time1.second == time2.wSecond;
}

Item::Time toTime(const SYSTEMTIME &time) {
    return {time.wHour, time.wMinute, time.wSecond};
}

void updateNext(list<Item> &config, const Item::Time &time, Item *next) {
    auto it = upper_bound(
            config.begin(),
            config.end(),
            time,
            [](const Item::Time &t, const Item &item) {
                return t < item.time;
            }
    );

    if (it == config.end()) {
        *next = *config.begin();
    } else {
        *next = *it;
    }

    cout << "下一次执行时间：" << setfill('0') << setw(2) << next->time.hour << ":"
         << setfill('0') << setw(2) << next->time.minute << ":"
         << setfill('0') << setw(2) << next->time.second << endl;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
int enter() {
    ifstream fileStream(CONFIG);

    if (!fileStream.is_open() || fileStream.peek() == ifstream::traits_type::eof()) return createConfig();

    cout << "请问你想作甚？" << endl
         << "1. 开始运行" << endl
         << "2. 重新配置" << endl
         << "请输入：" << flush;

    int choice;
    cin >> choice;
    switch (choice) {
        case 1:
            return readConfig();
        case 2:

            cout << "确认重新配置？你的所有配置都会被清空。如需更改配置，请打开" << CONFIG << "[Y/n]" << flush;
            char confirm;
            cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                remove(CONFIG);
                return createConfig();
            } else {
                return enter();
            }
        default:
            cout << "输入错误，请重新输入。" << endl;
            return enter();
    }
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma ide diagnostic ignored "ConstantFunctionResult"

int readConfig() {
    ifstream fileStream(CONFIG);

    list<Item> config;
    string line;
    while (getline(fileStream, line)) {
        if (line.empty()) continue;

        try {
            auto item = parseItem(line);
            addCommand(config, item.first, item.second);
        } catch (const exception &e) {
            cerr << "处理配置时遇到了问题。错误信息：" << e.what() << endl;
        }
    }

    fileStream.close();

    printConfig(config);
    cout << endl;

    Item *nextItem = new Item();
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    updateNext(config, toTime(systemTime), nextItem);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        if (timeEqual(nextItem->time, systemTime)) {
            for (auto &command: nextItem->command) {
                cout << "尝试执行：" << command << endl;
                ShellExecute(
                        nullptr,
                        "open",
                        command.c_str(),
                        nullptr,
                        nullptr,
                        SW_SHOWNORMAL
                );

                DWORD exitCode = GetLastError();
                if (exitCode != 0) {
                    LPSTR messageBuffer = nullptr;
                    size_t size = FormatMessageA(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER
                            | FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_IGNORE_INSERTS,
                            nullptr,
                            exitCode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPSTR) &messageBuffer,
                            0,
                            nullptr
                    );
                    string message(messageBuffer, size);
                    LocalFree(messageBuffer);
                    cerr << "执行命令时遇到了问题。错误信息：" << message << endl;
                }
            }
            cout << endl;
            updateNext(config, nextItem->time, nextItem);
        }

        Sleep(1000);
        GetLocalTime(&systemTime);
    }
#pragma clang diagnostic pop
}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma ide diagnostic ignored "ConstantFunctionResult"

int createConfig() {
    ofstream fileStream(CONFIG);

    if (!fileStream.is_open()) {
        cerr << "创建配置文件失败" << endl;
        return 1;
    }

    cout << "配置为空，请填上它！" << endl << endl
         << "接下来请在窗口中输入时间与文件路径完善配置，每行一个，格式：" << endl << endl
         << "时 分 秒 文件路径" << endl << endl
         << "时间为 24 小时制，注意用空格分开" << endl
         << "文件路径，能打开就行" << endl << endl
         << "例子：" << endl
         << R"(12 00 00 C:\Users\Administrator\Desktop\午夜凶铃.mp5)" << endl
         << "在中午 12 点整打开 Administrator 桌面上的午夜凶铃.mp5 文件" << endl << endl
         << "11 45 14 Z:\\只因你太美.mp4" << endl
         << "在 11 点 45 分 14 秒打开 Z 盘下的只因你太美.mp4 文件并发现其内容为 Rick Roll" << endl << endl
         << "添加完毕后请输入 114514 并按回车" << endl
         << "如果你不知道文件路径是什么，可以在添加文件时将文件拖进这个控制台窗口（一般是一个黑框框）"
         << "或者选中文件并按 Ctrl+Shift+C 复制" << endl << endl
         << "小提示：按 Ctrl+C 中止程序" << endl
         << "小提示：可以在一个时间指定多个任务" << endl << endl
         << "下面请开始你的表演：" << endl;

    while (true) {
        string line;
        getline(cin, line);
        istringstream lineStream(line);
        string sign;
        lineStream >> sign;
        if (sign == "114514") break;

        try {
            parseItem(line);
        } catch (const exception &e) {
            cerr << "处理输入时遇到了问题，请重试。错误信息：" << e.what() << endl;
            continue;
        }

        fileStream << line << endl;
    }

    fileStream.close();

    cout << endl << "很好，配置已写入文件，正在尝试读取..." << endl << endl;

    return readConfig();
}

#pragma clang diagnostic pop

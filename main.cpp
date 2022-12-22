#include <iostream>
#include <fstream>
#include <windows.h>
#include <charconv>
#include <list>
#include <iomanip>
#include <utility>
#include <filesystem>

#define VERSION "1.0"
#define CONFIG "����.txt"

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

    cout << "BoomYouUp ը���� �汾 " << VERSION << " �Ͽƴ��� ����� �� 2022-12-22 ��д" << endl;

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
        throw invalid_argument("ʱ���ʽ����");
    }

    lineStream >> command;

    return {
            Item::Time(stoi(hour), stoi(minute), stoi(second)),
            command
    };
}

void printConfig(const list<Item> &config) {
    cout << "���ý����У��������£�" << endl;
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

    cout << "��һ��ִ��ʱ�䣺" << setfill('0') << setw(2) << next->time.hour << ":"
         << setfill('0') << setw(2) << next->time.minute << ":"
         << setfill('0') << setw(2) << next->time.second << endl;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
int enter() {
    ifstream fileStream(CONFIG);

    if (!fileStream.is_open() || fileStream.peek() == ifstream::traits_type::eof()) return createConfig();

    cout << "��������������" << endl
         << "1. ��ʼ����" << endl
         << "2. ��������" << endl
         << "�����룺" << flush;

    int choice;
    cin >> choice;
    switch (choice) {
        case 1:
            return readConfig();
        case 2:

            cout << "ȷ���������ã�����������ö��ᱻ��ա�����������ã����" << CONFIG << "[Y/n]" << flush;
            char confirm;
            cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                remove(CONFIG);
                return createConfig();
            } else {
                return enter();
            }
        default:
            cout << "����������������롣" << endl;
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
            cerr << "��������ʱ���������⡣������Ϣ��" << e.what() << endl;
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
                cout << "����ִ�У�" << command << endl;
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
                    cerr << "ִ������ʱ���������⡣������Ϣ��" << message << endl;
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
        cerr << "���������ļ�ʧ��" << endl;
        return 1;
    }

    cout << "����Ϊ�գ�����������" << endl << endl
         << "���������ڴ���������ʱ�����ļ�·���������ã�ÿ��һ������ʽ��" << endl << endl
         << "ʱ �� �� �ļ�·��" << endl << endl
         << "ʱ��Ϊ 24 Сʱ�ƣ�ע���ÿո�ֿ�" << endl
         << "�ļ�·�����ܴ򿪾���" << endl << endl
         << "���ӣ�" << endl
         << R"(12 00 00 C:\Users\Administrator\Desktop\��ҹ����.mp5)" << endl
         << "������ 12 ������ Administrator �����ϵ���ҹ����.mp5 �ļ�" << endl << endl
         << "11 45 14 Z:\\ֻ����̫��.mp4" << endl
         << "�� 11 �� 45 �� 14 ��� Z ���µ�ֻ����̫��.mp4 �ļ�������������Ϊ Rick Roll" << endl << endl
         << "�����Ϻ������� 114514 �����س�" << endl
         << "����㲻֪���ļ�·����ʲô������������ļ�ʱ���ļ��Ͻ��������̨���ڣ�һ����һ���ڿ��"
         << "����ѡ���ļ����� Ctrl+Shift+C ����" << endl << endl
         << "С��ʾ���� Ctrl+C ��ֹ����" << endl
         << "С��ʾ��������һ��ʱ��ָ���������" << endl << endl
         << "�����뿪ʼ��ı��ݣ�" << endl;

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
            cerr << "��������ʱ���������⣬�����ԡ�������Ϣ��" << e.what() << endl;
            continue;
        }

        fileStream << line << endl;
    }

    fileStream.close();

    cout << endl << "�ܺã�������д���ļ������ڳ��Զ�ȡ..." << endl << endl;

    return readConfig();
}

#pragma clang diagnostic pop

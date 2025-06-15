#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <iostream>
using namespace std;

class Process {
private:
    string pid;
    int runtime;
    int arrivaltime;
    int priority;
    string state;
    int attribute;
    int space;
    bool waitingIO;

public:
    Process* next;

    Process();
    Process(int _space, string _pid, int _runtime, int _arrivaltime, int _priority, string _state, int _attribute);

    void set_Process(int _space, string _pid, int _runtime, int _arrivaltime, int _priority, string _state, int _attribute);
    void show_Process();
    void show_ProcessWithResources(); // 新增：显示进程及其资源需求

    // 接口函数
    int get_priority();
    string get_state();
    int get_runtime();
    int get_arrivaltime();
    string get_pid();
    int get_attribute();
    int get_space();
    vector<string> getRequiredResources(); // 新增：获取进程所需资源列表

    void set_state(string _state);
    void set_runtime(int _runtime);
    void set_priority(int _priority);
    void set_arrivaltime(int _arrivaltime);
    void set_pid(string _pid);
    void set_attribute(int _attribute);
    void set_space(int _space);
};

#endif // PROCESS_H
#include "./include/Process.h"

Process::Process() : next(nullptr) {}

Process::Process(int _space, string _pid, int _runtime, int _arrivaltime, int _priority, string _state, int _attribute) {
    space = _space;
    pid = _pid;
    runtime = _runtime;
    arrivaltime = _arrivaltime;
    priority = _priority;
    state = _state;
    attribute = _attribute;
    next = nullptr;
}

void Process::show_Process() {
    cout << "PID: " << pid << " | State: " << state << " | Priority: " << priority
         << " | Runtime: " << runtime << " | Arrive: " << arrivaltime << " | Space: " << space << endl;
}

int Process::get_priority() { return priority; }
string Process::get_state() { return state; }
int Process::get_runtime() { return runtime; }
int Process::get_arrivaltime() { return arrivaltime; }
string Process::get_pid() { return pid; }
int Process::get_attribute() { return attribute; }
int Process::get_space() { return space; }
// vector<string> Process::get_preprogress() { return preprogress; }

void Process::set_state(string _state) { state = _state; }
void Process::set_runtime(int _runtime) { runtime = _runtime; }
void Process::set_priority(int _priority) { priority = _priority; }
void Process::set_arrivaltime(int _arrivaltime) { arrivaltime = _arrivaltime; }
// void Process::show_Process() {
//     cout << "PID: " << pid 
//          << " State: " << state 
//          << " Priority: " << priority 
//          << " Runtime: " << runtime 
//          << " Arrive: " << arrivaltime 
//          << " Space: " << space;
// }

void Process::show_ProcessWithResources() {
    show_Process();
    
    vector<string> resources = getRequiredResources();
    cout << " Resources: [";
    for (size_t i = 0; i < resources.size(); i++) {
        cout << resources[i];
        if (i < resources.size() - 1) cout << ", ";
    }
    cout << "]";
}

vector<string> Process::getRequiredResources() {
    vector<string> resources;
    
    // 所有进程都需要CPU
    resources.push_back("CPU:1");
    
    // 根据内存需求
    if (space > 0) {
        resources.push_back("Memory:" + to_string(space));
    }
    
    // 根据属性决定其他资源
    if (attribute == 1) {
        resources.push_back("Disk:1");
    } else if (attribute == 2) {
        resources.push_back("Printer:1");
    }
    
    return resources;
}
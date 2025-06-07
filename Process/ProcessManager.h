#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include "Process.h"
#include "ResourceManager.h"
#include <map>
#include "Page/PageMng.h" 
#include <string>
#include <vector>

using namespace std;

class ProcessManager {
private:
    Process* readyHead;
    Process* blockedHead;
    Process* runningHead;
    map<string, Process*> allProcs;
    int currentTime;
    ResourceManager* resourceManager;
    PagingMemoryManager* pagingManager;

public:
    ProcessManager();
    ~ProcessManager();
    
    // 进程管理
    Process* createProcess(int space, string pid, int runtime, int arrivaltime, int priority, int attribute, vector<string> pre);
    void terminateProcess(Process* proc);
    
    // 调度相关
    bool scheduleProcess(Process* proc);
    Process* selectProcess(int method);
    void runScheduler(int method);
    void interactiveScheduler(int method);
    bool hasNewProcesses();
    
    // 队列管理
    void addToBlockedQueue(Process* proc);
    void removeFromReadyQueue(Process* proc);
    void removeFromBlockedQueue(Process* proc);
    void moveToReadyQueue(Process* proc);
    void checkBlockedProcesses();
    void checkArrivingProcesses();

    // 资源管理
    void releaseProcessResources(Process* proc);
    
    // 显示和状态
    void showAll();
    void showSystemStatus();
    void showDetailedProcessInfo();
    void showProcessesWithResources(); 
    void showResourceRequirements();   
    
    // 辅助函数
    int getProcessCount();
    bool hasProcesses();
};

#endif // PROCESSMANAGER_H
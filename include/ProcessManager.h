#include <string>
#include <map>
#include <vector>
#include "Process.h"
#include "PageMng.h"
#include "ResourceManager.h"

class ProcessManager {
private:
    Process* readyHead;
    Process* blockedHead;
    Process* runningHead;
    std::map<std::string, Process*> allProcs;
    PagingMemoryManager* pagingManager;
    ResourceManager* resourceManager;
    int currentTime;

public:
    ProcessManager();
    ~ProcessManager();

    Process* createProcess(int space, std::string pid, int runtime, int arrivaltime, int priority, int attribute);
    void runScheduler(int method);
    void interactiveScheduler(int method);
    bool scheduleProcess(Process* proc);
    void handleTimeSlice();
    void wakeIOWaitingProcesses();

    void releaseProcessResources(Process* proc);
    void terminateProcess(Process* proc);

    void checkArrivingProcesses();
    void checkBlockedProcesses();
    bool hasProcesses();
    bool hasNewProcesses();

    void addToBlockedQueue(Process* proc);
    void moveToReadyQueue(Process* proc);
    void removeFromReadyQueue(Process* proc);
    void removeFromBlockedQueue(Process* proc);

    Process* selectProcess(int method);

    void showSystemStatus();
    void showAll();
    void showDetailedProcessInfo();
    void showProcessesWithResources();
    void showResourceRequirements();
    int getProcessCount();
};
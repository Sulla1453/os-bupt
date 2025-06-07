#include "ProcessManager.h"
#include <iostream>
#include <iomanip>
using namespace std;

ProcessManager::ProcessManager() : readyHead(nullptr), blockedHead(nullptr), 
                                   runningHead(nullptr), currentTime(0) {
    resourceManager = new ResourceManager();
    pagingManager = new PagingMemoryManager(256, 4); // 假设有256个页框，每个4KB=>1G
}

ProcessManager::~ProcessManager() {
    delete resourceManager;
    delete pagingManager;
    
    // 清理所有进程
    while (readyHead) {
        Process* temp = readyHead;
        readyHead = readyHead->next;
        delete temp;
    }
    
    while (blockedHead) {
        Process* temp = blockedHead;
        blockedHead = blockedHead->next;
        delete temp;
    }
    
    while (runningHead) {
        Process* temp = runningHead;
        runningHead = runningHead->next;
        delete temp;
    }
}

Process* ProcessManager::createProcess(int space, string pid, int runtime, int arrivaltime, int priority, int attribute, vector<string> pre) {
    Process* proc = new Process(space, pid, runtime, arrivaltime, priority, "new", attribute, pre);
    allProcs[pid] = proc;

    
    // 调用分页管理器进行内存分配
    if (!pagingManager->allocateMemory(atoi(pid.c_str()), space)) {
        cout << "Memory allocation failed for process " << pid << endl;
        // 根据需要可以删除进程并返回 nullptr
    }
    
    // 只有在到达时间小于等于当前时间时才加入ready队列
    if (arrivaltime <= currentTime) {
        proc->next = readyHead;
        readyHead = proc;
        proc->set_state("ready");
    } else {
        // 否则保持new状态，等待到达时间
        proc->set_state("new");
        proc->next = nullptr;
    }
    
    return proc;
}

bool ProcessManager::scheduleProcess(Process* proc) {
    // 尝试为进程分配资源
    if (resourceManager->requestResources(proc)) {
        proc->set_state("running");
        // 添加到运行队列
        proc->next = runningHead;
        runningHead = proc;
        return true;
    } else {
        // 资源不足，加入阻塞队列
        addToBlockedQueue(proc);
        return false;
    }
}

void ProcessManager::addToBlockedQueue(Process* proc) {
    removeFromReadyQueue(proc);
    proc->next = blockedHead;
    blockedHead = proc;
    proc->set_state("blocked");
}

void ProcessManager::removeFromReadyQueue(Process* proc) {
    Process* prev = nullptr;
    Process* curr = readyHead;
    
    while (curr && curr != proc) {
        prev = curr;
        curr = curr->next;
    }
    
    if (curr) {
        if (prev) prev->next = curr->next;
        else readyHead = curr->next;
        curr->next = nullptr;
    }
}

void ProcessManager::removeFromBlockedQueue(Process* proc) {
    Process* prev = nullptr;
    Process* curr = blockedHead;
    
    while (curr && curr != proc) {
        prev = curr;
        curr = curr->next;
    }
    
    if (curr) {
        if (prev) prev->next = curr->next;
        else blockedHead = curr->next;
        curr->next = nullptr;
    }
}

void ProcessManager::moveToReadyQueue(Process* proc) {
    proc->next = readyHead;
    readyHead = proc;
    proc->set_state("ready");
}
void ProcessManager::checkArrivingProcesses() {
    // 检查是否有进程在当前时间到达
    for (auto& pair : allProcs) {
        Process* proc = pair.second;
        if (proc->get_state() == "new" && proc->get_arrivaltime() <= currentTime) {
            // 进程到达，移动到ready队列
            proc->next = readyHead;
            readyHead = proc;
            proc->set_state("ready");
            cout << "Process " << proc->get_pid() << " arrived at time " << currentTime << endl;
        }
    }
}

void ProcessManager::checkBlockedProcesses() {
    Process* prev = nullptr;
    Process* curr = blockedHead;
    
    while (curr) {
        if (resourceManager->requestResources(curr)) {
            // 资源现在可用，移动到就绪队列
            if (prev) prev->next = curr->next;
            else blockedHead = curr->next;
            
            Process* next = curr->next;
            moveToReadyQueue(curr);
            curr = next;
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

Process* ProcessManager::selectProcess(int method) {
    if (!readyHead) return nullptr;
    
    Process* selected = nullptr;
    Process* curr = readyHead;
    
    switch (method) {
        case 0: { // SRTF (Shortest Remaining Time First)
            int minTime = INT_MAX;
            while (curr) {
                // 只考虑已经到达的进程
                if (curr->get_arrivaltime() <= currentTime) {
                    if (curr->get_runtime() < minTime) {
                        minTime = curr->get_runtime();
                        selected = curr;
                    }
                }
                curr = curr->next;
            }
            break;
        }
        case 1: { // HRRN (Highest Response Ratio Next)
            double maxRR = -1.0;
            while (curr) {
                // 只考虑已经到达的进程
                if (curr->get_arrivaltime() <= currentTime) {
                    int wait = currentTime - curr->get_arrivaltime();
                    double rr = (wait + curr->get_runtime()) / (double)curr->get_runtime();
                    if (rr > maxRR) {
                        maxRR = rr;
                        selected = curr;
                    }
                }
                curr = curr->next;
            }
            break;
        }
        case 2: { // FCFS (First Come First Served)
            int earliestArrival = INT_MAX;
            while (curr) {
                // 只考虑已经到达的进程
                if (curr->get_arrivaltime() <= currentTime) {
                    if (curr->get_arrivaltime() < earliestArrival) {
                        earliestArrival = curr->get_arrivaltime();
                        selected = curr;
                    }
                }
                curr = curr->next;
            }
            break;
        }
        default:
            // 默认选择第一个已到达的进程
            while (curr) {
                if (curr->get_arrivaltime() <= currentTime) {
                    selected = curr;
                    break;
                }
                curr = curr->next;
            }
            break;
    }
    
    return selected;
}
bool ProcessManager::hasNewProcesses() {
    for (auto& pair : allProcs) {
        if (pair.second->get_state() == "new") {
            return true;
        }
    }
    return false;
}

void ProcessManager::runScheduler(int method) {
    cout << "\n=== Starting Scheduler (Method: ";
    switch (method) {
        case 0: cout << "SRTF"; break;
        case 1: cout << "HRRN"; break;
        case 2: cout << "FCFS"; break;
        default: cout << "Unknown"; break;
    }
    cout << ") ===" << endl;
    
    int step = 1;
    
    while (hasProcesses()||hasNewProcesses()) {
        cout << "\n--- Step " << step++ << " ---" << endl;
        cout << "Current Time: " << currentTime << endl;
        
        checkArrivingProcesses(); // 检查是否有新进程到达
        resourceManager->showResourceStatus();
        showSystemStatus();
        showResourceRequirements(); // 显示资源需求
        
        // 选择进程
        Process* selected = selectProcess(method);
        
        if (selected) {
            cout << "\nSelected process: ";
            selected->show_ProcessWithResources(); // 显示选中进程及其资源需求
            cout << endl;
            
            // 尝试调度选中的进程
            if (scheduleProcess(selected)) {
                removeFromReadyQueue(selected);
                
                cout << "Successfully scheduled: ";
                selected->show_ProcessWithResources();
                cout << endl;
                
                // 模拟进程运行
                int runTime = selected->get_runtime();
                currentTime += runTime;
                
                cout << "Process " << selected->get_pid() 
                     << " completed at time " << currentTime << endl;
                
                // 释放资源并终止进程
                releaseProcessResources(selected);
                terminateProcess(selected);
                
                // 检查是否有阻塞进程可以被唤醒
                checkBlockedProcesses();
            } else {
                cout << "Process " << selected->get_pid() 
                     << " blocked due to insufficient resources" << endl;
                cout << "Required resources: ";
                vector<string> resources = selected->getRequiredResources();
                for (size_t i = 0; i < resources.size(); i++) {
                    cout << resources[i];
                    if (i < resources.size() - 1) cout << ", ";
                }
                cout << endl;
            }
        } else {
            // 没有就绪进程，只有阻塞进程
            if (blockedHead|| hasNewProcesses()) {
                cout << "No ready processes, advancing time..." << endl;
                currentTime++;
                checkBlockedProcesses();
            } else {
                cout << "All processes completed!" << endl;
                break;
            }
        }
        
        // 防止无限循环
        if (step > 100) {
            cout << "Maximum steps reached, terminating..." << endl;
            break;
        }
    }
    
    cout << "\n=== Scheduler Completed ===" << endl;
    cout << "Final time: " << currentTime << endl;
}

void ProcessManager::interactiveScheduler(int method) {
    cout << "\n=== Interactive Scheduler ===" << endl;
    cout << "Press Enter to proceed step by step..." << endl;
    
    int step = 1;
    
    while (readyHead || blockedHead) {
        cout << "\n--- Step " << step++ << " (Time: " << currentTime << ") ---" << endl;
        
        // 显示当前状态
        resourceManager->showResourceStatus();
        showSystemStatus();
        
        // 等待用户输入
        cout << "\nPress Enter to continue (or 'q' to quit): ";
        string input;
        getline(cin, input);
        if (input == "q" || input == "Q") {
            cout << "Scheduler terminated by user." << endl;
            break;
        }
        
        // 选择进程
        Process* selected = selectProcess(method);
        
        if (selected) {
            cout << "\nSelected process: " << selected->get_pid() << endl;
            cout << "Attempting to schedule..." << endl;
            
            // 尝试调度选中的进程
            if (scheduleProcess(selected)) {
                removeFromReadyQueue(selected);
                
                cout << "Successfully scheduled: ";
                selected->show_Process();
                
                // 模拟进程运行
                int runTime = selected->get_runtime();
                currentTime += runTime;
                
                cout << "Process " << selected->get_pid() 
                     << " completed at time " << currentTime << endl;
                
                // 释放资源并终止进程
                releaseProcessResources(selected);
                terminateProcess(selected);
                
                // 检查是否有阻塞进程可以被唤醒
                checkBlockedProcesses();
            } else {
                cout << "Process " << selected->get_pid() 
                     << " blocked due to insufficient resources" << endl;
            }
        } else {
            // 没有就绪进程，只有阻塞进程
            if (blockedHead) {
                cout << "No ready processes, advancing time..." << endl;
                currentTime++;
                checkBlockedProcesses();
            } else {
                cout << "All processes completed!" << endl;
                break;
            }
        }
        
        // 防止无限循环
        if (step > 100) {
            cout << "Maximum steps reached, terminating..." << endl;
            break;
        }
    }
    
    cout << "\n=== Interactive Scheduler Completed ===" << endl;
    cout << "Final time: " << currentTime << endl;
}

void ProcessManager::releaseProcessResources(Process* proc) {
    resourceManager->releaseResources(proc);
}

void ProcessManager::terminateProcess(Process* proc) {
    cout << "Terminating process: " << proc->get_pid() << endl;
    proc->set_state("terminated");
    allProcs.erase(proc->get_pid());

    // 释放分页管理器的资源
    pagingManager->deallocateMemory(atoi(proc->get_pid().c_str()));
    
    // 从运行队列中移除
    Process* prev = nullptr;
    Process* curr = runningHead;
    while (curr && curr != proc) {
        prev = curr;
        curr = curr->next;
    }
    if (curr) {
        if (prev) prev->next = curr->next;
        else runningHead = curr->next;
    }
    
    delete proc;
}

void ProcessManager::showSystemStatus() {
    cout << "\n=== System Status ===" << endl;
    
    // 显示就绪队列及其资源需求
    cout << "Ready Queue: ";
    if (!readyHead) {
        cout << "(empty)";
    } else {
        cout << endl;
        Process* curr = readyHead;
        while (curr) {
            cout << "  ";
            curr->show_ProcessWithResources();
            cout << endl;
            curr = curr->next;
        }
    }
    
    // 显示阻塞队列及其资源需求
    cout << "Blocked Queue: ";
    if (!blockedHead) {
        cout << "(empty)";
    } else {
        cout << endl;
        Process* curr = blockedHead;
        while (curr) {
            cout << "  ";
            curr->show_ProcessWithResources();
            cout << endl;
            curr = curr->next;
        }
    }
    
    // 显示运行队列及其资源需求
    cout << "Running Queue: ";
    if (!runningHead) {
        cout << "(empty)";
    } else {
        cout << endl;
        Process* curr = runningHead;
        while (curr) {
            cout << "  ";
            curr->show_ProcessWithResources();
            cout << endl;
            curr = curr->next;
        }
    }
    
    cout << "===================" << endl;
}

void ProcessManager::showAll() {
    cout << "\n=== Process Manager Status ===" << endl;
    cout << "Current Time: " << currentTime << endl;
    showSystemStatus();
    resourceManager->showResourceStatus();
    cout << "=============================" << endl;
}

void ProcessManager::showDetailedProcessInfo() {
    cout << "\n=== Detailed Process Information ===" << endl;
    cout << left << setw(8) << "PID" 
         << setw(10) << "State" 
         << setw(8) << "Runtime" 
         << setw(8) << "Arrive" 
         << setw(8) << "Priority" 
         << setw(8) << "Space" 
         << setw(10) << "Attribute" << endl;
    cout << string(60, '-') << endl;
    
    // 显示就绪进程
    Process* curr = readyHead;
    while (curr) {
        cout << left << setw(8) << curr->get_pid()
             << setw(10) << curr->get_state()
             << setw(8) << curr->get_runtime()
             << setw(8) << curr->get_arrivaltime()
             << setw(8) << curr->get_priority()
             << setw(8) << curr->get_space()
             << setw(10) << curr->get_attribute() << endl;
        curr = curr->next;
    }
    
    // 显示阻塞进程
    curr = blockedHead;
    while (curr) {
        cout << left << setw(8) << curr->get_pid()
             << setw(10) << curr->get_state()
             << setw(8) << curr->get_runtime()
             << setw(8) << curr->get_arrivaltime()
             << setw(8) << curr->get_priority()
             << setw(8) << curr->get_space()
             << setw(10) << curr->get_attribute() << endl;
        curr = curr->next;
    }
    
    // 显示运行进程
    curr = runningHead;
    while (curr) {
        cout << left << setw(8) << curr->get_pid()
             << setw(10) << curr->get_state()
             << setw(8) << curr->get_runtime()
             << setw(8) << curr->get_arrivaltime()
             << setw(8) << curr->get_priority()
             << setw(8) << curr->get_space()
             << setw(10) << curr->get_attribute() << endl;
        curr = curr->next;
    }
    
    cout << "===================================" << endl;
}
void ProcessManager::showProcessesWithResources() {
    cout << "\n=== All Processes with Resource Requirements ===" << endl;
    
    cout << "Ready Processes:" << endl;
    Process* curr = readyHead;
    while (curr) {
        cout << "  ";
        curr->show_ProcessWithResources();
        cout << endl;
        curr = curr->next;
    }
    
    cout << "Blocked Processes:" << endl;
    curr = blockedHead;
    while (curr) {
        cout << "  ";
        curr->show_ProcessWithResources();
        cout << endl;
        curr = curr->next;
    }
    
    cout << "Running Processes:" << endl;
    curr = runningHead;
    while (curr) {
        cout << "  ";
        curr->show_ProcessWithResources();
        cout << endl;
        curr = curr->next;
    }
    
    cout << "=============================================" << endl;
}
void ProcessManager::showResourceRequirements() {
    cout << "\n=== Resource Requirements Summary ===" << endl;
    
    // 统计各种资源的总需求
    int totalCPU = 0, totalMemory = 0, totalDisk = 0, totalPrinter = 0;
    
    // 统计就绪进程
    Process* curr = readyHead;
    while (curr) {
        totalCPU++;
        totalMemory += curr->get_space();
        if (curr->get_attribute() == 1) totalDisk++;
        if (curr->get_attribute() == 2) totalPrinter++;
        curr = curr->next;
    }
    
    // 统计阻塞进程
    curr = blockedHead;
    while (curr) {
        totalCPU++;
        totalMemory += curr->get_space();
        if (curr->get_attribute() == 1) totalDisk++;
        if (curr->get_attribute() == 2) totalPrinter++;
        curr = curr->next;
    }
    
    // 统计运行进程
    curr = runningHead;
    while (curr) {
        totalCPU++;
        totalMemory += curr->get_space();
        if (curr->get_attribute() == 1) totalDisk++;
        if (curr->get_attribute() == 2) totalPrinter++;
        curr = curr->next;
    }
    
    cout << "Total CPU needed: " << totalCPU << endl;
    cout << "Total Memory needed: " << totalMemory << endl;
    cout << "Total Disk needed: " << totalDisk << endl;
    cout << "Total Printer needed: " << totalPrinter << endl;
    cout << "=================================" << endl;
}

int ProcessManager::getProcessCount() {
    int count = 0;
    
    Process* curr = readyHead;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    curr = blockedHead;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    curr = runningHead;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    return count;
}

bool ProcessManager::hasProcesses() {
    return (readyHead != nullptr) || (blockedHead != nullptr) || (runningHead != nullptr);
}
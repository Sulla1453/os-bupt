#include "ResourceManager.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

map<string, Semaphore*> SystemSemaphores;
ResourceManager::ResourceManager(PagingMemoryManager* pm) : pagingManager(pm) {
    initializeResources();
}

ResourceManager::~ResourceManager() {
    for (auto& pair : resources) {
        delete pair.second;
    }
    resources.clear();
}

void ResourceManager::initializeResources() {
    resources["CPU"] = new Resource(2, "CPU");
    resources["Memory"] = new Resource(1024, "Memory");
    resources["Disk"] = new Resource(1, "Disk");
    resources["Printer"] = new Resource(1, "Printer");
}
bool ResourceManager::handleMemoryShortage(Process* process) {
    cout << "ResourceManager: 内存不足，尝试页置换操作..." << endl;
    
    // 查找可以换出的进程（LRU算法）
    Process* victimProcess = findLRUProcess();
    if (victimProcess) {
        // 将victim进程换出到磁盘
        swapOut(victimProcess);
        
        // 尝试为新进程分配内存
        if (pagingManager->allocateMemory(stoi(process->get_pid()), process->get_space())) {
            cout << "ResourceManager: 换页成功，为进程 " << process->get_pid() << " 分配内存" << endl;
            return true;
        }
    }
    
    return false;
}

void ResourceManager::addToWaitingQueue(Process* process, const string& resourceName) {
    // 将进程加入资源等待队列
    if (SystemSemaphores.find(resourceName) != SystemSemaphores.end()) {
        SystemSemaphores[resourceName]->P(process);
    }
}

bool ResourceManager::requestResources(Process* process) {
    vector<string> requiredResources;
    vector<int> requiredAmounts;
    if (!process) {
        std::cout << "ResourceManager: 进程指针为空" << std::endl;
        return false;
    }
    std::cout << "ResourceManager: 进程 " << process->get_pid() 
              << " 请求资源分配" << std::endl;
    if (process->get_space() > 0) {
        std::cout << "ResourceManager: 为进程 " << process->get_pid() 
                  << " 分配内存 " << process->get_space() << "KB" << std::endl;
        if (!pagingManager->allocateMemory(stoi(process->get_pid()), process->get_space())) {
            std::cout << "ResourceManager: 内存分配失败，尝试换页操作..." << std::endl;
            return false; // 
            // 内存不足时的处理策略
            if (handleMemoryShortage(process)) {
                // 再次尝试分配
                if (pagingManager->allocateMemory(std::stoi(process->get_pid()), process->get_space())) {
                    std::cout << "ResourceManager: 换页后内存分配成功" << std::endl;
                } else {
                    std::cout << "ResourceManager: 换页后仍无法分配内存，进程进入等待状态" << std::endl;
                    addToWaitingQueue(process, "Memory");
                    return false;
                }
            } else {
                std::cout << "ResourceManager: 换页操作失败，进程进入等待状态" << std::endl;
                addToWaitingQueue(process, "Memory");
                return false;
            }
        }
    }
    
    // 所有进程都需要CPU
    requiredResources.push_back("CPU");
    requiredAmounts.push_back(1);
    
    // // 根据内存需求
    // if (process->get_space() > 0) {
    //     requiredResources.push_back("Memory");
    //     requiredAmounts.push_back(process->get_space());
    // }
    
    // 根据属性决定其他资源
    if (process->get_attribute() == 1) {
        requiredResources.push_back("Disk");
        requiredAmounts.push_back(1);
    }
    if (process->get_attribute() == 2) {
        requiredResources.push_back("Printer");
        requiredAmounts.push_back(1);
    }
    
    // 检查所有资源是否可用
    for (int i = 0; i < requiredResources.size(); i++) {
        string resourceName = requiredResources[i];
        int amount = requiredAmounts[i];
        
        if (resources[resourceName]->available < amount) {
            return false; // 资源不足
        }
    }
    
    // 分配资源
    vector<string> allocatedResources;
    for (int i = 0; i < requiredResources.size(); i++) {
        string resourceName = requiredResources[i];
        int amount = requiredAmounts[i];
        
        if (allocateResource(process, resourceName, amount)) {
            allocatedResources.push_back(resourceName);
        }
    }
    
    processResources[process->get_pid()] = allocatedResources;
    return true;
}

void ResourceManager::releaseResources(Process* process) {
    string pid = process->get_pid();
    pagingManager->deallocateMemory(stoi(process->get_pid()));
    
    if (processResources.find(pid) != processResources.end()) {
        for (const string& resourceName : processResources[pid]) {
            freeResource(process, resourceName);
        }
        processResources.erase(pid);
    }
}

bool ResourceManager::allocateResource(Process* process, const string& resourceName, int amount) {
    if (resources.find(resourceName) != resources.end()) {
        if (resources[resourceName]->available >= amount) {
            resources[resourceName]->available -= amount;
            cout << "Allocated " << amount << " " << resourceName 
                 << " to process " << process->get_pid() << endl;
            return true;
        }
    }
    return false;
}

void ResourceManager::freeResource(Process* process, const string& resourceName, int amount) {
    if (resources.find(resourceName) != resources.end()) {
        resources[resourceName]->available += amount;
        cout << "Released " << amount << " " << resourceName 
             << " from process " << process->get_pid() << endl;
    }
}

void ResourceManager::showResourceStatus() {
    cout << "\n=== Resource Status ===" << endl;
    for (auto& pair : resources) {
        Resource* res = pair.second;
        cout << res->name << ": " << res->available << "/" << res->total 
             << " (Used: " << (res->total - res->available) << ")" << endl;
    }
    cout << "======================" << endl;
}
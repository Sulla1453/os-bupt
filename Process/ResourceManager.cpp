#include "ResourceManager.h"
#include <iostream>
using namespace std;

ResourceManager::ResourceManager() {
    initializeResources();
}

ResourceManager::~ResourceManager() {
    for (auto& pair : resources) {
        delete pair.second;
    }
}

void ResourceManager::initializeResources() {
    resources["CPU"] = new Resource(2, "CPU");
    resources["Memory"] = new Resource(1024, "Memory");
    resources["Disk"] = new Resource(1, "Disk");
    resources["Printer"] = new Resource(1, "Printer");
}

bool ResourceManager::requestResources(Process* process) {
    vector<string> requiredResources;
    vector<int> requiredAmounts;
    
    // 所有进程都需要CPU
    requiredResources.push_back("CPU");
    requiredAmounts.push_back(1);
    
    // 根据内存需求
    if (process->get_space() > 0) {
        requiredResources.push_back("Memory");
        requiredAmounts.push_back(process->get_space());
    }
    
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
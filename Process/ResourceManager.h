#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "Process.h"
#include <map>
#include <string>
#include <vector>

class ResourceManager {
private:
    struct Resource {
        int total;
        int available;
        string name;
        
        Resource(int t, string n) : total(t), available(t), name(n) {}
    };
    
    map<string, Resource*> resources;
    map<string, vector<string>> processResources; // 进程占用的资源

public:
    ResourceManager();
    ~ResourceManager();
    
    void initializeResources();
    bool requestResources(Process* process);
    void releaseResources(Process* process);
    
    bool allocateResource(Process* process, const string& resourceName, int amount = 1);
    void freeResource(Process* process, const string& resourceName, int amount = 1);
    
    void showResourceStatus();
};

#endif // RESOURCEMANAGER_H
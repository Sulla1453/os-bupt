#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <string>
#include <queue>
#include "Process.h"

class Semaphore {
private:
    int value;
    std::queue<Process*> waitingQueue;
    std::string name;

public:
    Semaphore(int initial_value, std::string sem_name);
    
    bool P(Process* process);  // wait操作
    void V();                  // signal操作
    
    int getValue() const;
    bool hasWaitingProcesses() const;
    Process* getNextWaitingProcess();
    std::string getName() const;
    
    void showStatus();
};

#endif
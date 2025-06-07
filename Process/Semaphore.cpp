#include "Semaphore.h"
#include <iostream>

Semaphore::Semaphore(int initial_value, std::string sem_name) 
    : value(initial_value), name(sem_name) {}

bool Semaphore::P(Process* process) {
    value--;
    if (value < 0) {
        // 资源不足，进程进入等待队列
        waitingQueue.push(process);
        process->set_state("blocked");
        std::cout << "Process " << process->get_pid() 
                  << " blocked waiting for " << name << std::endl;
        return false;
    }
    return true;
}

void Semaphore::V() {
    value++;
    if (value <= 0 && !waitingQueue.empty()) {
        // 有等待的进程，唤醒一个
        Process* process = waitingQueue.front();
        waitingQueue.pop();
        process->set_state("ready");
        std::cout << "Process " << process->get_pid() 
                  << " waken up for " << name << std::endl;
    }
}

int Semaphore::getValue() const {
    return value;
}

bool Semaphore::hasWaitingProcesses() const {
    return !waitingQueue.empty();
}

Process* Semaphore::getNextWaitingProcess() {
    if (!waitingQueue.empty()) {
        Process* process = waitingQueue.front();
        waitingQueue.pop();
        return process;
    }
    return nullptr;
}

std::string Semaphore::getName() const {
    return name;
}

void Semaphore::showStatus() {
    std::cout << "Semaphore " << name << ": value=" << value 
              << ", waiting=" << waitingQueue.size() << std::endl;
}
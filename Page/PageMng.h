#ifndef PAGEMNG_H
#define PAGEMNG_H

#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <iomanip>
#include <algorithm>
#include <cstring>

class PagingMemoryManager {
private:
    struct PageFrame {
        bool occupied;      // 页框是否被占用
        int processId;      // 占用该页框的进程ID
        int pageNumber;     // 逻辑页号

        PageFrame();
    };

    struct ProcessInfo {
        int processId;
        int pageCount;              // 进程占用的页数
        std::vector<int> pageTable; // 页表：逻辑页号 -> 物理页框号

        ProcessInfo();
        ProcessInfo(int id, int count);
    };

    int totalFrames;                       // 总页框数
    int frameSize;                         // 页框大小(KB)
    std::vector<PageFrame> physicalMemory; // 物理内存页框
    std::queue<int> freeFrames;            // 空闲页框队列
    std::map<int, ProcessInfo> processes;  // 进程信息表

public:
    PagingMemoryManager(int frames, int size);

    // 为进程分配内存
    bool allocateMemory(int processId, int memorySize);

    // 回收进程内存
    bool deallocateMemory(int processId);

    // 逻辑地址转换为物理地址
    int translateAddress(int processId, int logicalAddress);

    // 显示内存状态
    void displayMemoryStatus();

    // 获取内存利用率
    double getMemoryUtilization();

    // 获取空闲内存大小
    int getFreeMemory();
};

// 测试函数声明
void runDemo();

#endif // PAGEMNG_H
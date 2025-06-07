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
        
        PageFrame() : occupied(false), processId(-1), pageNumber(-1) {}
    };
    
    struct ProcessInfo {
        int processId;
        int pageCount;      // 进程占用的页数
        std::vector<int> pageTable;  // 页表：逻辑页号 -> 物理页框号
        
        ProcessInfo() : processId(-1), pageCount(0) {}
        ProcessInfo(int id, int count) : processId(id), pageCount(count) {
            pageTable.resize(count, -1);
        }
    };
    
    int totalFrames;                    // 总页框数
    int frameSize;                      // 页框大小(KB)
    std::vector<PageFrame> physicalMemory;  // 物理内存页框
    std::queue<int> freeFrames;         // 空闲页框队列
    std::map<int, ProcessInfo> processes;   // 进程信息表
    
public:
    PagingMemoryManager(int frames, int size) : totalFrames(frames), frameSize(size) {
        physicalMemory.resize(totalFrames);
        // 初始化所有页框为空闲
        for (int i = 0; i < totalFrames; i++) {
            freeFrames.push(i);
        }
        
        std::cout << "分页式存储管理系统初始化完成" << std::endl;
        std::cout << "总页框数: " << totalFrames << std::endl;
        std::cout << "页框大小: " << frameSize << "KB" << std::endl;
        std::cout << "总内存大小: " << totalFrames * frameSize << "KB" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }
    
    // 为进程分配内存
    bool allocateMemory(int processId, int memorySize) {
        // 计算需要的页数
        int pagesNeeded = (memorySize + frameSize - 1) / frameSize;  // 向上取整
        
        if (pagesNeeded > freeFrames.size()) {
            std::cout << "内存分配失败: 进程 " << processId 
                      << " 需要 " << pagesNeeded << " 页，但只有 " 
                      << freeFrames.size() << " 页可用" << std::endl;
            return false;
        }
        
        // 检查进程是否已存在
        if (processes.find(processId) != processes.end()) {
            std::cout << "内存分配失败: 进程 " << processId << " 已存在" << std::endl;
            return false;
        }
        
        // 创建进程信息
        ProcessInfo process(processId, pagesNeeded);
        
        // 分配页框
        std::vector<int> allocatedFrames;
        for (int i = 0; i < pagesNeeded; i++) {
            int frameNum = freeFrames.front();
            freeFrames.pop();
            allocatedFrames.push_back(frameNum);
            
            // 更新页框信息
            physicalMemory[frameNum].occupied = true;
            physicalMemory[frameNum].processId = processId;
            physicalMemory[frameNum].pageNumber = i;
            
            // 更新页表
            process.pageTable[i] = frameNum;
        }
        
        // 保存进程信息
        processes[processId] = process;
        
        std::cout << "内存分配成功: 进程 " << processId 
                  << " 分配了 " << pagesNeeded << " 页 (" 
                  << memorySize << "KB)" << std::endl;
        std::cout << "分配的页框: ";
        for (int frame : allocatedFrames) {
            std::cout << frame << " ";
        }
        std::cout << std::endl;
        
        return true;
    }
    
    // 回收进程内存
    bool deallocateMemory(int processId) {
        auto it = processes.find(processId);
        if (it == processes.end()) {
            std::cout << "内存回收失败: 进程 " << processId << " 不存在" << std::endl;
            return false;
        }
        
        ProcessInfo& process = it->second;
        std::vector<int> freedFrames;
        int pageCount = process.pageCount;  // 保存页数，因为后面会删除进程信息
        
        // 释放所有页框
        for (int i = 0; i < process.pageCount; i++) {
            int frameNum = process.pageTable[i];
            if (frameNum != -1) {
                // 重置页框信息
                physicalMemory[frameNum].occupied = false;
                physicalMemory[frameNum].processId = -1;
                physicalMemory[frameNum].pageNumber = -1;
                
                // 添加到空闲队列
                freeFrames.push(frameNum);
                freedFrames.push_back(frameNum);
            }
        }
        
        // 删除进程信息
        processes.erase(it);
        
        std::cout << "内存回收成功: 进程 " << processId 
                  << " 释放了 " << pageCount << " 页" << std::endl;
        std::cout << "释放的页框: ";
        for (int frame : freedFrames) {
            std::cout << frame << " ";
        }
        std::cout << std::endl;
        
        return true;
    }
    
    // 逻辑地址转换为物理地址
    int translateAddress(int processId, int logicalAddress) {
        auto it = processes.find(processId);
        if (it == processes.end()) {
            std::cout << "地址转换失败: 进程 " << processId << " 不存在" << std::endl;
            return -1;
        }
        
        ProcessInfo& process = it->second;
        
        // 计算页号和页内偏移
        int pageNumber = logicalAddress / (frameSize * 1024);  // 转换为字节
        int offset = logicalAddress % (frameSize * 1024);
        
        if (pageNumber >= process.pageCount) {
            std::cout << "地址转换失败: 页号 " << pageNumber << " 超出范围" << std::endl;
            return -1;
        }
        
        int frameNumber = process.pageTable[pageNumber];
        if (frameNumber == -1) {
            std::cout << "地址转换失败: 页 " << pageNumber << " 未分配" << std::endl;
            return -1;
        }
        
        int physicalAddress = frameNumber * frameSize * 1024 + offset;
        
        std::cout << "地址转换: 进程 " << processId 
                  << " 逻辑地址 " << logicalAddress 
                  << " -> 物理地址 " << physicalAddress 
                  << " (页号:" << pageNumber << ", 页框:" << frameNumber 
                  << ", 偏移:" << offset << ")" << std::endl;
        
        return physicalAddress;
    }
    
    // 显示内存状态
    void displayMemoryStatus() {
        std::cout << "\n======== 内存状态 ========" << std::endl;
        
        // 显示页框使用情况
        std::cout << "页框使用情况:" << std::endl;
        std::cout << "页框号\t状态\t进程ID\t逻辑页号" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        
        for (int i = 0; i < totalFrames; i++) {
            std::cout << std::setw(4) << i << "\t";
            if (physicalMemory[i].occupied) {
                std::cout << "占用\t" << physicalMemory[i].processId 
                          << "\t" << physicalMemory[i].pageNumber;
            } else {
                std::cout << "空闲\t-\t-";
            }
            std::cout << std::endl;
        }
        
        // 显示进程信息
        std::cout << "\n进程信息:" << std::endl;
        std::cout << "进程ID\t页数\t页表映射" << std::endl;
        std::cout << "------------------------------------" << std::endl;
        
        for (auto& pair : processes) {
            ProcessInfo& process = pair.second;
            std::cout << process.processId << "\t" << process.pageCount << "\t";
            for (int i = 0; i < process.pageCount; i++) {
                std::cout << i << "->" << process.pageTable[i] << " ";
            }
            std::cout << std::endl;
        }
        
        // 显示内存利用率
        int occupiedFrames = totalFrames - freeFrames.size();
        double utilization = (double)occupiedFrames / totalFrames * 100;
        std::cout << "\n内存利用率: " << std::fixed << std::setprecision(2) 
                  << utilization << "% (" << occupiedFrames << "/" 
                  << totalFrames << ")" << std::endl;
        std::cout << "空闲页框数: " << freeFrames.size() << std::endl;
        std::cout << "================================\n" << std::endl;
    }
    
    // 获取内存利用率
    double getMemoryUtilization() {
        int occupiedFrames = totalFrames - freeFrames.size();
        return (double)occupiedFrames / totalFrames * 100;
    }
    
    // 获取空闲内存大小
    int getFreeMemory() {
        return freeFrames.size() * frameSize;
    }
};

// 测试函数
void runDemo() {
    std::cout << "=== 分页式存储管理系统演示 ===" << std::endl;
    
    // 创建管理器：16个页框，每个4KB
    PagingMemoryManager manager(16, 4);
    
    std::cout << "\n1. 分配内存测试:" << std::endl;
    manager.allocateMemory(101, 12);  // 进程101需要12KB（3页）
    manager.allocateMemory(102, 20);  // 进程102需要20KB（5页）
    manager.allocateMemory(103, 8);   // 进程103需要8KB（2页）
    
    manager.displayMemoryStatus();
    
    std::cout << "2. 地址转换测试:" << std::endl;
    manager.translateAddress(101, 5000);   // 进程101的逻辑地址5000
    manager.translateAddress(102, 10000);  // 进程102的逻辑地址10000
    manager.translateAddress(103, 2000);   // 进程103的逻辑地址2000
    
    std::cout << "\n3. 内存回收测试:" << std::endl;
    manager.deallocateMemory(102);  // 回收进程102
    
    manager.displayMemoryStatus();
    
    std::cout << "4. 再次分配测试:" << std::endl;
    manager.allocateMemory(104, 16);  // 进程104需要16KB（4页）
    
    manager.displayMemoryStatus();
    
    std::cout << "5. 尝试分配过大内存:" << std::endl;
    manager.allocateMemory(105, 100); // 尝试分配100KB（超出可用内存）
    
    std::cout << "\n6. 清理所有进程:" << std::endl;
    manager.deallocateMemory(101);
    manager.deallocateMemory(103);
    manager.deallocateMemory(104);
    
    manager.displayMemoryStatus();
}

int main() {
    runDemo();
    return 0;
}
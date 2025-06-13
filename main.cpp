#include "Process/ProcessManager.h"
#include "InterputMng/InterputMng.h"
#include "ResourceMng/ResourceManager.h"
#include "Page/PageMng.h"
#include "FileSystem/FileSystem.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

class OSKernel {
public:
    OSKernel() {
        // 初始化组件
        pagingManager = std::make_unique<PagingMemoryManager>(64, 4); // 64页框，4KB每页
        resourceManager = std::make_unique<ResourceManager>(pagingManager.get());
        processManager = std::make_unique<ProcessManager>();
        
        // 设置组件间的依赖关系
        processManager->setResourceManager(resourceManager.get());
        processManager->setPagingManager(pagingManager.get());
        
        // 初始化中断管理器
        GlobalInterruptManager::initialize();
        auto& interruptMgr = GlobalInterruptManager::getInstance();
        interruptMgr.setProcessManager(processManager.get());
        
        setupInterruptHandlers();
        std::cout << "[内核] 系统初始化完成" << std::endl;
    }
    
    void setupInterruptHandlers() {
        auto& interruptMgr = GlobalInterruptManager::getInstance();
        
        // 注册自定义中断处理程序
        interruptMgr.registerInterrupt(TIMER_INTERRUPT, [this]() {
            on_timer();
        });
        
        interruptMgr.registerInterrupt(SYSTEM_CALL_INTERRUPT, [this]() {
            on_syscall();
        });
    }
    
    void on_timer() {
        std::cout << "[内核] 定时器中断处理" << std::endl;
        
        // 检查新到达的进程
        processManager->checkArrivingProcesses();
        
        // 处理时间片轮转
        processManager->handleTimeSlice();
        
        // 检查阻塞进程是否可以恢复
        processManager->checkBlockedProcesses();
        
        // 执行调度
        processManager->schedule();
    }

    void on_syscall() {
        std::cout << "[内核] 系统调用中断处理" << std::endl;
        // 处理系统调用
    }
    
    void run() {
        std::cout << "[内核] 启动操作系统模拟器" << std::endl;
        
        // 启用定时器中断（100ms间隔）
        auto& interruptMgr = GlobalInterruptManager::getInstance();
        interruptMgr.enableTimer(100);
        
        // 创建一些测试进程
        createTestProcesses();
        
        // 主循环
        mainLoop();
        
        // 清理
        cleanup();
    }
    
private:
    void createTestProcesses() {
        std::cout << "[内核] 创建测试进程" << std::endl;
        
        // 创建几个不同类型的进程
        processManager->createProcess(8, "101", 5000, 0, 3, 0, {});  // 普通进程
        processManager->createProcess(12, "102", 3000, 1000, 2, 1, {}); // 需要打印机
        processManager->createProcess(6, "103", 4000, 2000, 1, 2, {});  // 需要扫描仪
        processManager->createProcess(10, "104", 6000, 1500, 4, 0, {"101"}); // 依赖进程101
    }
    
    void mainLoop() {
        int cycles = 0;
        const int maxCycles = 100; // 运行100个周期
        
        while (!exit_flag && cycles < maxCycles) {
            // 处理所有待处理的中断
            auto& interruptMgr = GlobalInterruptManager::getInstance();
            interruptMgr.processAllInterrupts();
            
            // 更新系统时间
            processManager->incrementTime(100); // 增加100ms
            
            // 显示系统状态（每10个周期显示一次）
            if (cycles % 10 == 0) {
                displaySystemStatus();
            }
            
            // 短暂休眠，模拟系统时钟
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cycles++;
        }
        
        std::cout << "[内核] 模拟运行完成" << std::endl;
    }
    
    void displaySystemStatus() {
        std::cout << "\n=== 系统状态 ===" << std::endl;
        processManager->displayProcesses();
        resourceManager->showResourceStatus();
        std::cout << "内存利用率: " << pagingManager->getMemoryUtilization() << "%" << std::endl;
        std::cout << "================\n" << std::endl;
    }
    
    void cleanup() {
        auto& interruptMgr = GlobalInterruptManager::getInstance();
        interruptMgr.disableTimer();
        GlobalInterruptManager::cleanup();
        std::cout << "[内核] 系统清理完成" << std::endl;
    }

private:
    std::unique_ptr<PagingMemoryManager> pagingManager;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<ProcessManager> processManager;
    bool exit_flag = false;
};

// 显示菜单的辅助函数
void showMenu() {
    std::cout << "\n=== 操作系统模拟器 ===" << std::endl;
    std::cout << "1. 启动自动演示" << std::endl;
    std::cout << "2. 手动创建进程" << std::endl;
    std::cout << "3. 显示系统状态" << std::endl;
    std::cout << "4. 资源管理演示" << std::endl;
    std::cout << "5. 内存管理演示" << std::endl;
    std::cout << "0. 退出" << std::endl;
    std::cout << "请选择: ";
}

int main() {
    try {
        std::cout << "=== 操作系统课程设计 - OS模拟器 ===" << std::endl;
        
        int choice;
        showMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1: {
                // 自动演示模式
                OSKernel kernel;
                kernel.run();
                break;
            }
            case 2: {
                // 手动模式 - 可以扩展
                std::cout << "手动模式开发中..." << std::endl;
                break;
            }
            case 3: {
                // 状态显示 - 可以扩展
                std::cout << "状态显示开发中..." << std::endl;
                break;
            }
            case 4: {
                // 资源管理演示
                std::cout << "启动资源管理演示..." << std::endl;
                // 可以调用 ResourceManager 的演示函数
                break;
            }
            case 5: {
                // 内存管理演示
                std::cout << "启动内存管理演示..." << std::endl;
                // 可以调用 runPageManagerDemo()
                break;
            }
            case 0:
                std::cout << "退出系统" << std::endl;
                break;
            default:
                std::cout << "无效选择" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "系统错误: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
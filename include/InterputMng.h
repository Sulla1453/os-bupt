#ifndef INTERPUT_MNG_H
#define INTERPUT_MNG_H

#include <vector>
#include <functional>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>

// 前向声明
class ProcessManager;

// 中断类型枚举
enum InterruptType {
    TIMER_INTERRUPT = 0,      // 定时器中断
    KEYBOARD_INTERRUPT = 1,   // 键盘中断
    IO_INTERPUT = 2,         // I/O中断
    MEMORY_INTERRUPT = 3,     // 内存中断
    SYSTEM_CALL_INTERRUPT = 4, // 系统调用中断
    INTERRUPT_COUNT = 5       // 中断类型总数
};

// ==================== 中断向量表 ====================
class InterruptVectorTable {
private:
    std::vector<std::function<void()>> handlers;

public:
    InterruptVectorTable();
    void registerHandler(InterruptType type, std::function<void()> handler);
    void handleInterrupt(InterruptType type);
};

// ==================== 中断控制器 ====================
class InterruptController {
private:
    std::vector<InterruptType> pendingInterrupts;
    mutable std::mutex mtx;

public:
    InterruptController();
    void triggerInterrupt(InterruptType type);
    void processInterrupts(InterruptVectorTable& ivt);
    bool hasPendingInterrupts() const;
};

// ==================== 定时器类 ====================
class Timer {
private:
    InterruptController& controller;
    int interval;
    std::atomic<bool> running;
    std::thread timerThread;

public:
    Timer(InterruptController& controller, int intervalMs);
    ~Timer();
    
    void start();
    void stop();
    void setInterval(int intervalMs);
    bool isRunning() const;
};

// ==================== 中断上下文 ====================
class InterruptContext {
private:
    int processId;
    int programCounter;
    int stackPointer;

public:
    InterruptContext();
    void saveContext(int pid, int pc, int sp);
    void restoreContext();
};

// ==================== 中断管理器 ====================
class InterruptManager {
private:
    std::unique_ptr<InterruptVectorTable> ivt;
    std::unique_ptr<InterruptController> controller;
    std::unique_ptr<Timer> timer;
    std::map<int, std::function<void()>> interruptVector;
    
    bool timerEnabled;
    int timerInterval;
    ProcessManager* processManager;

    void initializeDefaultHandlers();

public:
    InterruptManager();
    ~InterruptManager();

    // 中断注册和触发
    void registerInterrupt(int interruptNum, std::function<void()> handler);
    void triggerInterrupt(int interruptNum);
    void processAllInterrupts();

    // 具体中断处理函数
    void handleTimerInterrupt();
    void handleMemoryInterrupt();
    void handleKeyboardInterrupt();
    void handleIOInterrupt();
    void handleSystemCallInterrupt();

    // 定时器管理
    void enableTimer(int intervalMs = 100);
    void disableTimer();
    void setTimerInterval(int intervalMs);
    bool isTimerEnabled() const;

    // 进程管理器设置
    void setProcessManager(ProcessManager* pm);

    // 中断控制
    void setInterruptPriority(InterruptType type, int priority);
    void enableInterrupt(InterruptType type);
    void disableInterrupt(InterruptType type);

    // 获取组件引用
    InterruptController& getController();
    InterruptVectorTable& getVectorTable();
};

// ==================== 全局中断管理器（单例） ====================
class GlobalInterruptManager {
private:
    static std::unique_ptr<InterruptManager> instance;
    static std::mutex instanceMutex;

public:
    static InterruptManager& getInstance();
    static void initialize();
    static void cleanup();
};

#endif // INTERPUT_MNG_H
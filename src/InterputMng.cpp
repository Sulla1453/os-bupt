#include "../include/InterputMng.h"
#include "../include/ProcessManager.h"
#include <iostream>
#include <algorithm>

using namespace std;

// ==================== InterruptVectorTable 实现 ====================
InterruptVectorTable::InterruptVectorTable() {
    handlers.resize(INTERRUPT_COUNT);
    // 初始化默认空处理器
    for (size_t i = 0; i < handlers.size(); ++i) {
        handlers[i] = []() {
            cout << "[IVT] 未注册的中断处理程序被调用" << endl;
        };
    }
}

void InterruptVectorTable::registerHandler(InterruptType type, function<void()> handler) {
    if (type >= 0 && type < INTERRUPT_COUNT) {
        handlers[type] = handler;
        cout << "[IVT] 已注册中断类型 " << type << " 的处理程序" << endl;
    } else {
        cout << "[IVT] 错误：无效的中断类型 " << type << endl;
    }
}

void InterruptVectorTable::handleInterrupt(InterruptType type) {
    if (type >= 0 && type < INTERRUPT_COUNT && handlers[type]) {
        cout << "[IVT] 处理中断类型: " << type << endl;
        handlers[type]();
    } else {
        cout << "[IVT] 错误：未找到中断类型 " << type << " 的处理程序" << endl;
    }
}

// ==================== InterruptController 实现 ====================
InterruptController::InterruptController() {
    cout << "[中断控制器] 初始化完成" << endl;
}

void InterruptController::triggerInterrupt(InterruptType type) {
    lock_guard<mutex> lock(mtx);
    pendingInterrupts.push_back(type);
    cout << "[中断控制器] 触发中断: " << type << endl;
}

void InterruptController::processInterrupts(InterruptVectorTable& ivt) {
    lock_guard<mutex> lock(mtx);
    while (!pendingInterrupts.empty()) {
        InterruptType type = pendingInterrupts.front();
        pendingInterrupts.erase(pendingInterrupts.begin());
        
        // 暂时释放锁来处理中断（避免死锁）
        mtx.unlock();
        ivt.handleInterrupt(type);
        mtx.lock();
    }
}

bool InterruptController::hasPendingInterrupts() const {
    lock_guard<mutex> lock(mtx);
    return !pendingInterrupts.empty();
}

// ==================== Timer 实现 ====================
Timer::Timer(InterruptController& controller, int intervalMs) 
    : controller(controller), interval(intervalMs), running(false) {
    cout << "[计时器] 创建，间隔: " << intervalMs << "ms" << endl;
}

Timer::~Timer() {
    stop();
}

void Timer::start() {
    if (running) {
        cout << "[计时器] 警告：计时器已在运行" << endl;
        return;
    }
    
    running = true;
    timerThread = thread([this]() {
        cout << "[计时器] 启动计时器线程" << endl;
        while (running) {
            this_thread::sleep_for(chrono::milliseconds(interval));
            if (running) {  // 双重检查避免关闭时触发中断
                controller.triggerInterrupt(TIMER_INTERRUPT);
            }
        }
        cout << "[计时器] 计时器线程退出" << endl;
    });
}

void Timer::stop() {
    if (running) {
        cout << "[计时器] 停止计时器" << endl;
        running = false;
        if (timerThread.joinable()) {
            timerThread.join();
        }
    }
}

void Timer::setInterval(int intervalMs) {
    bool wasRunning = running;
    if (wasRunning) {
        stop();
    }
    interval = intervalMs;
    cout << "[计时器] 设置新间隔: " << intervalMs << "ms" << endl;
    if (wasRunning) {
        start();
    }
}

bool Timer::isRunning() const {
    return running;
}

// ==================== InterruptContext 实现 ====================
InterruptContext::InterruptContext() : processId(-1), programCounter(0), stackPointer(0) {}

void InterruptContext::saveContext(int pid, int pc, int sp) {
    processId = pid;
    programCounter = pc;
    stackPointer = sp;
    cout << "[上下文] 保存进程 " << pid << " 的上下文" << endl;
}

void InterruptContext::restoreContext() {
    cout << "[上下文] 恢复进程 " << processId << " 的上下文" << endl;
    // 在实际系统中，这里会恢复寄存器等状态
}

// ==================== InterruptManager 实现 ====================
InterruptManager::InterruptManager() : timerEnabled(false), timerInterval(100), processManager(nullptr) {
    ivt = make_unique<InterruptVectorTable>();
    controller = make_unique<InterruptController>();
    
    initializeDefaultHandlers();
    cout << "[中断管理器] 初始化完成" << endl;
}

InterruptManager::~InterruptManager() {
    if (timer) {
        timer->stop();
    }
    cout << "[中断管理器] 析构完成" << endl;
}

void InterruptManager::registerInterrupt(int interruptNum, function<void()> handler) {
    interruptVector[interruptNum] = handler;
    
    if (interruptNum >= 0 && interruptNum < INTERRUPT_COUNT) {
        ivt->registerHandler(static_cast<InterruptType>(interruptNum), handler);
    }
    
    cout << "[中断管理器] 注册中断 " << interruptNum << endl;
}

void InterruptManager::triggerInterrupt(int interruptNum) {
    if (interruptNum >= 0 && interruptNum < INTERRUPT_COUNT) {
        controller->triggerInterrupt(static_cast<InterruptType>(interruptNum));
    } else {
        cout << "[中断管理器] 错误：无效的中断号 " << interruptNum << endl;
    }
}

void InterruptManager::processAllInterrupts() {
    controller->processInterrupts(*ivt);
}

void InterruptManager::handleTimerInterrupt() {
    static int timerTicks = 0;
    timerTicks++;
    
    cout << "[定时器中断] Tick #" << timerTicks << endl;
    
    // 如果设置了进程管理器，执行时间片调度
    if (processManager) {
        cout << "[定时器中断] 执行时间片轮转调度" << endl;
        // 这里可以调用进程管理器的时间片处理方法
        processManager->handleTimeSlice();
    }
}

void InterruptManager::handleMemoryInterrupt() {
    cout << "[内存中断] 处理内存相关中断（页面错误、内存不足等）" << endl;
    
    if (processManager) {
        // 处理内存不足，可能需要进程换出
        cout << "[内存中断] 检查内存状态，可能需要换页" << endl;
    }
}

void InterruptManager::handleKeyboardInterrupt() {
    cout << "[键盘中断] 检测到键盘输入" << endl;
    // 在实际系统中，这里会读取键盘缓冲区
}

void InterruptManager::handleIOInterrupt() {
    cout << "[I/O中断] 处理I/O设备完成信号" << endl;
    
    if (processManager) {
        // 检查是否有进程在等待I/O完成
        cout << "[I/O中断] 检查等待I/O的进程" << endl;
        processManager->wakeIOWaitingProcesses();
    }
}

void InterruptManager::handleSystemCallInterrupt() {
    cout << "[系统调用中断] 处理系统调用请求" << endl;
}

void InterruptManager::enableTimer(int intervalMs) {
    if (timer) {
        timer->stop();
    }
    
    timerInterval = intervalMs;
    timer = make_unique<Timer>(*controller, intervalMs);
    timer->start();
    timerEnabled = true;
    
    cout << "[中断管理器] 启用计时器，间隔: " << intervalMs << "ms" << endl;
}

void InterruptManager::disableTimer() {
    if (timer) {
        timer->stop();
        timer.reset();
    }
    timerEnabled = false;
    cout << "[中断管理器] 禁用计时器" << endl;
}

void InterruptManager::setTimerInterval(int intervalMs) {
    timerInterval = intervalMs;
    if (timer) {
        timer->setInterval(intervalMs);
    }
}

bool InterruptManager::isTimerEnabled() const {
    return timerEnabled && timer && timer->isRunning();
}

void InterruptManager::setProcessManager(ProcessManager* pm) {
    processManager = pm;
    cout << "[中断管理器] 设置进程管理器" << endl;
}

void InterruptManager::initializeDefaultHandlers() {
    // 注册默认中断处理程序
    registerInterrupt(TIMER_INTERRUPT, [this]() { handleTimerInterrupt(); });
    registerInterrupt(KEYBOARD_INTERRUPT, [this]() { handleKeyboardInterrupt(); });
    registerInterrupt(IO_INTERPUT, [this]() { handleIOInterrupt(); });
    
    cout << "[中断管理器] 默认处理程序初始化完成" << endl;
}

void InterruptManager::setInterruptPriority(InterruptType type, int priority) {
    cout << "[中断管理器] 设置中断类型 " << type << " 的优先级为 " << priority << endl;
    // 在更复杂的实现中，这里会管理中断优先级队列
}

void InterruptManager::enableInterrupt(InterruptType type) {
    cout << "[中断管理器] 启用中断类型: " << type << endl;
}

void InterruptManager::disableInterrupt(InterruptType type) {
    cout << "[中断管理器] 禁用中断类型: " << type << endl;
}

InterruptController& InterruptManager::getController() {
    return *controller;
}

InterruptVectorTable& InterruptManager::getVectorTable() {
    return *ivt;
}

// ==================== GlobalInterruptManager 实现 ====================
unique_ptr<InterruptManager> GlobalInterruptManager::instance = nullptr;
mutex GlobalInterruptManager::instanceMutex;

InterruptManager& GlobalInterruptManager::getInstance() {
    lock_guard<mutex> lock(instanceMutex);
    if (!instance) {
        instance = make_unique<InterruptManager>();
    }
    return *instance;
}

void GlobalInterruptManager::initialize() {
    lock_guard<mutex> lock(instanceMutex);
    if (!instance) {
        instance = make_unique<InterruptManager>();
        cout << "[全局中断管理器] 初始化完成" << endl;
    }
}

void GlobalInterruptManager::cleanup() {
    lock_guard<mutex> lock(instanceMutex);
    if (instance) {
        instance.reset();
        cout << "[全局中断管理器] 清理完成" << endl;
    }
}


int main() {
    // 初始化全局中断管理器
    GlobalInterruptManager::initialize();
    InterruptManager& im = GlobalInterruptManager::getInstance();
    
    std::cout << "[测试] 手动触发中断" << std::endl;
    // 手动触发各类中断
    im.triggerInterrupt(TIMER_INTERRUPT);
    im.triggerInterrupt(KEYBOARD_INTERRUPT);
    im.triggerInterrupt(IO_INTERPUT);
    
    // 处理所有挂起的中断
    im.processAllInterrupts();
    
    std::cout << "[测试] 启动计时器自动中断" << std::endl;
    // 启动计时器，每200ms触发一次定时器中断
    im.enableTimer(2000);
    // 让定时器运行一段时间（例如1秒）
    std::this_thread::sleep_for(std::chrono::seconds(10));
    // 关闭定时器
    im.disableTimer();
    
    GlobalInterruptManager::cleanup();
    
    return 0;
}
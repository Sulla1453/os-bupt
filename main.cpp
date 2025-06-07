#include "Process/ProcessManager.h"
#include "Process/Process.h"
#include "File/FileSys.h"
#include <iostream>

using namespace std;

int main() {
    while(1){
        cout <<"Type which system you want:"<<endl;
        cout <<"1. File System"<<endl;
        cout <<"2. Process System"<<endl;
        cout <<"0. Exit"<<endl;
        int choice;
        cin >> choice;
        cin.ignore(); // 清除缓冲区
        if (choice == 1) {
            runFileSystemDemo(); // 调用文件系统演示
        } else if (choice == 2) {
            break; // 退出循环，进入进程系统
        } else if (choice == 0) {
            return 0; // 退出程序
        } else {
            cout << "Invalid choice, please try again." << endl;
        }
    }
    ProcessManager pm;

    // 示例进程创建
    cout << "=== Creating Processes ===" << endl;
    pm.createProcess(100, "P1", 4, 0, 2, 0, {});
    pm.createProcess(200, "P2", 3, 1, 1, 0, {});
    pm.createProcess(150, "P3", 5, 2, 3, 1, {});
    pm.createProcess(50, "P4", 2, 0, 2, 2, {});
    pm.createProcess(300, "P5", 1, 0, 1, 0, {});

    cout << "\nUse interactive mode? (y/n): ";
    char interactive;
    cin >> interactive;
    cin.ignore(); // 清除缓冲区

    cout << "\nSelect scheduling algorithm:\n";
    cout << "0 - SRTF (Shortest Remaining Time First)\n";
    cout << "1 - HRRN (Highest Response Ratio Next)\n";
    cout << "2 - FCFS (First Come First Served)\n";
    cout << "Choice: ";
    int method;
    cin >> method;
    cin.ignore(); // 清除缓冲区

    // 显示初始状态和资源需求
    cout << "\n=== Initial System State ===" << endl;
    pm.showAll();
    pm.showProcessesWithResources(); // 显示所有进程及其资源需求
    pm.showResourceRequirements();   // 显示资源需求摘要

    if (interactive == 'y' || interactive == 'Y') {
        pm.interactiveScheduler(method);
    } else {
        pm.runScheduler(method);
    }

    cout << "\n=== Final System State ===" << endl;
    pm.showAll();
    
    cout << "\nPress Enter to exit...";
    cin.get();

    return 0;
}
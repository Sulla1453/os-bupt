#ifndef FILESYS_H
#define FILESYS_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <sstream>

// 文件类型枚举
enum FileType {
    REGULAR_FILE,
    DIRECTORY
};

// 存储方式枚举
enum StorageType {
    CONTIGUOUS,  // 连续存储
    LINKED,      // 链接存储
    INDEXED      // 索引存储
};

// 文件控制块（FCB）
struct FCB {
    std::string name;           // 文件名
    FileType type;              // 文件类型
    int size;                   // 文件大小
    time_t createTime;          // 创建时间
    time_t modifyTime;          // 修改时间
    StorageType storageType;    // 存储方式
    int startBlock;             // 起始块号
    std::vector<int> blocks;    // 数据块列表（用于链接和索引存储）
    int permissions;            // 权限

    FCB(const std::string& n, FileType t, StorageType st = CONTIGUOUS);
};

// 磁盘块
struct DiskBlock {
    char data[512];             // 块数据
    int nextBlock;              // 下一个块号（用于链接存储）
    bool used;                  // 是否被使用

    DiskBlock();
};

// 目录项
struct DirectoryEntry {
    std::string name;
    std::shared_ptr<FCB> fcb;

    DirectoryEntry(const std::string& n, std::shared_ptr<FCB> f);
};

// 目录节点（树型结构）
class DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
public:
    std::shared_ptr<FCB> fcb;
    std::shared_ptr<DirectoryNode> parent;
    std::map<std::string, std::shared_ptr<DirectoryNode>> children;
    std::vector<DirectoryEntry> entries;

    DirectoryNode(const std::string& name, std::shared_ptr<DirectoryNode> p = nullptr);
    bool addChild(const std::string& name, std::shared_ptr<DirectoryNode> child);
    bool addFile(const std::string& name, std::shared_ptr<FCB> fileFcb);
    bool removeEntry(const std::string& name);
};

// 文件描述符
struct FileDescriptor {
    std::shared_ptr<FCB> fcb;
    int position;           // 当前读写位置
    bool isOpen;

    FileDescriptor();
    FileDescriptor(std::shared_ptr<FCB> f);
};

// 存储空间管理器
class StorageManager {
private:
    std::vector<DiskBlock> disk;
    std::set<int> freeBlocks;
    int totalBlocks;
public:
    StorageManager(int blocks = 1000);
    std::vector<int> allocateContiguous(int count);
    std::vector<int> allocateLinked(int count);
    std::vector<int> allocateIndexed(int count);
    void deallocate(const std::vector<int>& blocks);
    bool writeBlock(int blockNum, const char* data, int size);
    bool readBlock(int blockNum, char* data, int size);
    int getNextBlock(int blockNum);
    int getFreeBlockCount() const;
};

// 文件系统类
class FileSystem {
private:
    std::shared_ptr<DirectoryNode> root;
    std::shared_ptr<DirectoryNode> currentDir;
    StorageManager storage;
    std::map<int, FileDescriptor> openFiles;
    int nextFd;
    std::vector<std::string> parsePath(const std::string& path);
    std::shared_ptr<DirectoryNode> findDirectory(const std::string& path, bool createPath = false);
public:
    FileSystem();
    bool createDirectory(const std::string& path);
    bool createFile(const std::string& path, StorageType storageType = CONTIGUOUS);
    bool remove(const std::string& path);
    int openFile(const std::string& path);
    bool closeFile(int fd);
    int writeFile(int fd, const char* data, int size);
    int readFile(int fd, char* buffer, int size);
    bool changeDirectory(const std::string& path);
    void listDirectory(const std::string& path = ".");
    std::string getCurrentPath();
    void showStorageStats();
};

// 简单的命令行界面
void runFileSystemDemo();

#endif // FILESYS_H
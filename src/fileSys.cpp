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
    
    FCB(const std::string& n, FileType t, StorageType st = CONTIGUOUS) 
        : name(n), type(t), size(0), storageType(st), startBlock(-1), permissions(0755) {
        createTime = modifyTime = time(nullptr);
    }
};

// 磁盘块
struct DiskBlock {
    char data[512];           // 块数据
    int nextBlock;            // 下一个块号（用于链接存储）
    bool used;                // 是否被使用
    
    DiskBlock() : nextBlock(-1), used(false) {
        memset(data, 0, sizeof(data));
    }
};

// 目录项
struct DirectoryEntry {
    std::string name;
    std::shared_ptr<FCB> fcb;
    
    DirectoryEntry(const std::string& n, std::shared_ptr<FCB> f) 
        : name(n), fcb(f) {}
};

// 目录节点（树型结构）
class DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
public:
    std::shared_ptr<FCB> fcb;
    std::shared_ptr<DirectoryNode> parent;
    std::map<std::string, std::shared_ptr<DirectoryNode>> children;
    std::vector<DirectoryEntry> entries;
    
    DirectoryNode(const std::string& name, std::shared_ptr<DirectoryNode> p = nullptr) 
        : parent(p) {
        fcb = std::make_shared<FCB>(name, DIRECTORY);
    }
    
    // 添加子节点
    bool addChild(const std::string& name, std::shared_ptr<DirectoryNode> child) {
        if (children.find(name) != children.end()) {
            return false; // 已存在
        }
        children[name] = child;
        child->parent = shared_from_this();
        entries.emplace_back(name, child->fcb);
        return true;
    }
    
    // 添加文件条目
    bool addFile(const std::string& name, std::shared_ptr<FCB> fileFcb) {
        for (const auto& entry : entries) {
            if (entry.name == name) {
                return false; // 已存在
            }
        }
        entries.emplace_back(name, fileFcb);
        return true;
    }
    
    // 删除条目
    bool removeEntry(const std::string& name) {
        children.erase(name);
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [&name](const DirectoryEntry& entry) {
                    return entry.name == name;
                }),
            entries.end()
        );
        return true;
    }
};

// 使DirectoryNode支持shared_from_this
// class DirectoryNode : public std::enable_shared_from_this<DirectoryNode> {
//     // ... 之前的代码保持不变
// };

// 文件描述符
struct FileDescriptor {
    std::shared_ptr<FCB> fcb;
    int position;           // 当前读写位置
    bool isOpen;
    
    FileDescriptor() : fcb(nullptr), position(0), isOpen(false) {}
    FileDescriptor(std::shared_ptr<FCB> f) : fcb(f), position(0), isOpen(true) {}
};

// 存储空间管理器
class StorageManager {
private:
    std::vector<DiskBlock> disk;
    std::set<int> freeBlocks;
    int totalBlocks;
    
public:
    StorageManager(int blocks = 1000) : totalBlocks(blocks) {
        disk.resize(totalBlocks);
        for (int i = 0; i < totalBlocks; i++) {
            freeBlocks.insert(i);
        }
    }
    
    // 分配连续块
    std::vector<int> allocateContiguous(int count) {
        std::vector<int> allocated;
        if (freeBlocks.size() < count) return allocated;
        
        // 寻找连续的空闲块
        auto it = freeBlocks.begin();
        while (it != freeBlocks.end()) {
            int start = *it;
            bool canAllocate = true;
            
            // 检查是否有足够的连续块
            for (int i = 0; i < count; i++) {
                if (freeBlocks.find(start + i) == freeBlocks.end()) {
                    canAllocate = false;
                    break;
                }
            }
            
            if (canAllocate) {
                for (int i = 0; i < count; i++) {
                    allocated.push_back(start + i);
                    freeBlocks.erase(start + i);
                    disk[start + i].used = true;
                }
                break;
            }
            ++it;
        }
        
        return allocated;
    }
    
    // 分配链接块
    std::vector<int> allocateLinked(int count) {
        std::vector<int> allocated;
        if (freeBlocks.size() < count) return allocated;
        
        auto it = freeBlocks.begin();
        int prev = -1;
        
        for (int i = 0; i < count && it != freeBlocks.end(); i++) {
            int block = *it;
            allocated.push_back(block);
            freeBlocks.erase(it++);
            disk[block].used = true;
            
            if (prev != -1) {
                disk[prev].nextBlock = block;
            }
            prev = block;
        }
        
        return allocated;
    }
    
    // 分配索引块
    std::vector<int> allocateIndexed(int count) {
        std::vector<int> allocated;
        if (freeBlocks.size() < count + 1) return allocated; // +1 for index block
        
        // 分配索引块
        int indexBlock = *freeBlocks.begin();
        freeBlocks.erase(freeBlocks.begin());
        disk[indexBlock].used = true;
        allocated.push_back(indexBlock);
        
        // 分配数据块
        auto it = freeBlocks.begin();
        for (int i = 0; i < count && it != freeBlocks.end(); i++) {
            int block = *it;
            allocated.push_back(block);
            freeBlocks.erase(it++);
            disk[block].used = true;
        }
        
        return allocated;
    }
    
    // 释放块
    void deallocate(const std::vector<int>& blocks) {
        for (int block : blocks) {
            if (block >= 0 && block < totalBlocks) {
                disk[block].used = false;
                disk[block].nextBlock = -1;
                memset(disk[block].data, 0, sizeof(disk[block].data));
                freeBlocks.insert(block);
            }
        }
    }
    
    // 写数据到块
    bool writeBlock(int blockNum, const char* data, int size) {
        if (blockNum < 0 || blockNum >= totalBlocks || !disk[blockNum].used) {
            return false;
        }
        memcpy(disk[blockNum].data, data, std::min(size, 512));
        return true;
    }
    
    // 从块读数据
    bool readBlock(int blockNum, char* data, int size) {
        if (blockNum < 0 || blockNum >= totalBlocks || !disk[blockNum].used) {
            return false;
        }
        memcpy(data, disk[blockNum].data, std::min(size, 512));
        return true;
    }
    
    // 获取下一个块号
    int getNextBlock(int blockNum) {
        if (blockNum < 0 || blockNum >= totalBlocks) return -1;
        return disk[blockNum].nextBlock;
    }
    
    // 获取空闲块数量
    int getFreeBlockCount() const {
        return freeBlocks.size();
    }
};

// 文件系统类
class FileSystem {
private:
    std::shared_ptr<DirectoryNode> root;
    std::shared_ptr<DirectoryNode> currentDir;
    StorageManager storage;
    std::map<int, FileDescriptor> openFiles;
    int nextFd;
    
    // 解析路径
    std::vector<std::string> parsePath(const std::string& path) {
        std::vector<std::string> components;
        std::string current;
        
        for (char c : path) {
            if (c == '/') {
                if (!current.empty()) {
                    components.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        
        if (!current.empty()) {
            components.push_back(current);
        }
        
        return components;
    }
    
    // 根据路径查找目录节点
    std::shared_ptr<DirectoryNode> findDirectory(const std::string& path, bool createPath = false) {
        auto components = parsePath(path);
        auto current = (path[0] == '/') ? root : currentDir;
        
        for (const auto& component : components) {
            if (component == "..") {
                if (current->parent) {
                    current = current->parent;
                }
            } else if (component != ".") {
                auto it = current->children.find(component);
                if (it != current->children.end()) {
                    current = it->second;
                } else if (createPath) {
                    auto newDir = std::make_shared<DirectoryNode>(component);
                    current->addChild(component, newDir);
                    current = newDir;
                } else {
                    return nullptr;
                }
            }
        }
        
        return current;
    }
    
public:
    FileSystem() : storage(1000), nextFd(1) {
        root = std::make_shared<DirectoryNode>("/");
        currentDir = root;
    }
    
    // 创建目录
    bool createDirectory(const std::string& path) {
        auto lastSlash = path.find_last_of('/');
        std::string dirPath = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";
        std::string dirName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        auto parentDir = dirPath.empty() ? currentDir : findDirectory(dirPath);
        if (!parentDir) return false;
        
        auto newDir = std::make_shared<DirectoryNode>(dirName);
        return parentDir->addChild(dirName, newDir);
    }
    
    // 创建文件
    bool createFile(const std::string& path, StorageType storageType = CONTIGUOUS) {
        auto lastSlash = path.find_last_of('/');
        std::string dirPath = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";
        std::string fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        auto parentDir = dirPath.empty() ? currentDir : findDirectory(dirPath);
        if (!parentDir) return false;
        
        auto fileFcb = std::make_shared<FCB>(fileName, REGULAR_FILE, storageType);
        return parentDir->addFile(fileName, fileFcb);
    }
    
    // 删除文件或目录
    bool remove(const std::string& path) {
        auto lastSlash = path.find_last_of('/');
        std::string dirPath = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        auto parentDir = dirPath.empty() ? currentDir : findDirectory(dirPath);
        if (!parentDir) return false;
        
        // 查找要删除的条目
        for (const auto& entry : parentDir->entries) {
            if (entry.name == name) {
                // 如果是文件，释放其占用的存储空间
                if (entry.fcb->type == REGULAR_FILE && !entry.fcb->blocks.empty()) {
                    storage.deallocate(entry.fcb->blocks);
                }
                
                parentDir->removeEntry(name);
                return true;
            }
        }
        
        return false;
    }
    
    // 打开文件
    int openFile(const std::string& path) {
        auto lastSlash = path.find_last_of('/');
        std::string dirPath = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";
        std::string fileName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        auto parentDir = dirPath.empty() ? currentDir : findDirectory(dirPath);
        if (!parentDir) return -1;
        
        // 查找文件
        for (const auto& entry : parentDir->entries) {
            if (entry.name == fileName && entry.fcb->type == REGULAR_FILE) {
                int fd = nextFd++;
                openFiles[fd] = FileDescriptor(entry.fcb);
                return fd;
            }
        }
        
        return -1;
    }
    
    // 关闭文件
    bool closeFile(int fd) {
        auto it = openFiles.find(fd);
        if (it != openFiles.end()) {
            it->second.isOpen = false;
            openFiles.erase(it);
            return true;
        }
        return false;
    }
    
    // 写文件
    int writeFile(int fd, const char* data, int size) {
        auto it = openFiles.find(fd);
        if (it == openFiles.end() || !it->second.isOpen) return -1;
        
        auto& file = it->second;
        auto fcb = file.fcb;
        
        // 计算需要的块数
        int blocksNeeded = (size + 511) / 512; // 向上取整
        
        // 如果文件还没有分配存储空间，现在分配
        if (fcb->blocks.empty()) {
            std::vector<int> allocated;
            
            switch (fcb->storageType) {
                case CONTIGUOUS:
                    allocated = storage.allocateContiguous(blocksNeeded);
                    break;
                case LINKED:
                    allocated = storage.allocateLinked(blocksNeeded);
                    break;
                case INDEXED:
                    allocated = storage.allocateIndexed(blocksNeeded);
                    break;
            }
            
            if (allocated.empty()) return -1; // 分配失败
            
            fcb->blocks = allocated;
            fcb->startBlock = allocated[0];
        }
        
        // 写数据到块
        int bytesWritten = 0;
        int dataOffset = 0;
        
        for (int i = 1; i < fcb->blocks.size() && dataOffset < size; i++) { // 跳过索引块（如果是索引存储）
            int blockNum = fcb->blocks[i];
            int blockOffset = (fcb->storageType == INDEXED) ? i - 1 : i;
            int bytesToWrite = std::min(512, size - dataOffset);
            
            if (storage.writeBlock(blockNum, data + dataOffset, bytesToWrite)) {
                bytesWritten += bytesToWrite;
                dataOffset += bytesToWrite;
            }
        }
        
        fcb->size = std::max(fcb->size, file.position + bytesWritten);
        fcb->modifyTime = time(nullptr);
        file.position += bytesWritten;
        
        return bytesWritten;
    }
    
    // 读文件
    int readFile(int fd, char* buffer, int size) {
        auto it = openFiles.find(fd);
        if (it == openFiles.end() || !it->second.isOpen) return -1;
        
        auto& file = it->second;
        auto fcb = file.fcb;
        
        if (fcb->blocks.empty() || file.position >= fcb->size) return 0;
        
        int bytesToRead = std::min(size, fcb->size - file.position);
        int bytesRead = 0;
        int bufferOffset = 0;
        
        // 根据存储类型读取数据
        for (int i = (fcb->storageType == INDEXED ? 1 : 0); 
             i < fcb->blocks.size() && bytesRead < bytesToRead; i++) {
            int blockNum = fcb->blocks[i];
            char blockData[512];
            
            if (storage.readBlock(blockNum, blockData, 512)) {
                int blockBytesToRead = std::min(512, bytesToRead - bytesRead);
                memcpy(buffer + bufferOffset, blockData, blockBytesToRead);
                bytesRead += blockBytesToRead;
                bufferOffset += blockBytesToRead;
            }
        }
        
        file.position += bytesRead;
        return bytesRead;
    }
    
    // 改变当前目录
    bool changeDirectory(const std::string& path) {
        auto newDir = findDirectory(path);
        if (newDir) {
            currentDir = newDir;
            return true;
        }
        return false;
    }
    
    // 列出目录内容
    void listDirectory(const std::string& path = ".") {
        auto dir = (path == ".") ? currentDir : findDirectory(path);
        if (!dir) {
            std::cout << "Directory not found: " << path << std::endl;
            return;
        }
        
        std::cout << "Directory: " << dir->fcb->name << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        for (const auto& entry : dir->entries) {
            std::cout << (entry.fcb->type == DIRECTORY ? "DIR" : "FILE") 
                      << "\t" << entry.name 
                      << "\t" << entry.fcb->size << " bytes"
                      << "\t" << ctime(&entry.fcb->modifyTime);
        }
    }
    
    // 获取当前目录路径
    std::string getCurrentPath() {
        std::string path;
        auto current = currentDir;
        std::vector<std::string> components;
        
        while (current && current != root) {
            components.push_back(current->fcb->name);
            current = current->parent;
        }
        
        if (components.empty()) {
            return "/";
        }
        
        std::reverse(components.begin(), components.end());
        for (const auto& comp : components) {
            path += "/" + comp;
        }
        
        return path;
    }
    
    // 显示存储统计信息
    void showStorageStats() {
        std::cout << "Storage Statistics:" << std::endl;
        std::cout << "Free blocks: " << storage.getFreeBlockCount() << std::endl;
        std::cout << "Used blocks: " << (1000 - storage.getFreeBlockCount()) << std::endl;
    }
};

// 简单的命令行界面
void runFileSystemDemo() {
    FileSystem fs;
    std::string command;
    
    std::cout << "Simple File System Demo" << std::endl;
    std::cout << "Commands: mkdir, touch, ls, cd, rm, open, write, read, close, pwd, stats, quit" << std::endl;
    
    while (true) {
        std::cout << fs.getCurrentPath() << "$ ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "mkdir") {
            std::string path;
            iss >> path;
            if (fs.createDirectory(path)) {
                std::cout << "Directory created: " << path << std::endl;
            } else {
                std::cout << "Failed to create directory: " << path << std::endl;
            }
        } else if (cmd == "touch") {
            std::string path;
            iss >> path;
            if (fs.createFile(path)) {
                std::cout << "File created: " << path << std::endl;
            } else {
                std::cout << "Failed to create file: " << path << std::endl;
            }
        } else if (cmd == "ls") {
            std::string path = ".";
            iss >> path;
            fs.listDirectory(path);
        } else if (cmd == "cd") {
            std::string path;
            iss >> path;
            if (!fs.changeDirectory(path)) {
                std::cout << "Directory not found: " << path << std::endl;
            }
        } else if (cmd == "rm") {
            std::string path;
            iss >> path;
            if (fs.remove(path)) {
                std::cout << "Removed: " << path << std::endl;
            } else {
                std::cout << "Failed to remove: " << path << std::endl;
            }
        } else if (cmd == "pwd") {
            std::cout << fs.getCurrentPath() << std::endl;
        } else if (cmd == "stats") {
            fs.showStorageStats();
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }
}

int main() {
    runFileSystemDemo();
    return 0;
}
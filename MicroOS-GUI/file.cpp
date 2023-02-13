#pragma once
#include "file.h"

//磁盘（文件形式）
fstream disk;

// 文件分配表项 每一项为16位
typedef unsigned short fid;

// 系统级打开文件表
vector<softEntry> soft; 

// 进程打开文件表
vector<poft> pofts; 

// 虚拟内存
vmEntry vm[VMSIZE]; 

QMutex fileLock;
// 目录逻辑上的二叉树型结构，使用向量下标连接同级文件（目录），通过同级目录起始块号（FCB中的startBlock）连接下一级目录内容
typedef vector<fcb> directory; 

// 当前工作目录名
string tempDirName;

// 当前工作目录
directory tempDir;

// FAT表
fid fat[FATENTRYNUM];


void create_disk() {
    ofstream ofs;
    ofs.open(DISKNAME, ios::out | ios::binary);
    if (ofs.is_open())
    {
        char a = 0x0;
        for (int i = 0; i < DISKSIZE; i++)
        {
            ofs.write((char*)&a, sizeof(char));
        }
        ofs.close();
        qDebug() << "success to create disk" << endl;
    }
    else
    {
        qDebug() << "fail to create disk" << endl;
    }
}

//进入文件系统 启动引导区 默认进入根目录 初始化文件分配表和虚拟内存
void start_fileSystem()
{
    //第一步：检查磁盘（以二进制方式打开disk文件）

    ifstream check;
    check.open(DISKNAME, ios::binary | ios::in);

    if (!check.good()) {
        create_disk();
        check.open(DISKNAME, ios::binary | ios::in);
    }

    if (!check.is_open()){
        qDebug() << "Can not find disk!" << endl;
        exit(-1);
    }
    else //验证成功，装载并进入后续操作
    {
        check.close();
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        //第二步，验证引导区，即验证首字节是否非0
        unsigned char boot;
        disk.read((char*)&boot, sizeof(unsigned char));
        if (boot == 0x0) //首字节为0，引导区验证失败
        {
            qDebug() << "BS block error" << endl;
            //询问用户是否要格式化磁盘
            init_disk();
            //格式化完成，弹出磁盘，重新启动文件系统
            disk.close();
            start_fileSystem();
        }
        else //首字节不为0 验证成功
        {
            disk.close();
            //读入FAT1，改变当前工作目录为根目录（cd root），读入根目录表项
            init_fat();
            tempDirName = "root";
            cd("root");
            init_vm();
        }
    }
}

// 格式化磁盘
void init_disk()
{
    //需要格式化磁盘的情况：引导区损坏
    //先全部清零磁盘，更改引导区首字符,更新FAT表，将将引导区，保留区，FAT和根目录块标记为占用,初始化根目录

    int offset;

    //清零磁盘
    offset = 0;
    disk.seekp(offset, disk.beg);
    unsigned char init = 0x0;
    for (int i = 0; i < DISKSIZE; i++)
    {
        disk.write((char*)&init, sizeof(unsigned char));
    }

    //更改引导区首字符:将文件流定位到首字符：

    //首字符偏移量是0
    offset = 0;
    disk.seekp(offset, disk.beg);
    unsigned char boot = 0x1;
    disk.write((char*)&boot, sizeof(unsigned char));

    //更新FAT表，将引导区，保留区，FAT和根目录块标记为占用
    offset = (1 + 1) * BLOCKSIZE;
    disk.seekp(offset, disk.beg);
    fid f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid));  //根目录：占用，结束块
    disk.write((char*)&f, sizeof(fid));  //引导区：占用，结束块
    f = 2;                                // FAT1表起始块
    for (int i = 0; i < FATSIZE - 1; i++) //前127块FAT1表项中对应加一
    {
        f++;
        disk.write((char*)&f, sizeof(fid));
    }
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid));  //最后一块FAT1对应表项记录为结束标志
    f = 2 + FATSIZE;                      // FAT2起始块
    for (int i = 0; i < FATSIZE - 1; i++) //前127块FAT12表项中对应加一
    {
        f++;
        disk.write((char*)&f, sizeof(fid));
    }
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid)); //最后一块FAT2对应表项记录为结束标志
    f = 1 + FIRSTROOTBLOCK;
    disk.write((char*)&f, sizeof(fid)); //根目录区第一块
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid)); //根目录区结束块

    //初始化根目录
    //定位到根目录首字符,制作根目录的目录项.
    offset = BLOCKSIZE * FIRSTROOTBLOCK;
    disk.seekp(offset, disk.beg);
    time_t lt;
    lt = time(NULL);
    unsigned int ctime = (unsigned int)lt;
    unsigned int utime = (unsigned int)lt;
    fcb root = {
        ".", 2, 0, FIRSTROOTBLOCK, 2, (unsigned int)sizeof(fcb), ctime, utime };
    //将'.'目录项写入磁盘根目录区
    disk.write((char*)&root, sizeof(fcb));

    qDebug() << "success to format your disk" << endl;
}

// 初始化文件分配表，从FAT1起始块读入整个文件分配表
void init_fat()
{

    int offset = FIRSTFATBLOCK * BLOCKSIZE;
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    disk.seekg(offset, disk.beg);
    for (int i = 0; i < FATENTRYNUM; i++)
    {
        disk.read((char*)&fat[i], sizeof(fid));
    }
    disk.close();
    qDebug() << "inital fat success" << endl;
}

int cd(string dirName)
{

    int flag = 1;
    // 0为失败,1为成功
    if (dirName.size() != 0)
    {
        // 对目录名称进行切片，存储在dirList中
        vector<string> dirList;
        // 切片后判断第一个字符串是否为root，是的话就是绝对路径，不是的话就是相对路径
        splitStr(dirName, dirList, "/");
        // 更新当前目录
        for (int i = 0; i < dirList.size(); i++)
        {
            flag = load_dirEntry(dirList[i]);
            if (flag == 0) //更新失败
            {
                break;
            }
        }
        if (flag == 1) //更新成功，修改当前工作目录名
        {
            if (dirList[0].compare("root") == 0) // 绝对路径直接替换
            {
                tempDirName = dirName;
            }
            else if (dirList[0].compare(".") == 0)
            {
                tempDirName = tempDirName;
            }
            else if (dirList[0].compare("..") == 0)
            {
                //工作目录返回上一级
                vector<string> name;
                splitStr(tempDirName, name, "/");
                string newName;
                int i = 0;
                for (i = 0; i < name.size() - 2; i++)
                {
                    newName += name[i] + "/";
                }
                newName += name[i];
                tempDirName = newName;
            }
            else // 相对路径进行拼接
            {
                tempDirName = tempDirName + '/' + dirName;
            }
        }
    }
    return flag;
}

int load_dirEntry(string dirName)
{
    int flag = 1;
    // 0失败，1成功
    // 一种是加载根目录，这种情况直接从根目录区读入就行
    if (dirName.compare("root") == 0)
    {
        // 这种情况需要先清空当前目录
        vector<fcb>().swap(tempDir);
        int offset = FIRSTROOTBLOCK * BLOCKSIZE;
        // 进行文件流定位
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekg(offset, disk.beg);
        fcb root;
        disk.read((char*)&root, sizeof(fcb));
        // 读入.目录项,规定目录块第一个fcb是.
        tempDir.push_back(root);
        // 将该目录项作为录项首项
        int entryNum = (int)root.size / 32;
        for (int i = 0; i < entryNum - 1; i++)
        {
            // 继续从磁盘中读出一个fcb,直到结束
            disk.read((char*)&root, sizeof(fcb));
            // 将fcb追加到目录项表
            tempDir.push_back(root);
        }
        disk.close();

    }

    // 另一种是加载目录，首先再当前的目录项链表中找有没有名称对应的目录项，
    // 找到了，进行文件流定位，读入.目录项，根据.目录项的大小，来判断需要装载的项数，然后更新目录项链表即可
    else
    {
        int j = -1;
        // 首先根据目录名在当前目录中找是否有匹配项
        for (int i = 0; i < tempDir.size(); i++)
        {
            // 存在目录项，属性为目录，权限可读或可写
            if (dirName.compare(tempDir[i].fileName) == 0 && tempDir[i].attribute == (unsigned char)0 && tempDir[i].rights > (char)0)
            {
                j = i;
                break;
            }
        }
        if (j >= 0)
        {
            // 找到该目录的起始块号和大小
            unsigned short startBlock = tempDir[j].startBlock;
            // 这种情况需要先清空当前目录
            vector<fcb>().swap(tempDir);
            int offset = startBlock * BLOCKSIZE;
            // 先判断是否存在.目录项
            disk.open(DISKNAME, ios::binary | ios::in | ios::out);
            disk.seekg(offset, disk.beg);
            fcb itself;
            disk.read((char*)&itself, sizeof(fcb));
            unsigned int size = itself.size;
            // 存在则加载到当前目录
            if (size > 0)
            {
                tempDir.push_back(itself);
                for (int i = 0; i < size / sizeof(fcb) - 1; i++)
                {
                    fcb dirEntry;
                    disk.read((char*)&dirEntry, sizeof(fcb));
                    tempDir.push_back(dirEntry);
                }
            }
            // 不存在则当前目录为空，仅当需要创建新目录事出现这种情况
            disk.close();
        }
        else
        {
            // 不存在该目录项
            flag = 0;
        }
    }
    return flag;
}

void ls(string dirName, stringstream &output)
{
    // 先输出当前目录
    output <<"temp directory:"<< tempDirName.c_str() << endl;
    if (dirName.compare("") == 0)
    {
        for (int i = 0; i < tempDir.size(); i++)
        {
            print_fcb(tempDir[i], output);
        }
    }
    // 指定目录
    else
    {
        // 保存当前目录名
        string tempName = tempDirName;
        int i = cd(dirName);
        if (i > 0) // 找到并跳转指定目录成功
        {
            for (int i = 0; i < tempDir.size(); i++)
            {
                print_fcb(tempDir[i], output);
            }
        }
        else //没找到
        {
            output << "no such directory" << endl;
        }
        // 跳回原来目录
        cd(tempName);
    }
}

int mkdir(string dirName)
{
    // 保存当前目录名
    string rowName = tempDirName;
    int flag = 1;
    // 0为失败,1为成功
    // 对目录名称进行切片，存储在dirList中
    vector<string> dirList;
    // 切片后判断第一个字符串是否为root，是的话就是绝对路径，不是的话就是相对路径
    splitStr(dirName, dirList, "/");
    if (dirList[0].compare("root") == 0) // 绝对路径，改造成相对root的相对路径
    {
        cd("root");
        vector<string>::iterator k = dirList.begin();
        dirList.erase(k); // 删除第一个字符串root
    }
    int j = 1;
    int k = -1;
    for (int i = 0; i < dirList.size(); i++) // 定位到第一处需要生成目录的地方
    {
        j = cd(dirList[i]);
        if (j == 1) // 存在这个目录，继续即可
        {
            continue;
        }
        else // 不存在这个目录，记录上一级下标，结束循环，完成定位
        {
            k = i;
            break;
        }
    }
    if (k == -1) // 说明该目录已存在
    {
        qDebug() << "There is already a directory named:" << dirName.c_str() << endl;
        flag = 0;
    }
    else
    {
        for (int i = k; i < dirList.size(); i++)
        {
            //逐级地创建目录文件
            //每次创建分为6步：1申请物理块，2在父目录创建对应名称的目录项，3修改父目录.目录项的大小(size,utime)
            // 4在申请到的物理块添加.项 5在申请的物理块添加目录项.. 6修改上二级目录对应的父目录的目录项(size,utime),如果当前目录就是root省略这一步
            //每次创建成功后进到下一级目录

            // 1申请物理块
            vector<unsigned short> blocks;
            get_blocks(1, blocks);

            // 2在父目录创建对应名称的目录项
            // 制作目录项
            // 目录的目录项权限为可写，属性为目录，默认长度1Block，默认大小64B(包含.和..两个目录项)
            time_t lt;
            lt = time(NULL);
            string fileName = dirList[i];
            char rights = RW;
            unsigned char attribute = DIR;
            unsigned short startBlock = blocks[0];
            unsigned short length = blocks.size();
            unsigned int size = 2 * FCBSIZE;
            unsigned int ctime = (unsigned int)lt;
            unsigned int utime = (unsigned int)lt;
            // 不能用string来初始化字符数组，所以文件名先填空，再更改
            fcb newEntry = {
                "", rights, attribute, startBlock, length, size, ctime, utime };
            strcpy(newEntry.fileName, fileName.c_str());
            flag = create_dir_entry("", newEntry);
            if (flag == 0)
            {
                qDebug() << "fail to mkdir: " << dirList[i].c_str() << endl;
                break;
            }

            // 3修改父目录.目录项的大小(size,utime)
            fcb itself;
            int location = get_dir_entry_location(".");
            itself = tempDir[location];
            itself.size = itself.size + FCBSIZE;
            itself.updateTime = utime;
            flag = update_dir_entry("", itself);
            if (flag == 0)
            {
                qDebug() << "fail to update: ." << endl;
                break;
            }

            // 4在申请到的物理块添加.项
            // 生成.项
            strcpy(newEntry.fileName, ".");
            flag = create_dir_entry(fileName, newEntry);
            if (flag == 0)
            {
                qDebug() << "fail to create: ." << endl;
                break;
            }

            // 5在申请到的物理块添加..项
            fcb father = itself;
            strcpy(father.fileName, "..");
            flag = create_dir_entry(fileName, father);
            if (flag == 0)
            {
                qDebug() << "fail to create: .." << endl;
                break;
            }

            // 6修改上二级目录对应的父目录的目录项(size,utime),如果当前目录就是root省略这一步
            if (tempDirName.compare("root") == 0) // 当前目录是root,跳过第六步
            {
                continue;
            }
            else
            {
                string fatherName = get_temp_dir_name();
                cd("..");
                location = get_dir_entry_location(fatherName);
                father = tempDir[location];
                //修改size和utime
                father.size += FCBSIZE;
                father.updateTime = utime;
                flag = update_dir_entry("", father);
                if (flag == 0)
                {
                    qDebug() << "fail to update father entry" << endl;
                    break;
                }
                cd(fatherName);
            }
            cd(fileName);
        }
    }
    cd(rowName);
    return flag;
}

int rmdir(string dirName)
{
    int flag = 1;
    // 简化起见，不做递归地删除
    // 当目录下有文件或目录时，不允许删除
    // 允许跨级删除
    // 允许相对路径和绝对路径
    string rowName = tempDirName;
    flag = cd(dirName);
    // 存在这个目录才能删除
    if (dirName.compare("root") == 0)
    {
        flag = 0; // 不允许删除根目录
    }
    if (flag > 0)
    {
        if (tempDir.size() > 2) // 只有两个目录项才允许删除
        {
            flag = 0;
        }
        if (flag > 0)
        {
            // 允许删除
            // 获取.的长度和起始块号和需要删除的目录名（不带斜线）
            int location = get_dir_entry_location(".");
            unsigned short length = tempDir[location].length;
            unsigned short startBlock = tempDir[location].startBlock;
            string entryName = get_temp_dir_name();
            // 1释放物理块 2删除父目录中相应的目录项 3修改父目录.目录项的内容(size和utime) 4修改上二级目录中父目录对应的目录项的内容(size和utime)（如果删除的是root目录下的目录则跳过这一步）
            cd(".."); // 先返回上一级，防止删除目录后无法返回

            vector<unsigned short> addr;
            // 获取fat表项下标
            flag = get_fats(startBlock, addr, length);
            if (flag > 0)
            {
                // 1根据长度和起始块号修改fat表
                // 生成fat表项内容
                vector<unsigned short> nextBlock;
                for (int i = 0; i < addr.size(); i++)
                { // 因为是释放物理块，所以直接赋值为空闲即可
                    nextBlock.push_back(FREEBLOCK);
                }
                update_fat(addr, nextBlock);

                // 2删除父目录中相应的目录项
                // 已经在父目录下，只要调用删除目录项函数即可
                delete_dir_entry(entryName);

                // 3修改父目录.目录项的内容
                location = get_dir_entry_location(".");
                fcb newSelf = tempDir[location];
                newSelf.size = newSelf.size - FCBSIZE;
                time_t lt;
                lt = time(NULL);
                unsigned int utime = (unsigned int)lt;
                newSelf.updateTime = utime;
                update_dir_entry(".", newSelf);

                // 4修改上二级目录中父目录对应的目录项的内容(size和utime)（如果删除的是root目录下的目录则跳过这一步）
                if (tempDirName.compare("root") != 0) // 不是root目录
                {
                    string fatherName = get_temp_dir_name();
                    cd("..");
                    location = get_dir_entry_location(fatherName);
                    fcb father = tempDir[location];
                    //修改size和utime
                    father.size -= FCBSIZE;
                    father.updateTime = utime;
                    flag = update_dir_entry("", father);
                    if (flag == 0)
                    {
                        qDebug() << "fail to update father entry" << endl;
                    }
                    cd(fatherName);
                }
            }
        }
    }
    else
    {
        qDebug() << "no such directory" << endl;
    }
    cd(rowName);
    return flag;
}

// 创建文件
int create_file(string fileName) 
{
    // 不允许跨级创建
    // 不允许创建同名文件
    // 仅当当前目录有可容纳多余FCB时创建
    int flag = 1;
    string rowName = tempDirName;
    // 判断文件名是否合规
    if (fileName.size() >= 14) //长度不超过13字节
    {
        flag = 0;
    }
    vector<string> list;
    splitStr(fileName, list, "/"); //不允许跨级
    if (list.size() > 1)
    {
        flag = 0;
    }
    int j = get_dir_entry_location(fileName);
    if (j >= 0) //不允许同名
    {
        flag = 0;
    }
    if (tempDir.size() >= 32) //不能在目录块没法容纳fcb时创建
    {
        flag = 0;
    }

    // 1申请一块物理块 2生成文件的fcb 3.在当前目录下新建该文件的目录项 4修改当前目录.目录项的内容（size，utime） 5.修改上二级目录父目录对应的目录项内容（当前目录若是根目录，则跳过）(size,utime)

    if (flag > 0)
    {
        // 1申请一块物理块
        vector<unsigned short> blocks;
        get_blocks(1, blocks);

        // 2生成文件的fcb
        char rights = RW;
        unsigned char attribute = COMMONFILE;
        unsigned short startBlock = blocks[0];
        unsigned short length = 1;
        unsigned int size = 0;
        time_t lt;
        lt = time(NULL);
        unsigned int ctime = (unsigned int)lt;
        unsigned int utime = (unsigned int)lt;
        fcb newFile = { "", rights, attribute, startBlock, length, size, ctime, utime };
        strcpy(newFile.fileName, fileName.c_str());

        // 3在当前目录下新建该文件的目录项
        flag = create_dir_entry("", newFile);

        // 4修改当前目录.目录项的内容（size，utime）
        int location = get_dir_entry_location(".");
        fcb father = tempDir[location];
        father.size += FCBSIZE;
        father.updateTime = utime;
        flag = update_dir_entry("", father);

        // 5.修改上二级目录父目录对应的目录项内容（当前目录若是根目录，则跳过）(size,utime)
        string fatherName = get_temp_dir_name();
        if (fatherName.compare("root") != 0)
        {
            string fatherName = get_temp_dir_name();
            cd(rowName);
            cd("..");
            location = get_dir_entry_location(fatherName);
            father = tempDir[location];
            //修改size和utime
            father.size += FCBSIZE;
            father.updateTime = utime;
            flag = update_dir_entry("", father);
            if (flag == 0)
            {
                qDebug() << "fail to update father entry" << endl;
            }
            cd(fatherName);
        }
    }

    cd(rowName);
    return flag;
}

// 删除文件
int rm(string fileName) 
{
    int flag = 1;
    string rowName = tempDirName;

    //不允许跨级删除
    vector<string> list;
    splitStr(fileName, list, "/"); 
    if (list.size() > 1)
    {
        flag = 0;
    }

    int location = get_dir_entry_location(fileName); // 存在该目录项才允许删除
    if (location < 0)
    {
        flag = 0;
    }
    // 1释放物理块 2当前目录下对应目录项删除 3当前目录.目录项修改（size和utime） 4上级目录中对应父目录的目录项修改（size和utime)根目录则不做操作
    if (flag > 0)
    {
        // 1释放物理块
        // 定位物理块和长度
        fcb target = tempDir[location];
        unsigned short startBlock = target.startBlock;
        unsigned short length = target.length;
        // 获取所有需要释放的物理块
        vector<unsigned short> addr;
        flag = get_fats(startBlock, addr, length);
        // 生成空闲标志
        vector<unsigned short> nextBlock;
        for (int i = 0; i < addr.size(); i++)
        {
            nextBlock.push_back(FREEBLOCK);
        }
        // 释放物理块
        update_fat(addr, nextBlock);

        // 2当前目录下对应目录项删除
        string targetName = target.fileName;
        flag = delete_dir_entry(targetName);

        // 3当前目录.目录项修改(size和utime)
        int fatherLocation = get_dir_entry_location(".");
        fcb father = tempDir[fatherLocation];
        father.size -= FCBSIZE;
        time_t lt;
        lt = time(NULL);
        unsigned int utime = (unsigned int)lt;
        father.updateTime = utime;
        flag = update_dir_entry("", father);

        // 4上级目录中对应父目录的目录项修改（size和utime)
        string fatherName = get_temp_dir_name();
        if (fatherName.compare("root") != 0)
        {
            string fatherName = get_temp_dir_name();
            cd(rowName);
            cd("..");
            location = get_dir_entry_location(fatherName);
            father = tempDir[location];
            //修改size和utime
            father.size -= FCBSIZE;
            father.updateTime = utime;
            flag = update_dir_entry("", father);
            if (flag == 0)
            {
                qDebug() << "fail to update father entry" << endl;
            }
            cd(fatherName);
        }
    }
    cd(rowName);
    return flag;
}

// 创建目录项  
int create_dir_entry(string dirName, fcb dirEntry) 
{
    // 分为创建.目录项和创建其他目录项
    qDebug() << "make entry" << endl;
    int flag = 1;
    string tempName = tempDirName;
    // 获取当前目录的起始块号
    // 如果当前目录是空的，一个目录项也没有，那么只能创建.目录项，起始块号从.目录项中获得
    int temBlock;
    int temDirSize;
    int offset;
    // 该目录需要有剩余空间
    if (tempDir.size() >= 32)
    {
        flag = 0;
    }
    if (strcmp(dirEntry.fileName, ".") == 0 && flag == 1) // 创建.目录项
    {
        temBlock = dirEntry.startBlock;
        temDirSize = 0;
        offset = temBlock * BLOCKSIZE + temDirSize * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
    }
    else if (strcmp(dirEntry.fileName, "..") == 0 && flag == 1) // 创建..目录项
    {
        cd(dirName);
        temBlock = tempDir[0].startBlock;
        temDirSize = 1;
        offset = temBlock * BLOCKSIZE + temDirSize * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
    }
    else if (flag == 1) // 创建其他目录项
    {
        cd(dirName);
        temBlock = tempDir[0].startBlock;
        temDirSize = tempDir.size();
        offset = temBlock * BLOCKSIZE + temDirSize * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
        tempDir.push_back(dirEntry);
    }
    else
    {
        qDebug() << "error:maybe block is damaged." << endl;
        flag = 0;
    }
    cd(tempName);
    return flag;
}

// 更新某目录项
int update_dir_entry(string dirName, fcb dirEntry) 
{
    int flag = 1;
    string tempName = tempDirName;
    cd(dirName);
    string name = dirEntry.fileName;
    int location = get_dir_entry_location(name);
    if (location >= 0)
    {
        // 写磁盘
        int tempBlock = tempDir[0].startBlock;
        int offset = tempBlock * BLOCKSIZE + location * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
        // 更新目录项
        tempDir[location] = dirEntry;
    }
    else
    {
        flag = 0;
    }

    cd(tempName);
    return flag;
}

// 删除某一目录项
int delete_dir_entry(string entryName) 
{
    int flag = 1;
    // 不允许删除 . 和 .. 目录项
    if (entryName.compare(".") == 0 || entryName.compare("..") == 0) 
    {
        flag = 0;
    }
    else
    {
        // 可以删除的情况
        int location = get_dir_entry_location(entryName);
        if (location >= 0)
        {
            // 从当前工作目录移除相应的目录项
            tempDir.erase(tempDir.begin() + location);
            // 将当前工作目录复写回磁盘
            // 定位
            location = get_dir_entry_location(".");
            unsigned short startBlock = tempDir[location].startBlock;
            int offset = startBlock * BLOCKSIZE;
            disk.open(DISKNAME, ios::binary | ios::in | ios::out);
            disk.seekp(offset, disk.beg);
            for (int i = 0; i < tempDir.size(); i++)
            {
                fcb dirEntry = tempDir[i];
                disk.write((char*)&dirEntry, sizeof(fcb));
            }
            disk.close();
        }
    }
    return flag;
}

// 申请物理块，返回值是物理块号的数组,申请成功，需要更新新文件分配表
void get_blocks(int blockSize, vector<unsigned short>& getBlocks) 
{
    // 顺序访问fat表，选取相应数量的物理块
    int i = 0;
    for (int j = 0; j < FATENTRYNUM; j++)
    {
        if (fat[j] == 0) // 对应表项空闲
        {
            getBlocks.push_back(j);
            i++;
        }
        if (i >= blockSize)
        {
            break;
        }
    }

    // 更新fat表项:地址(getBlocks)，下一块地址(nextBlock)
    vector<unsigned short> nextBlock;
    for (int i = 0; i < getBlocks.size() - 1; i++)
    {
        nextBlock.push_back(getBlocks[i + 1]);
    }
    // 标记最后一块的nextBlock为占用
    nextBlock.push_back(FENDFLAG);
    update_fat(getBlocks, nextBlock);
}

void update_fat(vector<unsigned short> addr, vector<unsigned short> nextBlock)
{
    if (addr.size() != nextBlock.size() || addr.size() < 1)
    {
        qDebug() << "fail to update fat" << endl;
    }
    else
    {
        for (int i = 0; i < addr.size(); i++)
        {
            fat[addr[i]] = nextBlock[i];
        }
    }
}

// 获取文件所有的块号
int get_fats(unsigned short firstBlock, vector<unsigned short>& addr, unsigned short blockSize) 
{
    int flag = 1;
    unsigned short nextBlockNum = firstBlock;
    addr.push_back(firstBlock);
    for (int i = 1; i < blockSize; i++)
    {
        nextBlockNum = fat[nextBlockNum];
        if (nextBlockNum <= 1)
        {
            flag = 0;
            break;
        }
        addr.push_back(nextBlockNum);
    }
    return flag;
}

// 打开文件
int open_file(string fileName, int mode, int pid) // 打开文件
{
    
    fileLock.lock();
    int flag = 1;
    // 支持绝对路径
    // 寻找该文件的fcb并验证权限
    // 在系统文件打开表中找有无该文件的表项，如果没有，新建这一项
    // 如果有，验证打开模式，只有读模式才能打开（写模式不允许多个进程同时打开文件）
    // 验证成功，允许打开
    // 修改系统级文件打开表的相关项
    // 新建进程级文件打开表项（若进程是第一次打开文件，那么新建一张该进程的表）
    string rowName = tempDirName;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    flag = cd(path);
    if (flag > 0)
    {
        int location = get_dir_entry_location(name);
        if (location >= 0) //存在目录项
        {
            fcb file = tempDir[location];
            if (file.attribute == COMMONFILE && file.rights >= mode) // 验证权限成功
            {
                // 在系统级文件打开表定位该项
                int softPosition = get_soft_position(path, name);
                if (softPosition < 0) // 定位失败，说明该文件首次被打开，生成表项追加到文件打开表
                {
                    int pn = 1; // 只有一个进程打开该文件
                    vector<int> pidArr;
                    pidArr.push_back(pid);
                    softEntry entry = { file, path, pn, pidArr, mode };
                    soft_entry_op(entry, MKSOFTENTRY);
                }
                else // 系统级定位成功 ,验证打开模式（只允许读），对应表项pn+1,压入pid;
                {
                    if (mode == RO)
                    {
                        softEntry entry = soft[softPosition];
                        // 是否重复打开？验证pid向量
                        for (int i = 0; i < entry.pid.size(); i++)
                        {
                            if (entry.pid[i] == pid)
                            {
                                flag = 0; // 不允许同一个进程重复打开同一个文件
                            }
                        }
                        if (flag > 0)
                        {
                            entry.pn += 1;
                            entry.pid.push_back(pid);
                            soft_entry_op(entry, UPDATEENTRY); // 更新操作
                        }
                    }
                    else // 打开模式为写模式，拒绝，不可以有多个进程写同一个文件
                    {
                        flag = 0;
                    }
                }
            }
            else // 非法操作：写目录 或者是写只读文件
            {
                flag = 0;
            }

            if (flag > 0) // 对进程级打开表的操作
            {
                // 根据pid在进程级打开文件表中搜索是否存在该进程的表
                int position = get_poft_position(pid);
                // 生成表项
                poftEntry entry = { file, path, mode };
                // 如果已经存在它的表说明打开过其他文件
                if (position >= 0)
                {
                    // 创建表项
                    poft_entry_op(entry, pid, MKPOFTENTRY);
                }
                // 不存在这张表，则创建一张
                else
                {
                    poft table;
                    vector<poftEntry> entries = { entry };
                    table.pid = pid;
                    table.entries = entries;
                    // 创建表
                    poft_op(table, MKPOFTTABLE);
                }
            }
        }
    }

    cd(rowName);
    fileLock.unlock();
    return flag;
}

//关闭文件
int close_file(string fileName, int pid)
{
    fileLock.lock();
    int flag = 1;

    // 仅支持绝对路径关闭文件
    // 在系统文件打开表中找有无该文件的表项，如果没有，提示这个文件没有被打开或不存在该文件
    // 在对应进程打开文件表中寻找该文件的表项，如果没有，提示没有或无权关闭文件
    // soft中找到了，查看pid数量，大于1则直接-1，并且将对应pid弹出，等于1则可以删除这个表项
    // pofts中找到了，对应表项删除
    string rowName = tempDirName;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    int position = get_soft_position(path, name);
    if (position < 0) //不存在表项
    {
        qDebug() << "no such file ,or the file is not opened" << endl;
        flag = 0;
    }
    else
    {   // 验证是否是该进程打开的文件
        int pTablePosition = get_poft_position(pid);
        int pEntryPosition = get_poft_entry_location(pid, path, name);
        if (pEntryPosition < 0)
        {
            qDebug() << "you cannot close file" << endl;
            flag = 0;
        }
        else
        {
            // soft中找到了，查看pid数量，大于1则直接-1，并且将对应pid弹出，等于1则可以删除这个表项
            softEntry sEntry = soft[position];
            int pn = sEntry.pn;
            if (pn == 1)
            {
                soft_entry_op(sEntry, RMSOFTENTRY);
            }
            else if (pn > 1) // 大于1则直接-1，并且将对应pid弹出
            {
                int pidLocation;
                for (int i = 0; i < sEntry.pid.size(); i++)
                {
                    if (pid == sEntry.pid[i])
                    {
                        pidLocation = i;
                    }
                }

                sEntry.pn -= 1;
                sEntry.pid.erase(sEntry.pid.begin() + pidLocation);

                int k = soft_entry_op(sEntry, UPDATEENTRY);
                qDebug() << "k:" << k << endl;
            }
            // pofts中找到了，对应表项删除（表是否删除?）
            poftEntry pEntry = pofts[pTablePosition].entries[pEntryPosition];
            int n = poft_entry_op(pEntry, pid, RMPOFTENTRY);
            if (pofts[pTablePosition].entries.size() == 0) // 如果进程的打开文件表没有内容，则删除
            {
                poft table = pofts[pTablePosition];
                poft_op(table, RMPOFTTABLE);
            }
        }
    }

    cd(rowName);
    fileLock.unlock();
    return flag;
}

// 读文件
int read_file(string fileName, vector<unsigned char*>& mem, int size)
{
    // 只有存在文件才可以读文件
    int flag = 1;
    int i = 0;
    string rowName = tempDirName;
    flag = exist_file(fileName);
    int blockSize = size <= 1024 ? 1 : size / 1024;

    // mem向量空间不足 错误
    // if (blockSize != mem.size()) return -1;

    if (flag >= 0)
    {
        string path;
        string name;
        get_path_and_name(fileName, path, name);
        cd(path);
        int location = get_dir_entry_location(name);
        fcb fileFcb = tempDir[location];
        //找到fcb，定位文件
        unsigned short startBlock = fileFcb.startBlock;
        unsigned short length = fileFcb.length;
        vector<unsigned short> fileAddr;
        get_fats(startBlock, fileAddr, length);

        unsigned short readSize = 0;

        if (blockSize > length)
        {
            readSize = length;
        }
        else if (blockSize <= length)
        {
            readSize = blockSize;
        }

        // 从磁盘中读
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        int tmpSize = size, tmpReadSize = 0;
        for (i = 0; i < readSize; i++)
        {
            // 寻址
            int offset = fileAddr[i] * BLOCKSIZE;
            disk.seekg(offset, disk.beg);
            tmpReadSize = tmpSize / BLOCKSIZE != 0 ? BLOCKSIZE : tmpSize;
            tmpSize -= BLOCKSIZE;
            // 将磁盘内容读到内存
            disk.read((char*)mem[i], tmpReadSize * sizeof(unsigned char));
        }
        // 磁盘操作结束
        disk.close();
    }

    cd(rowName);
    return flag;
}

// 写文件
int write_file(string fileName, vector<unsigned char*>& mem, int size)
{
    // 只有存在文件才可以写文件
    // 需要修改该文件的目录项utime
    // 可能需要申请物理块，更新fat表
    // 按块将内容写入磁盘
    // 可能需要需要修改fat的size和length
    // 完成后重新加载一遍目录
    int flag = 1;
    string rowName = tempDirName;
    flag = exist_file(fileName);
    int blockSize = size <= 1024 ? 1 : size / 1024;

    if (flag >= 0)
    {
        // 只能够按页（块）操作
        // 比较需要写入的大小是否超过了原先的大小（长度）
        string path;
        string name;
        get_path_and_name(fileName, path, name);
        cd(path);
        int location = get_dir_entry_location(name);
        fcb fileFcb = tempDir[location];
        unsigned short fileLength = fileFcb.length;
        unsigned short startBlock = fileFcb.startBlock;
        vector<unsigned short> fileAddr;
        time_t lt;
        lt = time(NULL);
        unsigned int utime = (unsigned int)lt;
        fileFcb.updateTime = utime;
        flag = get_fats(startBlock, fileAddr, fileLength); // 文件原来的物理块

        vector<unsigned short> fileBlocks;
        if (flag >= 0)
        {
            if (fileLength < blockSize) // 写入大小超过了原大小，重新分配物理块
            {
                // 申请新的物理块
                int newBlockSize = blockSize - fileLength;
                vector<unsigned short> blocks;
                get_blocks(newBlockSize, blocks);

                // 将申请到的物理块和文件原物理块相连,并更新文件分配表
                // 生成文件的所有块号 记在 fileBlock中。
                vector<unsigned short> nextBlocks;
                for (int i = 0; i < fileLength; i++)
                {
                    fileBlocks.push_back(fileAddr[i]);
                    if (i < fileLength - 1)
                    {
                        nextBlocks.push_back(fileAddr[i + 1]);
                    }
                }

                for (int i = 0; i < blocks.size(); i++)
                {
                    fileBlocks.push_back(blocks[i]);
                    nextBlocks.push_back(blocks[i]); // next的数量比file少一，这里保持一致即可
                }
                // next 补上结束标记
                nextBlocks.push_back(FENDFLAG);
                // 更新文件分配表
                update_fat(fileBlocks, nextBlocks);
                fileFcb.size = size;
                fileFcb.length = blockSize;
            }
            else
            {   
                fileFcb.size = size;
                fileBlocks = fileAddr;
            }

            // 申请好物理块之后，或者本身长度就够了
            // 逐块的写文件
            for (int i = 0; i < fileBlocks.size(); i++)
            {
                // 定位
                int offset = fileBlocks[i] * BLOCKSIZE;
                disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                disk.seekp(offset, disk.beg);
                disk.write((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                disk.close();
            }
            // 更新目录项
            cd(path);
            qDebug() << update_dir_entry("", fileFcb);
            cd(path); // 刷新当前路径
        }
    }

    cd(rowName);
    return flag;
}

// 拆分路径和文件名
void get_path_and_name(string fullName, string& path, string& filename) 
{
    vector<string> dirList;
    splitStr(fullName, dirList, "/");
    for (int i = 0; i < dirList.size() - 1; i++)
    {
        path = path + dirList[i] + "/";
    }
    filename = dirList[dirList.size() - 1];
}

// 系统打开文件表
// 通过文件名+路径来定位在系统打开文件表的位置
int get_soft_position(string path, string name) 
{
    int position = -1;
    for (int i = 0; i < soft.size(); i++)
    {
        int j = name.compare(soft[i].fileFcb.fileName);
        int k = path.compare(soft[i].path);
        if (j == 0 && k == 0)
        {
            position = i;
        }
    }
    return position;
}

int soft_entry_op(softEntry entry, int op) // 创建表项,仅当某文件被首个进程使用
{
    int flag = 1;
    if (op == MKSOFTENTRY)
    {
        soft.push_back(entry);
    }
    else if (op == RMSOFTENTRY)
    {
        string path = entry.path;
        string name = entry.fileFcb.fileName;
        int position = get_soft_position(path, name);
        if (soft[position].pn > 1) // 文件被多于一个进程使用，不允许删除
        {
            flag = 0;
        }
        else
        {
            soft.erase(soft.begin() + position);
        }
    }
    else if (op == UPDATEENTRY)
    {
        string path = entry.path;
        string name = entry.fileFcb.fileName;
        int position = get_soft_position(path, name);
        //soft[position] = entry;
        soft.erase(soft.begin() + position);
        soft.push_back(entry);
    }
    else
    {
        flag = 0;
    }

    return flag;
}

// 打印文件打开表内容
void print_soft(stringstream &output) 
{
    output << "soft: size: " << soft.size() << endl;
    for (int i = 0; i < soft.size(); i++)
    {
        output << "soft[" << i << "]:" << endl;
        output << "path:\t\t" << soft[i].path.c_str() << endl;
        output << "fileFcb:\t\t";
        print_fcb(soft[i].fileFcb, output);
        output << "mode:\t\t" << soft[i].mode << endl;
        output << "pn:\t\t" << soft[i].pn << endl;
        output << "pidList:\t\t" ;
        for (int j = 0; j < soft[i].pid.size(); j++)
        {
            output << soft[i].pid[j] << "\t\t";
        }
        output << endl;
    }
}

// 进程打开文件表
// 通过pid获得进程打开文件表
int get_poft_position(int pid) 
{
    int position = -1;
    for (int i = 0; i < pofts.size(); i++)
    {
        if (pofts[i].pid == pid)
        {
            position = i;
            break;
        }
    }
    return position;
}

// 通过pid,路径和文件名定位表项位置
int get_poft_entry_location(int pid, string path, string filename) 
{
    int position = -1;
    int tablePosition = get_poft_position(pid);
    if (tablePosition >= 0)
    {
        for (int i = 0; i < pofts[tablePosition].entries.size(); i++)
        {
            string entryPath = pofts[tablePosition].entries[i].path;
            string entryFilename = pofts[tablePosition].entries[i].fileFcb.fileName;
            if (filename == entryFilename && path == entryPath)
            {
                position = i;
            }
        }
    }

    return position;
}

// 对单张文件打开表的操作，分为创建表和删除表
int poft_op(poft table, int op) 
{
    int flag = 1;
    if (op == MKPOFTTABLE)
    {
        pofts.push_back(table);
    }
    else if (op == RMPOFTTABLE)
    {
        int pid = table.pid;
        int position = get_poft_position(pid);
        if (position >= 0)
        {
            pofts.erase(pofts.begin() + position);
        }
        else
        {
            flag = 0;
        }
    }
    else
    {
        flag = 0;
    }

    return flag;
}

// 对单张表中表项操作，分为创建表项，删除表项，修改表项
int poft_entry_op(poftEntry entry, int pid, int op) 
{
    int flag = 1;
    int tablePosition = get_poft_position(pid);
    string fileName = entry.fileFcb.fileName;
    string path = entry.path;
    if (tablePosition < 0)
    {
        flag = 0;
    }
    else if (op == MKPOFTENTRY)
    {
        //直接创建这一项
        pofts[tablePosition].entries.push_back(entry);
    }
    else if (op == RMPOFTENTRY)
    {
        //定位
        int entryPosition = get_poft_entry_location(pid, path, fileName);
        if (entryPosition < 0)
        {
            flag = 0;
        }
        else
        {
            pofts[tablePosition].entries.erase(pofts[tablePosition].entries.begin() + entryPosition);
        }
    }
    else if (op == UPDATEPOFTENTRY)
    {
        //定位
        int entryPosition = get_poft_entry_location(pid, path, fileName);
        if (entryPosition < 0)
        {
            flag = 0;
        }
        else
        {
            pofts[tablePosition].entries[tablePosition] = entry;
        }
    }

    return flag;
}
void print_pofts(stringstream &output)
{
    output << "pofts: size: " << pofts.size() << endl;
    for (int i = 0; i < pofts.size(); i++)
    {
        output << "poft[" << i << "]" << endl;
        output << "pid: " << pofts[i].pid << endl;
        int k = pofts[i].entries.size();
        output << "poft[" << i << "]: size: " << k << endl;
        for (int j = 0; j < k; j++)
        {
            output << "Entry[" << j << "]" << endl;
            output << "path:\t\t" << pofts[i].entries[j].path.c_str() << endl;
            output << "mode:\t\t" << pofts[i].entries[j].mode << endl;
            output << "fileFCB:\t\t";
            print_fcb(pofts[i].entries[j].fileFcb, output);
            output << endl;
        }
    }
}
//字符串切片
void splitStr(const string& str, vector<string>& arr, const string& cut)
{
    // str: 需要切割的字符串
    // arr: 切割后的字符串数组存放位置
    // cut：需要切割的符号
    string::size_type pos1, pos2;
    pos2 = str.find(cut);
    pos1 = 0;
    while (string::npos != pos2)
    {
        arr.push_back(str.substr(pos1, pos2 - pos1));
        pos1 = pos2 + cut.size();
        pos2 = str.find(cut, pos1);
    }
    if (pos1 != str.length())
        arr.push_back(str.substr(pos1));
}


void print_fcb(fcb dirEntry, stringstream& output)
{
    output << "name:" << dirEntry.fileName << "\t";
    output << "attribute:" << (int)dirEntry.attribute << "\t";
    output << "size:" << dirEntry.size << "\t";
    output << "length:" << dirEntry.length << "\t";
    output << "rights:" << (int)dirEntry.rights << "\t";
    output << "startblock:" << dirEntry.startBlock << "\t";
    output << "ctime:" << dirEntry.createTime << "\t";
    output << "utime" << dirEntry.updateTime << "\t";
    output << endl;
}

int get_dir_entry_location(string name)
{
    int location = -1;
    for (int i = 0; i < tempDir.size(); i++)
    {
        if (name.compare(tempDir[i].fileName) == 0)
        {
            location = i;
        }
    }
    if (location < 0)
    {
    }
    return location;
}

// 获取当前文件夹名称
string get_temp_dir_name()
{
    string name;
    vector<string> dirList;
    splitStr(tempDirName, dirList, "/");
    int location = dirList.size() - 1;
    name = dirList[location];
    return name;
}

// 退出文件系统
void exit_fileSystem()
{
    //关闭所有打开的文件（其实可以省略，因为打开文件表在全局变量）

    //保存文件分配表
    int offset = FIRSTFATBLOCK * BLOCKSIZE;
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    disk.seekp(offset, disk.beg);
    for (int i = 0; i < FATENTRYNUM; i++)
    {
        disk.write((char*)&fat[i], sizeof(fid));
    }
    disk.close();

    //备份
    offset = FIRSTFAT2BLOCK * BLOCKSIZE;
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    disk.seekp(offset, disk.beg);
    for (int i = 0; i < FATENTRYNUM; i++)
    {
        disk.write((char*)&fat[i], sizeof(fid));
    }
    disk.close();
}

// 文件是否存在
int exist_file(string fileName)
{
    int flag = 1;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    string rowName = tempDirName;
    flag = cd(path);
    if (flag >= 0)
    {
        int location = get_dir_entry_location(name);
        if (location < 0)
        {
            flag = 0;
            qDebug() << "no such file" << endl;
        }
    }
    cd(rowName);
    return flag;
}

// 获取文件fcb
fcb get_file_fcb(string fileName)
{
    fcb fileFcb;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    string rowName = tempDirName;
    cd(path);
    int location = get_dir_entry_location(name);
    if (location >= 0)
    {
        fileFcb = tempDir[location];
    }

    cd(rowName);
    return fileFcb;
}

// 获取文件大小,字节为单位
unsigned int get_file_size(string fileName)
{
    fcb fileFcb;
    unsigned int fileSize = 0;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    string rowName = tempDirName;
    cd(path);
    int location = get_dir_entry_location(name);
    if (location >= 0)
    {
        fileFcb = tempDir[location];
        fileSize = fileFcb.size;
    }

    cd(rowName);
    return fileSize;
}

// 虚拟内存
// 初始化
int init_vm()
{
    int flag = 1;
    //初始化vm数组
    for (int i = 0; i < VMSIZE; i++)
    {
        vmEntry entry = { 0, -1, "" }; //空闲标志，pid ,filename
    }

    // FAT2内容填充0
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    for (int i = 0; i < FATSIZE; i++)
    {
        int offset = (i + FIRSTFAT2BLOCK) * BLOCKSIZE;
        disk.seekp(offset, disk.beg);

        for (int j = 0; j < BLOCKSIZE; j++)
        {
            unsigned char fill = 0x0;
            disk.write((char*)&fill, sizeof(unsigned char));
        }
    }
    disk.close();

    return flag;
}

int read_file_vm(string fileName, vector<unsigned char*>& mem, int realSize, int logicSize, vector<unsigned char>& addr, int pid) // 文件名，读入地址 ， 真实大小（内存分给了几块），逻辑大小（包含虚拟内存的大小）虚拟内存页号
{
    // 判断是否存在这个文件，参数是否合理
    // 判断虚拟内存是否有足够的空间，不够则不允许操作
    // 判断文件大小和内存页数的关系，小于页数补0，大于则将文件剩余部分装入虚拟内存
    // 判断logicalSize是否是足够装入剩余文件，不够就拒绝（应该不会有这种情况）
    // 将剩余文件装入虚拟内存，如果文件大小小于realSize,那么虚拟内存补零
    // 所有虚拟内存的操作需要更新vm数组的内容
    int flag = 1;
    if (realSize >= logicSize)
    {
        flag = 0;
    }
    int vmFreeNum = 0;
    for (int i = 0; i < VMSIZE; i++)
    {
        if (vm[i].logicPageNum == 0)
        {
            vmFreeNum++;
        }
    }
    if (vmFreeNum < logicSize - realSize) //虚拟内存空间不足
    {
        flag = -1;
    }

    string rowName = tempDirName;
    if(flag==1)
        flag = exist_file(fileName);
    if (flag >= 0) // 存在这个文件,参数合理，虚拟内存足够
    {
        string path;
        string name;
        get_path_and_name(fileName, path, name);
        cd(path);
        int location = get_dir_entry_location(name);
        fcb fileFcb = tempDir[location];
        // 找到fcb，定位文件
        unsigned short startBlock = fileFcb.startBlock;
        unsigned short length = fileFcb.length;
        vector<unsigned short> fileAddr;
        get_fats(startBlock, fileAddr, length);
        // 判断logicSize是否大于length,小于拒绝操作
        if (logicSize < length)
        {
            flag = 0;
        }
        // 判断文件大小和内存页数的关系
        if (flag > 0)
        {
            //寻 找空闲的虚拟内存块，记录下标，标记占用

            vector<unsigned char> freeAddr;
            int needFreeNum = logicSize - realSize;
            int pageNum = realSize; // 相对于文件页号，不一定有用
            for (int i = 0; i < VMSIZE; i++)
            {
                if (vm[i].logicPageNum == 0) // 空闲
                {
                    vm[i].logicPageNum = pageNum;
                    pageNum++;
                    vm[i].fileName = path + name;
                    vm[i].pid = pid;

                    freeAddr.push_back(i);
                }
                if (freeAddr.size() >= needFreeNum)
                {
                    break; // 已经获取到足够的虚拟内存
                }
            }
            // 把下标装入addr
            addr = freeAddr;

            if (length > realSize && length <= logicSize) // mem部分填满,只有虚拟内存部分可能需要补0
            {
                // 读文件到mem中
                int i = 0;
                for (i = 0; i < realSize; i++)
                {
                    int offset = fileAddr[i] * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }
                // 读文件到buf,写文件到虚拟内存
                int j = 0;
                i = realSize;
                for (j = 0; j < length - realSize; j++)
                {
                    unsigned char* buf = new unsigned char[BLOCKSIZE];
                    int offset = fileAddr[i] * BLOCKSIZE; // 文件块号需要通过i,虚拟内存块号需要通过j
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)buf, BLOCKSIZE * sizeof(unsigned char));
                    disk.close();

                    offset = (freeAddr[j] + FIRSTFAT2BLOCK) * BLOCKSIZE; // vm数组下标，需要加上FAT2起始地址
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekp(offset, disk.beg);
                    disk.write((char*)buf, BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                    i++;
                    delete buf;
                }

                // 虚拟内存补零(写文件)
                j = length - realSize;
                for (int k = 0; k < logicSize - length; k++)
                {
                    int offset = (freeAddr[j] + FIRSTFAT2BLOCK) * BLOCKSIZE; // vm数组下标，需要加上FAT2起始地址
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekp(offset, disk.beg);
                    for (int s = 0; s < BLOCKSIZE; s++)
                    {
                        unsigned char fill = 0;
                        disk.write((char*)&fill, sizeof(unsigned char));
                    }

                    disk.close();

                    j++;
                }
            }

            else if (length <= realSize) // 虚拟内存部分需要补0，mem部分可能需要补零
            {
                // 读文件到mem中
                int i = 0;
                for (i = 0; i < length; i++)
                {
                    int offset = fileAddr[i] * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }

                // mem补0
                i = length;
                for (; i < realSize; i++)
                {
                    int offset = REBLOCK * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }

                // 虚拟内存补零(写文件)
                for (int j = 0; j < needFreeNum; j++)
                {
                    int offset = (freeAddr[j] + FIRSTFAT2BLOCK) * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekp(offset, disk.beg);
                    for (int s = 0; s < BLOCKSIZE; s++)
                    {
                        unsigned char fill = 0;
                        disk.write((char*)&fill, sizeof(unsigned char));
                    }

                    disk.close();
                }
            }
        }
    }

    return flag;
}

// 页面置换
int vm_swap(unsigned char* mem, unsigned char pageNum) // 需要换入的地址，虚拟页号
{
    // 先把需要换入的内容写到buf里，把换出的内容写到磁盘上，最后把buf的内容复制到内存
    int flag = 1;
    unsigned char* buf = new unsigned char[BLOCKSIZE]; //暂存

    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    int offset = (pageNum + FIRSTFAT2BLOCK) * BLOCKSIZE;
    disk.seekg(offset, disk.beg);
    disk.read((char*)buf, BLOCKSIZE * sizeof(unsigned char));
    disk.close();

    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    offset = (pageNum + FIRSTFAT2BLOCK) * BLOCKSIZE;
    disk.seekp(offset, disk.beg);
    disk.write((char*)mem, BLOCKSIZE * sizeof(unsigned char));
    disk.close();

    for (int i = 0; i < BLOCKSIZE; i++) 
    {
        mem[i] = buf[i];
    }
    delete buf;
    return flag;
}

// 当进程结束，释放虚拟内存对应的空间
int free_vm(int pid) 
{
    int flag = 1;
    for (int i = 0; i < VMSIZE; i++)
    {
        if (vm[i].pid == pid)
        {
            vm[i] = { 0, -1, "" };
            //对应磁盘需要清零
            disk.open(DISKNAME, ios::binary | ios::in | ios::out);
            int offset = (FIRSTFAT2BLOCK + i) * BLOCKSIZE;
            disk.seekp(offset, disk.beg);
            for (int j = 0; j < BLOCKSIZE; j++)
            {
                unsigned char fill = 0x0;
                disk.write((char*)&fill, sizeof(unsigned char));
            }
            disk.close();
        }
    }
    return flag;
}
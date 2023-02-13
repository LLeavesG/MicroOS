#pragma once
#include "file.h"

//���̣��ļ���ʽ��
fstream disk;

// �ļ�������� ÿһ��Ϊ16λ
typedef unsigned short fid;

// ϵͳ�����ļ���
vector<softEntry> soft; 

// ���̴��ļ���
vector<poft> pofts; 

// �����ڴ�
vmEntry vm[VMSIZE]; 

QMutex fileLock;
// Ŀ¼�߼��ϵĶ������ͽṹ��ʹ�������±�����ͬ���ļ���Ŀ¼����ͨ��ͬ��Ŀ¼��ʼ��ţ�FCB�е�startBlock��������һ��Ŀ¼����
typedef vector<fcb> directory; 

// ��ǰ����Ŀ¼��
string tempDirName;

// ��ǰ����Ŀ¼
directory tempDir;

// FAT��
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

//�����ļ�ϵͳ ���������� Ĭ�Ͻ����Ŀ¼ ��ʼ���ļ������������ڴ�
void start_fileSystem()
{
    //��һ���������̣��Զ����Ʒ�ʽ��disk�ļ���

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
    else //��֤�ɹ���װ�ز������������
    {
        check.close();
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        //�ڶ�������֤������������֤���ֽ��Ƿ��0
        unsigned char boot;
        disk.read((char*)&boot, sizeof(unsigned char));
        if (boot == 0x0) //���ֽ�Ϊ0����������֤ʧ��
        {
            qDebug() << "BS block error" << endl;
            //ѯ���û��Ƿ�Ҫ��ʽ������
            init_disk();
            //��ʽ����ɣ��������̣����������ļ�ϵͳ
            disk.close();
            start_fileSystem();
        }
        else //���ֽڲ�Ϊ0 ��֤�ɹ�
        {
            disk.close();
            //����FAT1���ı䵱ǰ����Ŀ¼Ϊ��Ŀ¼��cd root���������Ŀ¼����
            init_fat();
            tempDirName = "root";
            cd("root");
            init_vm();
        }
    }
}

// ��ʽ������
void init_disk()
{
    //��Ҫ��ʽ�����̵��������������
    //��ȫ��������̣��������������ַ�,����FAT����������������������FAT�͸�Ŀ¼����Ϊռ��,��ʼ����Ŀ¼

    int offset;

    //�������
    offset = 0;
    disk.seekp(offset, disk.beg);
    unsigned char init = 0x0;
    for (int i = 0; i < DISKSIZE; i++)
    {
        disk.write((char*)&init, sizeof(unsigned char));
    }

    //�������������ַ�:���ļ�����λ�����ַ���

    //���ַ�ƫ������0
    offset = 0;
    disk.seekp(offset, disk.beg);
    unsigned char boot = 0x1;
    disk.write((char*)&boot, sizeof(unsigned char));

    //����FAT��������������������FAT�͸�Ŀ¼����Ϊռ��
    offset = (1 + 1) * BLOCKSIZE;
    disk.seekp(offset, disk.beg);
    fid f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid));  //��Ŀ¼��ռ�ã�������
    disk.write((char*)&f, sizeof(fid));  //��������ռ�ã�������
    f = 2;                                // FAT1����ʼ��
    for (int i = 0; i < FATSIZE - 1; i++) //ǰ127��FAT1�����ж�Ӧ��һ
    {
        f++;
        disk.write((char*)&f, sizeof(fid));
    }
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid));  //���һ��FAT1��Ӧ�����¼Ϊ������־
    f = 2 + FATSIZE;                      // FAT2��ʼ��
    for (int i = 0; i < FATSIZE - 1; i++) //ǰ127��FAT12�����ж�Ӧ��һ
    {
        f++;
        disk.write((char*)&f, sizeof(fid));
    }
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid)); //���һ��FAT2��Ӧ�����¼Ϊ������־
    f = 1 + FIRSTROOTBLOCK;
    disk.write((char*)&f, sizeof(fid)); //��Ŀ¼����һ��
    f = FENDFLAG;
    disk.write((char*)&f, sizeof(fid)); //��Ŀ¼��������

    //��ʼ����Ŀ¼
    //��λ����Ŀ¼���ַ�,������Ŀ¼��Ŀ¼��.
    offset = BLOCKSIZE * FIRSTROOTBLOCK;
    disk.seekp(offset, disk.beg);
    time_t lt;
    lt = time(NULL);
    unsigned int ctime = (unsigned int)lt;
    unsigned int utime = (unsigned int)lt;
    fcb root = {
        ".", 2, 0, FIRSTROOTBLOCK, 2, (unsigned int)sizeof(fcb), ctime, utime };
    //��'.'Ŀ¼��д����̸�Ŀ¼��
    disk.write((char*)&root, sizeof(fcb));

    qDebug() << "success to format your disk" << endl;
}

// ��ʼ���ļ��������FAT1��ʼ����������ļ������
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
    // 0Ϊʧ��,1Ϊ�ɹ�
    if (dirName.size() != 0)
    {
        // ��Ŀ¼���ƽ�����Ƭ���洢��dirList��
        vector<string> dirList;
        // ��Ƭ���жϵ�һ���ַ����Ƿ�Ϊroot���ǵĻ����Ǿ���·�������ǵĻ��������·��
        splitStr(dirName, dirList, "/");
        // ���µ�ǰĿ¼
        for (int i = 0; i < dirList.size(); i++)
        {
            flag = load_dirEntry(dirList[i]);
            if (flag == 0) //����ʧ��
            {
                break;
            }
        }
        if (flag == 1) //���³ɹ����޸ĵ�ǰ����Ŀ¼��
        {
            if (dirList[0].compare("root") == 0) // ����·��ֱ���滻
            {
                tempDirName = dirName;
            }
            else if (dirList[0].compare(".") == 0)
            {
                tempDirName = tempDirName;
            }
            else if (dirList[0].compare("..") == 0)
            {
                //����Ŀ¼������һ��
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
            else // ���·������ƴ��
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
    // 0ʧ�ܣ�1�ɹ�
    // һ���Ǽ��ظ�Ŀ¼���������ֱ�ӴӸ�Ŀ¼���������
    if (dirName.compare("root") == 0)
    {
        // ���������Ҫ����յ�ǰĿ¼
        vector<fcb>().swap(tempDir);
        int offset = FIRSTROOTBLOCK * BLOCKSIZE;
        // �����ļ�����λ
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekg(offset, disk.beg);
        fcb root;
        disk.read((char*)&root, sizeof(fcb));
        // ����.Ŀ¼��,�涨Ŀ¼���һ��fcb��.
        tempDir.push_back(root);
        // ����Ŀ¼����Ϊ¼������
        int entryNum = (int)root.size / 32;
        for (int i = 0; i < entryNum - 1; i++)
        {
            // �����Ӵ����ж���һ��fcb,ֱ������
            disk.read((char*)&root, sizeof(fcb));
            // ��fcb׷�ӵ�Ŀ¼���
            tempDir.push_back(root);
        }
        disk.close();

    }

    // ��һ���Ǽ���Ŀ¼�������ٵ�ǰ��Ŀ¼������������û�����ƶ�Ӧ��Ŀ¼�
    // �ҵ��ˣ������ļ�����λ������.Ŀ¼�����.Ŀ¼��Ĵ�С�����ж���Ҫװ�ص�������Ȼ�����Ŀ¼��������
    else
    {
        int j = -1;
        // ���ȸ���Ŀ¼���ڵ�ǰĿ¼�����Ƿ���ƥ����
        for (int i = 0; i < tempDir.size(); i++)
        {
            // ����Ŀ¼�����ΪĿ¼��Ȩ�޿ɶ����д
            if (dirName.compare(tempDir[i].fileName) == 0 && tempDir[i].attribute == (unsigned char)0 && tempDir[i].rights > (char)0)
            {
                j = i;
                break;
            }
        }
        if (j >= 0)
        {
            // �ҵ���Ŀ¼����ʼ��źʹ�С
            unsigned short startBlock = tempDir[j].startBlock;
            // ���������Ҫ����յ�ǰĿ¼
            vector<fcb>().swap(tempDir);
            int offset = startBlock * BLOCKSIZE;
            // ���ж��Ƿ����.Ŀ¼��
            disk.open(DISKNAME, ios::binary | ios::in | ios::out);
            disk.seekg(offset, disk.beg);
            fcb itself;
            disk.read((char*)&itself, sizeof(fcb));
            unsigned int size = itself.size;
            // ��������ص���ǰĿ¼
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
            // ��������ǰĿ¼Ϊ�գ�������Ҫ������Ŀ¼�³����������
            disk.close();
        }
        else
        {
            // �����ڸ�Ŀ¼��
            flag = 0;
        }
    }
    return flag;
}

void ls(string dirName, stringstream &output)
{
    // �������ǰĿ¼
    output <<"temp directory:"<< tempDirName.c_str() << endl;
    if (dirName.compare("") == 0)
    {
        for (int i = 0; i < tempDir.size(); i++)
        {
            print_fcb(tempDir[i], output);
        }
    }
    // ָ��Ŀ¼
    else
    {
        // ���浱ǰĿ¼��
        string tempName = tempDirName;
        int i = cd(dirName);
        if (i > 0) // �ҵ�����תָ��Ŀ¼�ɹ�
        {
            for (int i = 0; i < tempDir.size(); i++)
            {
                print_fcb(tempDir[i], output);
            }
        }
        else //û�ҵ�
        {
            output << "no such directory" << endl;
        }
        // ����ԭ��Ŀ¼
        cd(tempName);
    }
}

int mkdir(string dirName)
{
    // ���浱ǰĿ¼��
    string rowName = tempDirName;
    int flag = 1;
    // 0Ϊʧ��,1Ϊ�ɹ�
    // ��Ŀ¼���ƽ�����Ƭ���洢��dirList��
    vector<string> dirList;
    // ��Ƭ���жϵ�һ���ַ����Ƿ�Ϊroot���ǵĻ����Ǿ���·�������ǵĻ��������·��
    splitStr(dirName, dirList, "/");
    if (dirList[0].compare("root") == 0) // ����·������������root�����·��
    {
        cd("root");
        vector<string>::iterator k = dirList.begin();
        dirList.erase(k); // ɾ����һ���ַ���root
    }
    int j = 1;
    int k = -1;
    for (int i = 0; i < dirList.size(); i++) // ��λ����һ����Ҫ����Ŀ¼�ĵط�
    {
        j = cd(dirList[i]);
        if (j == 1) // �������Ŀ¼����������
        {
            continue;
        }
        else // ���������Ŀ¼����¼��һ���±꣬����ѭ������ɶ�λ
        {
            k = i;
            break;
        }
    }
    if (k == -1) // ˵����Ŀ¼�Ѵ���
    {
        qDebug() << "There is already a directory named:" << dirName.c_str() << endl;
        flag = 0;
    }
    else
    {
        for (int i = k; i < dirList.size(); i++)
        {
            //�𼶵ش���Ŀ¼�ļ�
            //ÿ�δ�����Ϊ6����1��������飬2�ڸ�Ŀ¼������Ӧ���Ƶ�Ŀ¼�3�޸ĸ�Ŀ¼.Ŀ¼��Ĵ�С(size,utime)
            // 4�����뵽����������.�� 5���������������Ŀ¼��.. 6�޸��϶���Ŀ¼��Ӧ�ĸ�Ŀ¼��Ŀ¼��(size,utime),�����ǰĿ¼����rootʡ����һ��
            //ÿ�δ����ɹ��������һ��Ŀ¼

            // 1���������
            vector<unsigned short> blocks;
            get_blocks(1, blocks);

            // 2�ڸ�Ŀ¼������Ӧ���Ƶ�Ŀ¼��
            // ����Ŀ¼��
            // Ŀ¼��Ŀ¼��Ȩ��Ϊ��д������ΪĿ¼��Ĭ�ϳ���1Block��Ĭ�ϴ�С64B(����.��..����Ŀ¼��)
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
            // ������string����ʼ���ַ����飬�����ļ�������գ��ٸ���
            fcb newEntry = {
                "", rights, attribute, startBlock, length, size, ctime, utime };
            strcpy(newEntry.fileName, fileName.c_str());
            flag = create_dir_entry("", newEntry);
            if (flag == 0)
            {
                qDebug() << "fail to mkdir: " << dirList[i].c_str() << endl;
                break;
            }

            // 3�޸ĸ�Ŀ¼.Ŀ¼��Ĵ�С(size,utime)
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

            // 4�����뵽����������.��
            // ����.��
            strcpy(newEntry.fileName, ".");
            flag = create_dir_entry(fileName, newEntry);
            if (flag == 0)
            {
                qDebug() << "fail to create: ." << endl;
                break;
            }

            // 5�����뵽����������..��
            fcb father = itself;
            strcpy(father.fileName, "..");
            flag = create_dir_entry(fileName, father);
            if (flag == 0)
            {
                qDebug() << "fail to create: .." << endl;
                break;
            }

            // 6�޸��϶���Ŀ¼��Ӧ�ĸ�Ŀ¼��Ŀ¼��(size,utime),�����ǰĿ¼����rootʡ����һ��
            if (tempDirName.compare("root") == 0) // ��ǰĿ¼��root,����������
            {
                continue;
            }
            else
            {
                string fatherName = get_temp_dir_name();
                cd("..");
                location = get_dir_entry_location(fatherName);
                father = tempDir[location];
                //�޸�size��utime
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
    // ������������ݹ��ɾ��
    // ��Ŀ¼�����ļ���Ŀ¼ʱ��������ɾ��
    // ����缶ɾ��
    // �������·���;���·��
    string rowName = tempDirName;
    flag = cd(dirName);
    // �������Ŀ¼����ɾ��
    if (dirName.compare("root") == 0)
    {
        flag = 0; // ������ɾ����Ŀ¼
    }
    if (flag > 0)
    {
        if (tempDir.size() > 2) // ֻ������Ŀ¼�������ɾ��
        {
            flag = 0;
        }
        if (flag > 0)
        {
            // ����ɾ��
            // ��ȡ.�ĳ��Ⱥ���ʼ��ź���Ҫɾ����Ŀ¼��������б�ߣ�
            int location = get_dir_entry_location(".");
            unsigned short length = tempDir[location].length;
            unsigned short startBlock = tempDir[location].startBlock;
            string entryName = get_temp_dir_name();
            // 1�ͷ������ 2ɾ����Ŀ¼����Ӧ��Ŀ¼�� 3�޸ĸ�Ŀ¼.Ŀ¼�������(size��utime) 4�޸��϶���Ŀ¼�и�Ŀ¼��Ӧ��Ŀ¼�������(size��utime)�����ɾ������rootĿ¼�µ�Ŀ¼��������һ����
            cd(".."); // �ȷ�����һ������ֹɾ��Ŀ¼���޷�����

            vector<unsigned short> addr;
            // ��ȡfat�����±�
            flag = get_fats(startBlock, addr, length);
            if (flag > 0)
            {
                // 1���ݳ��Ⱥ���ʼ����޸�fat��
                // ����fat��������
                vector<unsigned short> nextBlock;
                for (int i = 0; i < addr.size(); i++)
                { // ��Ϊ���ͷ�����飬����ֱ�Ӹ�ֵΪ���м���
                    nextBlock.push_back(FREEBLOCK);
                }
                update_fat(addr, nextBlock);

                // 2ɾ����Ŀ¼����Ӧ��Ŀ¼��
                // �Ѿ��ڸ�Ŀ¼�£�ֻҪ����ɾ��Ŀ¼�������
                delete_dir_entry(entryName);

                // 3�޸ĸ�Ŀ¼.Ŀ¼�������
                location = get_dir_entry_location(".");
                fcb newSelf = tempDir[location];
                newSelf.size = newSelf.size - FCBSIZE;
                time_t lt;
                lt = time(NULL);
                unsigned int utime = (unsigned int)lt;
                newSelf.updateTime = utime;
                update_dir_entry(".", newSelf);

                // 4�޸��϶���Ŀ¼�и�Ŀ¼��Ӧ��Ŀ¼�������(size��utime)�����ɾ������rootĿ¼�µ�Ŀ¼��������һ����
                if (tempDirName.compare("root") != 0) // ����rootĿ¼
                {
                    string fatherName = get_temp_dir_name();
                    cd("..");
                    location = get_dir_entry_location(fatherName);
                    fcb father = tempDir[location];
                    //�޸�size��utime
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

// �����ļ�
int create_file(string fileName) 
{
    // ������缶����
    // ��������ͬ���ļ�
    // ������ǰĿ¼�п����ɶ���FCBʱ����
    int flag = 1;
    string rowName = tempDirName;
    // �ж��ļ����Ƿ�Ϲ�
    if (fileName.size() >= 14) //���Ȳ�����13�ֽ�
    {
        flag = 0;
    }
    vector<string> list;
    splitStr(fileName, list, "/"); //������缶
    if (list.size() > 1)
    {
        flag = 0;
    }
    int j = get_dir_entry_location(fileName);
    if (j >= 0) //������ͬ��
    {
        flag = 0;
    }
    if (tempDir.size() >= 32) //������Ŀ¼��û������fcbʱ����
    {
        flag = 0;
    }

    // 1����һ������� 2�����ļ���fcb 3.�ڵ�ǰĿ¼���½����ļ���Ŀ¼�� 4�޸ĵ�ǰĿ¼.Ŀ¼������ݣ�size��utime�� 5.�޸��϶���Ŀ¼��Ŀ¼��Ӧ��Ŀ¼�����ݣ���ǰĿ¼���Ǹ�Ŀ¼����������(size,utime)

    if (flag > 0)
    {
        // 1����һ�������
        vector<unsigned short> blocks;
        get_blocks(1, blocks);

        // 2�����ļ���fcb
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

        // 3�ڵ�ǰĿ¼���½����ļ���Ŀ¼��
        flag = create_dir_entry("", newFile);

        // 4�޸ĵ�ǰĿ¼.Ŀ¼������ݣ�size��utime��
        int location = get_dir_entry_location(".");
        fcb father = tempDir[location];
        father.size += FCBSIZE;
        father.updateTime = utime;
        flag = update_dir_entry("", father);

        // 5.�޸��϶���Ŀ¼��Ŀ¼��Ӧ��Ŀ¼�����ݣ���ǰĿ¼���Ǹ�Ŀ¼����������(size,utime)
        string fatherName = get_temp_dir_name();
        if (fatherName.compare("root") != 0)
        {
            string fatherName = get_temp_dir_name();
            cd(rowName);
            cd("..");
            location = get_dir_entry_location(fatherName);
            father = tempDir[location];
            //�޸�size��utime
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

// ɾ���ļ�
int rm(string fileName) 
{
    int flag = 1;
    string rowName = tempDirName;

    //������缶ɾ��
    vector<string> list;
    splitStr(fileName, list, "/"); 
    if (list.size() > 1)
    {
        flag = 0;
    }

    int location = get_dir_entry_location(fileName); // ���ڸ�Ŀ¼�������ɾ��
    if (location < 0)
    {
        flag = 0;
    }
    // 1�ͷ������ 2��ǰĿ¼�¶�ӦĿ¼��ɾ�� 3��ǰĿ¼.Ŀ¼���޸ģ�size��utime�� 4�ϼ�Ŀ¼�ж�Ӧ��Ŀ¼��Ŀ¼���޸ģ�size��utime)��Ŀ¼��������
    if (flag > 0)
    {
        // 1�ͷ������
        // ��λ�����ͳ���
        fcb target = tempDir[location];
        unsigned short startBlock = target.startBlock;
        unsigned short length = target.length;
        // ��ȡ������Ҫ�ͷŵ������
        vector<unsigned short> addr;
        flag = get_fats(startBlock, addr, length);
        // ���ɿ��б�־
        vector<unsigned short> nextBlock;
        for (int i = 0; i < addr.size(); i++)
        {
            nextBlock.push_back(FREEBLOCK);
        }
        // �ͷ������
        update_fat(addr, nextBlock);

        // 2��ǰĿ¼�¶�ӦĿ¼��ɾ��
        string targetName = target.fileName;
        flag = delete_dir_entry(targetName);

        // 3��ǰĿ¼.Ŀ¼���޸�(size��utime)
        int fatherLocation = get_dir_entry_location(".");
        fcb father = tempDir[fatherLocation];
        father.size -= FCBSIZE;
        time_t lt;
        lt = time(NULL);
        unsigned int utime = (unsigned int)lt;
        father.updateTime = utime;
        flag = update_dir_entry("", father);

        // 4�ϼ�Ŀ¼�ж�Ӧ��Ŀ¼��Ŀ¼���޸ģ�size��utime)
        string fatherName = get_temp_dir_name();
        if (fatherName.compare("root") != 0)
        {
            string fatherName = get_temp_dir_name();
            cd(rowName);
            cd("..");
            location = get_dir_entry_location(fatherName);
            father = tempDir[location];
            //�޸�size��utime
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

// ����Ŀ¼��  
int create_dir_entry(string dirName, fcb dirEntry) 
{
    // ��Ϊ����.Ŀ¼��ʹ�������Ŀ¼��
    qDebug() << "make entry" << endl;
    int flag = 1;
    string tempName = tempDirName;
    // ��ȡ��ǰĿ¼����ʼ���
    // �����ǰĿ¼�ǿյģ�һ��Ŀ¼��Ҳû�У���ôֻ�ܴ���.Ŀ¼���ʼ��Ŵ�.Ŀ¼���л��
    int temBlock;
    int temDirSize;
    int offset;
    // ��Ŀ¼��Ҫ��ʣ��ռ�
    if (tempDir.size() >= 32)
    {
        flag = 0;
    }
    if (strcmp(dirEntry.fileName, ".") == 0 && flag == 1) // ����.Ŀ¼��
    {
        temBlock = dirEntry.startBlock;
        temDirSize = 0;
        offset = temBlock * BLOCKSIZE + temDirSize * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
    }
    else if (strcmp(dirEntry.fileName, "..") == 0 && flag == 1) // ����..Ŀ¼��
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
    else if (flag == 1) // ��������Ŀ¼��
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

// ����ĳĿ¼��
int update_dir_entry(string dirName, fcb dirEntry) 
{
    int flag = 1;
    string tempName = tempDirName;
    cd(dirName);
    string name = dirEntry.fileName;
    int location = get_dir_entry_location(name);
    if (location >= 0)
    {
        // д����
        int tempBlock = tempDir[0].startBlock;
        int offset = tempBlock * BLOCKSIZE + location * FCBSIZE;
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        disk.seekp(offset, disk.beg);
        disk.write((char*)&dirEntry, sizeof(fcb));
        disk.close();
        // ����Ŀ¼��
        tempDir[location] = dirEntry;
    }
    else
    {
        flag = 0;
    }

    cd(tempName);
    return flag;
}

// ɾ��ĳһĿ¼��
int delete_dir_entry(string entryName) 
{
    int flag = 1;
    // ������ɾ�� . �� .. Ŀ¼��
    if (entryName.compare(".") == 0 || entryName.compare("..") == 0) 
    {
        flag = 0;
    }
    else
    {
        // ����ɾ�������
        int location = get_dir_entry_location(entryName);
        if (location >= 0)
        {
            // �ӵ�ǰ����Ŀ¼�Ƴ���Ӧ��Ŀ¼��
            tempDir.erase(tempDir.begin() + location);
            // ����ǰ����Ŀ¼��д�ش���
            // ��λ
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

// ��������飬����ֵ�������ŵ�����,����ɹ�����Ҫ�������ļ������
void get_blocks(int blockSize, vector<unsigned short>& getBlocks) 
{
    // ˳�����fat��ѡȡ��Ӧ�����������
    int i = 0;
    for (int j = 0; j < FATENTRYNUM; j++)
    {
        if (fat[j] == 0) // ��Ӧ�������
        {
            getBlocks.push_back(j);
            i++;
        }
        if (i >= blockSize)
        {
            break;
        }
    }

    // ����fat����:��ַ(getBlocks)����һ���ַ(nextBlock)
    vector<unsigned short> nextBlock;
    for (int i = 0; i < getBlocks.size() - 1; i++)
    {
        nextBlock.push_back(getBlocks[i + 1]);
    }
    // ������һ���nextBlockΪռ��
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

// ��ȡ�ļ����еĿ��
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

// ���ļ�
int open_file(string fileName, int mode, int pid) // ���ļ�
{
    
    fileLock.lock();
    int flag = 1;
    // ֧�־���·��
    // Ѱ�Ҹ��ļ���fcb����֤Ȩ��
    // ��ϵͳ�ļ��򿪱��������޸��ļ��ı�����û�У��½���һ��
    // ����У���֤��ģʽ��ֻ�ж�ģʽ���ܴ򿪣�дģʽ������������ͬʱ���ļ���
    // ��֤�ɹ��������
    // �޸�ϵͳ���ļ��򿪱�������
    // �½����̼��ļ��򿪱���������ǵ�һ�δ��ļ�����ô�½�һ�Ÿý��̵ı�
    string rowName = tempDirName;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    flag = cd(path);
    if (flag > 0)
    {
        int location = get_dir_entry_location(name);
        if (location >= 0) //����Ŀ¼��
        {
            fcb file = tempDir[location];
            if (file.attribute == COMMONFILE && file.rights >= mode) // ��֤Ȩ�޳ɹ�
            {
                // ��ϵͳ���ļ��򿪱�λ����
                int softPosition = get_soft_position(path, name);
                if (softPosition < 0) // ��λʧ�ܣ�˵�����ļ��״α��򿪣����ɱ���׷�ӵ��ļ��򿪱�
                {
                    int pn = 1; // ֻ��һ�����̴򿪸��ļ�
                    vector<int> pidArr;
                    pidArr.push_back(pid);
                    softEntry entry = { file, path, pn, pidArr, mode };
                    soft_entry_op(entry, MKSOFTENTRY);
                }
                else // ϵͳ����λ�ɹ� ,��֤��ģʽ��ֻ�����������Ӧ����pn+1,ѹ��pid;
                {
                    if (mode == RO)
                    {
                        softEntry entry = soft[softPosition];
                        // �Ƿ��ظ��򿪣���֤pid����
                        for (int i = 0; i < entry.pid.size(); i++)
                        {
                            if (entry.pid[i] == pid)
                            {
                                flag = 0; // ������ͬһ�������ظ���ͬһ���ļ�
                            }
                        }
                        if (flag > 0)
                        {
                            entry.pn += 1;
                            entry.pid.push_back(pid);
                            soft_entry_op(entry, UPDATEENTRY); // ���²���
                        }
                    }
                    else // ��ģʽΪдģʽ���ܾ����������ж������дͬһ���ļ�
                    {
                        flag = 0;
                    }
                }
            }
            else // �Ƿ�������дĿ¼ ������дֻ���ļ�
            {
                flag = 0;
            }

            if (flag > 0) // �Խ��̼��򿪱�Ĳ���
            {
                // ����pid�ڽ��̼����ļ����������Ƿ���ڸý��̵ı�
                int position = get_poft_position(pid);
                // ���ɱ���
                poftEntry entry = { file, path, mode };
                // ����Ѿ��������ı�˵���򿪹������ļ�
                if (position >= 0)
                {
                    // ��������
                    poft_entry_op(entry, pid, MKPOFTENTRY);
                }
                // ���������ű��򴴽�һ��
                else
                {
                    poft table;
                    vector<poftEntry> entries = { entry };
                    table.pid = pid;
                    table.entries = entries;
                    // ������
                    poft_op(table, MKPOFTTABLE);
                }
            }
        }
    }

    cd(rowName);
    fileLock.unlock();
    return flag;
}

//�ر��ļ�
int close_file(string fileName, int pid)
{
    fileLock.lock();
    int flag = 1;

    // ��֧�־���·���ر��ļ�
    // ��ϵͳ�ļ��򿪱��������޸��ļ��ı�����û�У���ʾ����ļ�û�б��򿪻򲻴��ڸ��ļ�
    // �ڶ�Ӧ���̴��ļ�����Ѱ�Ҹ��ļ��ı�����û�У���ʾû�л���Ȩ�ر��ļ�
    // soft���ҵ��ˣ��鿴pid����������1��ֱ��-1�����ҽ���Ӧpid����������1�����ɾ���������
    // pofts���ҵ��ˣ���Ӧ����ɾ��
    string rowName = tempDirName;
    string path;
    string name;
    get_path_and_name(fileName, path, name);
    int position = get_soft_position(path, name);
    if (position < 0) //�����ڱ���
    {
        qDebug() << "no such file ,or the file is not opened" << endl;
        flag = 0;
    }
    else
    {   // ��֤�Ƿ��Ǹý��̴򿪵��ļ�
        int pTablePosition = get_poft_position(pid);
        int pEntryPosition = get_poft_entry_location(pid, path, name);
        if (pEntryPosition < 0)
        {
            qDebug() << "you cannot close file" << endl;
            flag = 0;
        }
        else
        {
            // soft���ҵ��ˣ��鿴pid����������1��ֱ��-1�����ҽ���Ӧpid����������1�����ɾ���������
            softEntry sEntry = soft[position];
            int pn = sEntry.pn;
            if (pn == 1)
            {
                soft_entry_op(sEntry, RMSOFTENTRY);
            }
            else if (pn > 1) // ����1��ֱ��-1�����ҽ���Ӧpid����
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
            // pofts���ҵ��ˣ���Ӧ����ɾ�������Ƿ�ɾ��?��
            poftEntry pEntry = pofts[pTablePosition].entries[pEntryPosition];
            int n = poft_entry_op(pEntry, pid, RMPOFTENTRY);
            if (pofts[pTablePosition].entries.size() == 0) // ������̵Ĵ��ļ���û�����ݣ���ɾ��
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

// ���ļ�
int read_file(string fileName, vector<unsigned char*>& mem, int size)
{
    // ֻ�д����ļ��ſ��Զ��ļ�
    int flag = 1;
    int i = 0;
    string rowName = tempDirName;
    flag = exist_file(fileName);
    int blockSize = size <= 1024 ? 1 : size / 1024;

    // mem�����ռ䲻�� ����
    // if (blockSize != mem.size()) return -1;

    if (flag >= 0)
    {
        string path;
        string name;
        get_path_and_name(fileName, path, name);
        cd(path);
        int location = get_dir_entry_location(name);
        fcb fileFcb = tempDir[location];
        //�ҵ�fcb����λ�ļ�
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

        // �Ӵ����ж�
        disk.open(DISKNAME, ios::binary | ios::in | ios::out);
        int tmpSize = size, tmpReadSize = 0;
        for (i = 0; i < readSize; i++)
        {
            // Ѱַ
            int offset = fileAddr[i] * BLOCKSIZE;
            disk.seekg(offset, disk.beg);
            tmpReadSize = tmpSize / BLOCKSIZE != 0 ? BLOCKSIZE : tmpSize;
            tmpSize -= BLOCKSIZE;
            // ���������ݶ����ڴ�
            disk.read((char*)mem[i], tmpReadSize * sizeof(unsigned char));
        }
        // ���̲�������
        disk.close();
    }

    cd(rowName);
    return flag;
}

// д�ļ�
int write_file(string fileName, vector<unsigned char*>& mem, int size)
{
    // ֻ�д����ļ��ſ���д�ļ�
    // ��Ҫ�޸ĸ��ļ���Ŀ¼��utime
    // ������Ҫ��������飬����fat��
    // ���齫����д�����
    // ������Ҫ��Ҫ�޸�fat��size��length
    // ��ɺ����¼���һ��Ŀ¼
    int flag = 1;
    string rowName = tempDirName;
    flag = exist_file(fileName);
    int blockSize = size <= 1024 ? 1 : size / 1024;

    if (flag >= 0)
    {
        // ֻ�ܹ���ҳ���飩����
        // �Ƚ���Ҫд��Ĵ�С�Ƿ񳬹���ԭ�ȵĴ�С�����ȣ�
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
        flag = get_fats(startBlock, fileAddr, fileLength); // �ļ�ԭ���������

        vector<unsigned short> fileBlocks;
        if (flag >= 0)
        {
            if (fileLength < blockSize) // д���С������ԭ��С�����·��������
            {
                // �����µ������
                int newBlockSize = blockSize - fileLength;
                vector<unsigned short> blocks;
                get_blocks(newBlockSize, blocks);

                // �����뵽���������ļ�ԭ���������,�������ļ������
                // �����ļ������п�� ���� fileBlock�С�
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
                    nextBlocks.push_back(blocks[i]); // next��������file��һ�����ﱣ��һ�¼���
                }
                // next ���Ͻ������
                nextBlocks.push_back(FENDFLAG);
                // �����ļ������
                update_fat(fileBlocks, nextBlocks);
                fileFcb.size = size;
                fileFcb.length = blockSize;
            }
            else
            {   
                fileFcb.size = size;
                fileBlocks = fileAddr;
            }

            // ����������֮�󣬻��߱����Ⱦ͹���
            // ����д�ļ�
            for (int i = 0; i < fileBlocks.size(); i++)
            {
                // ��λ
                int offset = fileBlocks[i] * BLOCKSIZE;
                disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                disk.seekp(offset, disk.beg);
                disk.write((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                disk.close();
            }
            // ����Ŀ¼��
            cd(path);
            qDebug() << update_dir_entry("", fileFcb);
            cd(path); // ˢ�µ�ǰ·��
        }
    }

    cd(rowName);
    return flag;
}

// ���·�����ļ���
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

// ϵͳ���ļ���
// ͨ���ļ���+·������λ��ϵͳ���ļ����λ��
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

int soft_entry_op(softEntry entry, int op) // ��������,����ĳ�ļ����׸�����ʹ��
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
        if (soft[position].pn > 1) // �ļ�������һ������ʹ�ã�������ɾ��
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

// ��ӡ�ļ��򿪱�����
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

// ���̴��ļ���
// ͨ��pid��ý��̴��ļ���
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

// ͨ��pid,·�����ļ�����λ����λ��
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

// �Ե����ļ��򿪱�Ĳ�������Ϊ�������ɾ����
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

// �Ե��ű��б����������Ϊ�������ɾ������޸ı���
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
        //ֱ�Ӵ�����һ��
        pofts[tablePosition].entries.push_back(entry);
    }
    else if (op == RMPOFTENTRY)
    {
        //��λ
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
        //��λ
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
//�ַ�����Ƭ
void splitStr(const string& str, vector<string>& arr, const string& cut)
{
    // str: ��Ҫ�и���ַ���
    // arr: �и����ַ���������λ��
    // cut����Ҫ�и�ķ���
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

// ��ȡ��ǰ�ļ�������
string get_temp_dir_name()
{
    string name;
    vector<string> dirList;
    splitStr(tempDirName, dirList, "/");
    int location = dirList.size() - 1;
    name = dirList[location];
    return name;
}

// �˳��ļ�ϵͳ
void exit_fileSystem()
{
    //�ر����д򿪵��ļ�����ʵ����ʡ�ԣ���Ϊ���ļ�����ȫ�ֱ�����

    //�����ļ������
    int offset = FIRSTFATBLOCK * BLOCKSIZE;
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    disk.seekp(offset, disk.beg);
    for (int i = 0; i < FATENTRYNUM; i++)
    {
        disk.write((char*)&fat[i], sizeof(fid));
    }
    disk.close();

    //����
    offset = FIRSTFAT2BLOCK * BLOCKSIZE;
    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
    disk.seekp(offset, disk.beg);
    for (int i = 0; i < FATENTRYNUM; i++)
    {
        disk.write((char*)&fat[i], sizeof(fid));
    }
    disk.close();
}

// �ļ��Ƿ����
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

// ��ȡ�ļ�fcb
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

// ��ȡ�ļ���С,�ֽ�Ϊ��λ
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

// �����ڴ�
// ��ʼ��
int init_vm()
{
    int flag = 1;
    //��ʼ��vm����
    for (int i = 0; i < VMSIZE; i++)
    {
        vmEntry entry = { 0, -1, "" }; //���б�־��pid ,filename
    }

    // FAT2�������0
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

int read_file_vm(string fileName, vector<unsigned char*>& mem, int realSize, int logicSize, vector<unsigned char>& addr, int pid) // �ļ����������ַ �� ��ʵ��С���ڴ�ָ��˼��飩���߼���С�����������ڴ�Ĵ�С�������ڴ�ҳ��
{
    // �ж��Ƿ��������ļ��������Ƿ����
    // �ж������ڴ��Ƿ����㹻�Ŀռ䣬�������������
    // �ж��ļ���С���ڴ�ҳ���Ĺ�ϵ��С��ҳ����0���������ļ�ʣ�ಿ��װ�������ڴ�
    // �ж�logicalSize�Ƿ����㹻װ��ʣ���ļ��������;ܾ���Ӧ�ò��������������
    // ��ʣ���ļ�װ�������ڴ棬����ļ���СС��realSize,��ô�����ڴ油��
    // ���������ڴ�Ĳ�����Ҫ����vm���������
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
    if (vmFreeNum < logicSize - realSize) //�����ڴ�ռ䲻��
    {
        flag = -1;
    }

    string rowName = tempDirName;
    if(flag==1)
        flag = exist_file(fileName);
    if (flag >= 0) // ��������ļ�,�������������ڴ��㹻
    {
        string path;
        string name;
        get_path_and_name(fileName, path, name);
        cd(path);
        int location = get_dir_entry_location(name);
        fcb fileFcb = tempDir[location];
        // �ҵ�fcb����λ�ļ�
        unsigned short startBlock = fileFcb.startBlock;
        unsigned short length = fileFcb.length;
        vector<unsigned short> fileAddr;
        get_fats(startBlock, fileAddr, length);
        // �ж�logicSize�Ƿ����length,С�ھܾ�����
        if (logicSize < length)
        {
            flag = 0;
        }
        // �ж��ļ���С���ڴ�ҳ���Ĺ�ϵ
        if (flag > 0)
        {
            //Ѱ �ҿ��е������ڴ�飬��¼�±꣬���ռ��

            vector<unsigned char> freeAddr;
            int needFreeNum = logicSize - realSize;
            int pageNum = realSize; // ������ļ�ҳ�ţ���һ������
            for (int i = 0; i < VMSIZE; i++)
            {
                if (vm[i].logicPageNum == 0) // ����
                {
                    vm[i].logicPageNum = pageNum;
                    pageNum++;
                    vm[i].fileName = path + name;
                    vm[i].pid = pid;

                    freeAddr.push_back(i);
                }
                if (freeAddr.size() >= needFreeNum)
                {
                    break; // �Ѿ���ȡ���㹻�������ڴ�
                }
            }
            // ���±�װ��addr
            addr = freeAddr;

            if (length > realSize && length <= logicSize) // mem��������,ֻ�������ڴ沿�ֿ�����Ҫ��0
            {
                // ���ļ���mem��
                int i = 0;
                for (i = 0; i < realSize; i++)
                {
                    int offset = fileAddr[i] * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }
                // ���ļ���buf,д�ļ��������ڴ�
                int j = 0;
                i = realSize;
                for (j = 0; j < length - realSize; j++)
                {
                    unsigned char* buf = new unsigned char[BLOCKSIZE];
                    int offset = fileAddr[i] * BLOCKSIZE; // �ļ������Ҫͨ��i,�����ڴ�����Ҫͨ��j
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)buf, BLOCKSIZE * sizeof(unsigned char));
                    disk.close();

                    offset = (freeAddr[j] + FIRSTFAT2BLOCK) * BLOCKSIZE; // vm�����±꣬��Ҫ����FAT2��ʼ��ַ
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekp(offset, disk.beg);
                    disk.write((char*)buf, BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                    i++;
                    delete buf;
                }

                // �����ڴ油��(д�ļ�)
                j = length - realSize;
                for (int k = 0; k < logicSize - length; k++)
                {
                    int offset = (freeAddr[j] + FIRSTFAT2BLOCK) * BLOCKSIZE; // vm�����±꣬��Ҫ����FAT2��ʼ��ַ
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

            else if (length <= realSize) // �����ڴ沿����Ҫ��0��mem���ֿ�����Ҫ����
            {
                // ���ļ���mem��
                int i = 0;
                for (i = 0; i < length; i++)
                {
                    int offset = fileAddr[i] * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }

                // mem��0
                i = length;
                for (; i < realSize; i++)
                {
                    int offset = REBLOCK * BLOCKSIZE;
                    disk.open(DISKNAME, ios::binary | ios::in | ios::out);
                    disk.seekg(offset, disk.beg);
                    disk.read((char*)mem[i], BLOCKSIZE * sizeof(unsigned char));
                    disk.close();
                }

                // �����ڴ油��(д�ļ�)
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

// ҳ���û�
int vm_swap(unsigned char* mem, unsigned char pageNum) // ��Ҫ����ĵ�ַ������ҳ��
{
    // �Ȱ���Ҫ���������д��buf��ѻ���������д�������ϣ�����buf�����ݸ��Ƶ��ڴ�
    int flag = 1;
    unsigned char* buf = new unsigned char[BLOCKSIZE]; //�ݴ�

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

// �����̽������ͷ������ڴ��Ӧ�Ŀռ�
int free_vm(int pid) 
{
    int flag = 1;
    for (int i = 0; i < VMSIZE; i++)
    {
        if (vm[i].pid == pid)
        {
            vm[i] = { 0, -1, "" };
            //��Ӧ������Ҫ����
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
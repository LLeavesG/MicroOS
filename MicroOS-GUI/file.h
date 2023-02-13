#pragma once

#include "def.h"

#ifndef FILE_H
#define FILE_H

/*�ļ�ϵͳ�ṩ���ܣ�
 *�ļ�ϵͳ������
 *�ļ������FAT���������ز���
 *ϵͳ���ļ���Ĳ���
 *���̴��ļ���Ĳ���
 *��Ŀ¼�Ĳ���
 *��Ŀ¼��Ĳ���
 *����ͨ�ļ��Ĳ���
 *��������
 */

 //�ļ�ϵͳ��������СΪ1KB,���̷�Ϊ������(�ݶ�ռ1��)�������飨1�飩��FAT1��ռ128�飩,FAT2(���ݣ�ռ128��)����Ŀ¼�����ݶ�ռ2�飩,����������С�ɱ䣩
#define BLOCKSIZE 1024     //���̿��С��1KB
#define DISKSIZE 67108864  //���̿ռ���64MB
#define ENDBLOCK 65535     //���һ����̿��
#define FREEBLOCK 0        //���̿���б�־
#define FIRSTROOTBLOCK 258 //��Ŀ¼��ʼ���
#define FIRSTDATABLOCK 260 //���ݴ�����ʼ���
#define FATSIZE 128        // FAT��С
#define FIRSTFATBLOCK 2    // FAT��ʼ���
#define FIRSTFAT2BLOCK 130 // FAT2��ʼ���
#define REBLOCK 1          //�������

#define DISKNAME "disk"   //�����ļ���
#define FENDFLAG 1        //��������Ŀ����Ϊ��������
#define DIR 0             // fcb����
#define COMMONFILE 1      // fcb����
#define FATENTRYNUM 65536 // FAT������
#define NRW 0             //���ɶ�д
#define RO 1              //ֻ��
#define RW 2              //��д
#define FCBSIZE 32        // FCB��С

#define MKSOFTENTRY 1 //����soft����
#define RMSOFTENTRY 2 //ɾ��soft����
#define UPDATEENTRY 3 //�޸�soft����

#define MKPOFTTABLE 1     //����poft��
#define RMPOFTTABLE 2     //ɾ��poft��
#define MKPOFTENTRY 1     //����poft����
#define RMPOFTENTRY 2     //ɾ��poft����
#define UPDATEPOFTENTRY 3 //�޸�soft����

#define VMSIZE 32
/*�ļ����ƿ飬ÿ��Ŀ¼��ռ32�ֽ�(14+1+1+2+2+4+4+4)*/
typedef struct FCB
{
    char fileName[14];         //�ļ���&��չ��
    char rights;               //�ļ�Ȩ�ޣ�0���ɶ�д��1ֻ����2��д
    unsigned char attribute;   //���ԣ��ݶ�0��ʾĿ¼��1��ʾ��ͨ�ļ�
    unsigned short startBlock; //�ļ���ʼ�Ĵ��̿��
    unsigned short length;     //�ļ��ĳ��ȣ�ָռ���ٸ����̿�
    unsigned int size;         //�ļ���С����λ�ֽ�
    unsigned int createTime;   //�ļ�����ʱ��
    unsigned int updateTime;   //�ļ����һ���޸�ʱ��
} fcb;
//���˸�Ŀ¼������Ŀ¼ֻ����ռ1��



//ϵͳ���ļ�����
typedef struct softEntry
{
    fcb fileFcb;     //�ļ�fcb
    string path;     //·����,�Է��������ļ�
    int pn;          //������ǰʹ�ø��ļ��Ľ�����
    vector<int> pid; //���̱�ʶ�����飬���ڱ�����Щ������ʹ�ø��ļ�
    int mode;        //�ļ���ģʽ�������������̴��ļ��ķ�ʽ
} softEntry;

//���̴��ļ�����
typedef struct poftEntry
{
    fcb fileFcb; //Ŀ¼��
    string path; //·����,�Է��������ļ�
    // int fd;      //�ļ�����������λϵͳ���ļ���
    int mode; //�ļ���ģʽ
} poftEntry;

//���������ļ��򿪱�
typedef struct poft
{
    int pid;                   //��������id
    vector<poftEntry> entries; //���̴��ļ���������
} poft;

//�����ڴ����
typedef struct vmEntry
{
    int logicPageNum; // 0��ʾ�ÿ����
    int pid;
    string fileName;
} vmEntry;


//���̼�����
void init_disk(); //��ʼ������

//�ļ�ϵͳ������
void start_fileSystem(); //�����ļ�ϵͳ ���������� Ĭ�Ͻ����Ŀ¼

void exit_fileSystem(); //�˳��ļ�ϵͳ �����ļ������

//�ļ������FAT���������ز���
void init_fat(); //��ʼ���ļ��������FAT1��ʼ����������ļ������

void update_fat(vector<unsigned short> addr, vector<unsigned short> nextBlock); //�����ļ������

void get_blocks(int blockSize, vector<unsigned short>& getBlocks); //��������飬����ֵ�������ŵ�����,����ɹ�����Ҫ�������ļ������

int get_fats(unsigned short firstBlock, vector<unsigned short>& addr, unsigned short blockSize); //��ȡ�ļ����еĿ��

// int free_blocks(int *addr, int *blockNum); //�򻯵������ļ������

//ϵͳ���ļ���Ĳ���
int get_soft_position(string path, string name); //ͨ���ļ���+·������λ��ϵͳ���ļ����λ��
int soft_entry_op(softEntry entry, int op);      //�������,����������ɾ�������£�������Ҫ�����ı���Ͳ�������
void print_soft(stringstream& output);                               //��ӡ�ļ��򿪱�����

//���̴��ļ���Ĳ���
int get_poft_position(int pid);                                     //ͨ��pid��ý��̴��ļ���
int get_poft_entry_location(int pid, string path, string filename); //ͨ��pid·�����ļ�����λ����λ��
int poft_op(poft table, int op);                                    //�Ե����ļ��򿪱�Ĳ�������Ϊ�������ɾ����
int poft_entry_op(poftEntry entry, int pid, int op);                //�Ե��ű��б����������Ϊ�������ɾ������޸ı���
void print_pofts(stringstream &output);                                                 //��ӡ�ļ��򿪱�����
// int create_poft_entry(fcb dirEntry, int mode, int block, int offset); //��������,�����ļ����˽����״�ʹ��ʱ
// int delete_poft_entry(int fd); //ɾ����������ļ����ٱ��˽���ʹ��ʱ
// int update_poft_entry(char **ways); //�޸ı���
// int create_poft(int pid, fcb dirEntry, int mode, int block, int offset); //�½��������µĽ��̴��׸��ļ�
// int delete_poft_entry(int pid); //ɾ�����������̹ر�����ʹ�õ��ļ�
//�Ա���޸ļ�Ϊ�Ա�����޸�

//��Ŀ¼�Ĳ���
void ls(string dirName, stringstream &output); //��ʾĿ¼:

int cd(string dirName); //�ı䵱ǰ����Ŀ¼,�ɹ�����1��ʧ�ܷ���0����ͬ

int mkdir(string dirName); //����Ŀ¼

int rmdir(string dirName); //ɾ��Ŀ¼

int load_dirEntry(string dirName); //����ĳ��Ŀ¼�µ�����Ŀ¼��

//��ȡ��ǰ�ļ�������
string get_temp_dir_name();

//��Ŀ¼��Ĳ���
int create_dir_entry(string dirName, fcb dirEntry); //����Ŀ¼��

int update_dir_entry(string dirName, fcb dirEntry); //����ĳĿ¼��

int delete_dir_entry(string entryName); //ɾ��ĳһĿ¼��

//����ͨ�ļ��Ĳ���
int create_file(string fileName); //�����ļ�
int rm(string fileName);          //ɾ���ļ�

int open_file(string fileName, int mode, int pid); //���ļ� �����ļ�������fd

int close_file(string fileName, int pid); //�ر��ļ�

int read_file(string fileName, vector<unsigned char*>& mem, int size);

int write_file(string fileName, vector<unsigned char*>& mem, int size);

//��������:�����ڴ�

int init_vm(); //��ʼ�������ڴ棺��FAT2�������ֳ�����Ϊ�����ڴ�

int read_file_vm(string fileName, vector<unsigned char*>& mem, int realSize, int logicSize, vector<unsigned char>& addr, int pid); // �ļ����������ַ �� ��ʵ��С���ڴ�ָ��˼��飩���߼���С�����������ڴ�Ĵ�С�������ڴ�ҳ��

int vm_swap(unsigned char* mem, unsigned char pageNum); //�ļ��� ��Ҫ����ĵ�ַ������ҳ��

int free_vm(int pid); // ��ĳ�����̽������ͷ������ڴ��Ӧ�Ŀռ�

//���Ժ͸�������

int exist_file(string fileName);//�ж��Ƿ���ڴ��ļ� ����·��

void splitStr(const string& str, vector<string>& arr, const string& cut);//���·��

void print_fcb(fcb dirEntry, stringstream& output);

int get_dir_entry_location(string name);

void get_path_and_name(string fullName, string& path, string& filename); //��ȡ·�����ļ���

fcb get_file_fcb(string fileName);

unsigned int get_file_size(string fileName); //��ȡ�ļ���С

#endif


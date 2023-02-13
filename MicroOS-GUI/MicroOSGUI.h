#pragma once
#include "def.h"
#include <QtWidgets/QMainWindow>
#include "ui_MicroOSGUI.h"
#include <QLabel>  
#include <QLCDNumber>
#include <QTimer>
#include <QTime>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTextBrowser>
#include <QTableView>
#include <QTableWidgetItem>
#include <string>
#include<mutex>

#define CMD_NOT_FOUND ": command not found"
#define CMD_FILE_CANNOT_FOUND ": No such file or directory"
#define CMD_USAGE_ERROR ": command usage error, help with -h"
#define COMPILE_FAILED ": compile failed"
#define MAX_SHELL_BUFFER 1024
#define MYLOG qDebug() << "[" << __FILE__ << ":" << __LINE__ << ":" << __func__ << __DATE__ << __TIME__<< "]"
#define MemBlockNum 23
class MicroOSGUI : public QMainWindow
{
    Q_OBJECT

public:
    MicroOSGUI(QWidget *parent = Q_NULLPTR);
    bool eventFilter(QObject* obj, QEvent* event);
    int get_cmd_token(string& cmd, vector<string>& token);
    QLabel* lab;
    QString getTime();
    void closeEvent(QCloseEvent* event);
   
//signal:
public slots:
    void onTimerOut();
    void refreshProTable();
    //ˢ���ڴ��
    void refreshMemTable();
    //ˢ���ļ��򿪱�
    void refreshFileTable();
    //ˢ���豸���б�
    void refreshDeviceTable();
    //��ս��̱�
    void clearProcTable();
    //����ڴ��
    void clearMemTable();
    //����ڴ��ļ��򿪱�
    void clearFileTable();
    //����ڴ��ļ��򿪱�
    void clearDeviceTable();
    //���ü�ʱ��
    void systemTimeInit();
    //��ȡ������Ϣ
    void readProcBuffer();
    //��ȡ�ڴ���Ϣ�����
    void readMemBuffer();

private:
    Ui::MicroOSGUIClass ui;
    

    //shell����
    QTextEdit* shell;
    //������Ϣ�ı�����ؼ�
    QTextBrowser* processInfo;
    //�ڴ���Ϣ�ı�����ؼ�
    QTextBrowser* memInfo;
    //������Ϣ�ı�����ؼ�
    QTextBrowser* fileInfo;
    //������Ϣ����ؼ�
    QTableWidget* processView;
    //�ڴ���Ϣ����ؼ�
    QTableWidget* memView;
    //�ļ���Ϣ����ؼ�
    QTableWidget* fileView;
    //�豸��Ϣ����ؼ�
    QTableWidget* deviceView;
    //������Ϣ����ؼ�
    QLCDNumber* systemTime;
    QTimer* timer;
    

};

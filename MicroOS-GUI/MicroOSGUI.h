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
    //刷新内存表
    void refreshMemTable();
    //刷新文件打开表
    void refreshFileTable();
    //刷新设备队列表
    void refreshDeviceTable();
    //清空进程表
    void clearProcTable();
    //清空内存表
    void clearMemTable();
    //清空内存文件打开表
    void clearFileTable();
    //清空内存文件打开表
    void clearDeviceTable();
    //设置计时器
    void systemTimeInit();
    //读取运行信息
    void readProcBuffer();
    //读取内存信息并输出
    void readMemBuffer();

private:
    Ui::MicroOSGUIClass ui;
    

    //shell窗口
    QTextEdit* shell;
    //进程信息文本输出控件
    QTextBrowser* processInfo;
    //内存信息文本输出控件
    QTextBrowser* memInfo;
    //进程信息文本输出控件
    QTextBrowser* fileInfo;
    //进程信息输出控件
    QTableWidget* processView;
    //内存信息输出控件
    QTableWidget* memView;
    //文件信息输出控件
    QTableWidget* fileView;
    //设备信息输出控件
    QTableWidget* deviceView;
    //进程信息输出控件
    QLCDNumber* systemTime;
    QTimer* timer;
    

};

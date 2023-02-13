#include "MicroOSGUI.h"
#include <QKeyEvent>
#include "qtextstream.h"
#include "kernel.h"
#include "qdebug.h"
#include "file.h"
#include "device.h"

bool isWriting = false;

QString buffer = "        processInfo part is ready";
QString memBuffer;                  //�ڴ���Ϣ������
extern CPUThread* cpuThread;
extern int latestPid;
extern map<int, PCBptr> pidPool;
extern map<int, string> procName;

extern list<int> begin_queue;		//�½�����
extern list<int> waiting_queue;	    //�ȴ�����
extern list<int> ready_queue;		//��������
extern list<int> running_queue;	    //�����ڼ�¼��Щ����������

extern vector<softEntry> soft;      //�ļ��򿪱�
extern QMutex bufferLock;           //������Ϣ������������
extern QMutex memBufferLock;        //�ڴ���Ϣ������������
extern QMutex fileLock;             //�ļ���Ϣ������������
extern int flag[USE_SIZE / PAGE_SIZE];  //�ڴ����Ϣ

extern SDT* SDTPtr;                 //�豸����ָ��



QString stateArray[] = { "BEGIN","READY","RUNNING","WAITING","FINISHED","DIED" };
QString deviceBusy[] = { "IDLE" ,"BUSY"};

MicroOSGUI::MicroOSGUI(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    shell = ui.shell;
    shell->append("MicroOS>");
    shell->installEventFilter(this);

    //ע��ui�ؼ�
    systemTime = ui.systemTime;
    processView = ui.processView;
    processInfo = ui.processInfo;
    memInfo = ui.memInfo;
    memView = ui.memView;
    fileView = ui.fileView;
    deviceView = ui.deviceView;
    
    //��ʼ�������ؼ�
    systemTimeInit(); 
    refreshProTable();
    refreshMemTable();
    
    //�½�һ��QTimer����
    timer = new QTimer();
    //���ö�ʱ��ÿ�����ٺ��뷢��һ��timeout()�ź�
    timer->setInterval(1);
    //������ʱ��
    timer->start();

   
    //�źźͲ�
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimerOut()));
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshMemTable()));
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshProTable()));
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshFileTable()));
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshDeviceTable()));
    connect(timer, SIGNAL(timeout()), this, SLOT(readMemBuffer()));
    connect(timer, SIGNAL(timeout()), this, SLOT(readProcBuffer()));
    

    //�������ô��ڵı���
    this->setWindowTitle("MicroOS");

}
void MicroOSGUI::onTimerOut()
{
    //��ȡ��ǰʱ��
    QDateTime time = QDateTime::currentDateTime();
    QString text = time.toString("yyyy-MM-dd HH:mm:ss");
    systemTime->display(text);
   
}
void MicroOSGUI::readProcBuffer()
{
    //��ȡ������Ϣ�����������Ӧ�ؼ�����
    bufferLock.lock();
    QDateTime time = QDateTime::currentDateTime();
    QString text = time.toString("yyyy-MM-dd HH:mm:ss.zzz");
    if (!buffer.isEmpty())
    {
        processInfo->append(text + "\r\n" + buffer);
        buffer.clear();
    }
    bufferLock.unlock();
}
void MicroOSGUI::readMemBuffer()
{
    //��ȡ�ڴ���Ϣ�����������Ӧ�ؼ�����
    memBufferLock.lock();
    QDateTime time = QDateTime::currentDateTime();
    QString text = time.toString("yyyy-MM-dd HH:mm:ss.zzz");
    if (!memBuffer.isEmpty())
    {
        memInfo->append(text + "\r\n" + memBuffer);
        memBuffer.clear();
    }
    memBufferLock.unlock();
}
QString MicroOSGUI::getTime()
{
    //��ȡ��ǰʱ��
    QDateTime time = QDateTime::currentDateTime();
    QString text = time.toString("yyyy-MM-dd HH:mm:ss");
    return text;
}

void MicroOSGUI::closeEvent(QCloseEvent* event)
{
    qDebug() << "exit" << endl;
    exit_system();

}

void MicroOSGUI::refreshDeviceTable()
{
    //ˢ���豸�򿪱�ؼ�
    
    //����ؼ�������
    clearDeviceTable();

    int rowi = 0; 
    int i = 0;

    //���豸״̬д���豸��ؼ�
    while (i<SDTPtr->size) {

        DCTItem* tmpDCTPtr = SDTPtr->SDTitem[i].DCTItemPtr;
        PCBPtrQueue* tmpPCBptrQueuePtr;
        unsigned char deviceID = tmpDCTPtr->deviceID;
        tmpPCBptrQueuePtr = tmpDCTPtr->waitingQueueHead->next;

        if (tmpPCBptrQueuePtr != NULL) {
            deviceView->setRowCount(rowi+ 1);  //ѭ�������� Ҳ�����ú��ٿ�ʼ
            
            deviceView->setItem(rowi, 0, new QTableWidgetItem(QString::number(deviceID)));
            deviceView->setItem(rowi, 1, new QTableWidgetItem(deviceBusy[(int)tmpDCTPtr->busy]));
            QString procDeviceQueueQStr;
            while (tmpPCBptrQueuePtr != NULL) {
                procDeviceQueueQStr.append(QString::number(tmpPCBptrQueuePtr->pcbPtr->pid));
                procDeviceQueueQStr.append("->");
                tmpPCBptrQueuePtr = tmpPCBptrQueuePtr->next;
            }

            procDeviceQueueQStr.append("NULL");
            deviceView->setItem(rowi, 2, new QTableWidgetItem(procDeviceQueueQStr));
            rowi ++;
        }
         
        i++;
    }
}
void MicroOSGUI::refreshProTable()
{

    //ˢ�½���״̬��ؼ�

    //����ؼ�������
    clearProcTable();
    PCBptr nice;
    int rowi = 1;
    map<int, PCBptr>::iterator iter;
    iter = pidPool.begin();

    //������״̬д�����״̬��ؼ�
   
    while (iter != pidPool.end()) {
       
        processView->setRowCount(rowi);
        int temp = iter->first;
        processView->setItem(rowi-1, 0, new QTableWidgetItem(QString::number(temp)));
        processView->setItem(rowi-1, 1, new QTableWidgetItem(QString::fromStdString(procName[iter->second->pid])));
        processView->setItem(rowi-1, 2, new QTableWidgetItem(stateArray[iter->second->state]));
        processView->setItem(rowi-1, 3, new QTableWidgetItem(QString::number(iter->second->address)));
        rowi++;iter++;
    } 
}
void MicroOSGUI::refreshMemTable()
{
    //ˢ���ڴ�״̬��ؼ�

    //����ؼ�������
    clearMemTable();
    
    int rowNum = MemBlockNum;
    memView->setRowCount(MemBlockNum);

    //���ڴ�״̬д���ڴ�״̬��ؼ�
    for (int i = 0; i < rowNum; i++)
    {
        memView->setItem(i , 0, new QTableWidgetItem(QString::number(i)));
        if (flag[i]== -1)
            memView->setItem(i, 1, new QTableWidgetItem("free"));
        else
        {
            memView->setItem(i, 1, new QTableWidgetItem("busy"));

            memView->setItem(i, 2, new QTableWidgetItem(QString::fromStdString("pid: " + to_string(flag[i]))));
        }
        }
}

void MicroOSGUI::refreshFileTable()
{
    //ˢ���ļ��򿪱�ؼ�

    //����ؼ�������
    clearFileTable();
    PCBptr nice;
    int rowi = soft.size();
    int rowNum = MemBlockNum;
    fileView->setRowCount(rowi);

    //���ļ��򿪱�״̬д���ļ��򿪱�״̬��ؼ�
    for (int i = 0; i < rowi; i++)
    {
        
        fileView->setItem(i, 0, new QTableWidgetItem(soft[i].fileFcb.fileName));
        fileView->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(soft[i].path)));
            
        int pidnum = soft[i].pid.size();
        string apid;
        for (int j = 0; j <pidnum ; j++)
        {
            apid.append(to_string(soft[i].pid[j])+" ");
        }
        fileView->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(to_string(soft[i].pn))));
        fileView->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(apid)));
        fileView->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(to_string(soft[i].mode))));
    }
}

void MicroOSGUI::clearProcTable()
{
    int rowCount = processView->rowCount();
    for (int i = 0; i < rowCount; i++)
    {
        processView->removeRow(0);
    }
}
void MicroOSGUI::clearDeviceTable()
{
    int rowCount = deviceView->rowCount();
    for (int i = 0; i < rowCount; i++)
    {
        deviceView->removeRow(0);
    }
}
void MicroOSGUI::clearMemTable()
{
    int rowCount = memView->rowCount();
    for (int i = 0; i < rowCount; i++)
    {
        memView->removeRow(0);
    }
}
void MicroOSGUI::clearFileTable()
{
    int rowCount = fileView->rowCount();
    for (int i = 0; i < rowCount; i++)
    {
        fileView->removeRow(0);
    }
}
void MicroOSGUI::systemTimeInit()
{
    systemTime->setDigitCount(25);
    //������ʾ��ģʽΪʮ����
    systemTime->setMode(QLCDNumber::Dec);
    //������ʾ��ʽ
    systemTime->setSegmentStyle(QLCDNumber::Flat);
}



vector<string> token;

bool MicroOSGUI::eventFilter(QObject* obj, QEvent* event)
{
    int i = 0;
    stringstream output;
    vector<unsigned char*> tmpBufferVector;

    if (obj == shell) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* k = static_cast<QKeyEvent*>(event);

            if (!isWriting && (k->key() == Qt::Key_Return || k->key() == Qt::Key_Enter)) {
                QString text;
                QTextStream textStream(&text);

                text = shell->toPlainText();
                textStream.seek(text.lastIndexOf("MicroOS>") + 8);
                QString lastLine = textStream.readLine();

                string cmd = lastLine.toStdString();

                string cmdHead;

                output.clear();
                int argc = get_cmd_token(cmd, token);
                cmdHead = token[0];
                if (cmdHead == "ls") {
                    if (argc == 1) {
                        ls("", output);
                    }
                    else if (argc == 2) {
                        ls(token[1], output);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "cd") {
                    if (argc == 2) {
                        cd(token[1]);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "mkdir") {
                    if (argc == 2) {
                        mkdir(token[1]);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }

                }
                else if (cmdHead == "cfile") {
                    if (argc == 2) {
                        create_file(token[1]);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }

                }
                else if (cmdHead == "rmdir") {
                    if (argc == 2) {
                        rmdir(token[1]);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }

                }
                else if (cmdHead == "rm") {
                    if (argc == 2) {
                        rm(token[1]);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }

                }
                else if (cmdHead == "wfile") {
                    if (argc == 2) {
                        if (exist_file(token[1]) != 0) {
                            isWriting = true;
                            shell->clear();
                        }
                        else {
                            output << token[1] << CMD_FILE_CANNOT_FOUND << endl;
                        }
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }

                }
                else if (cmdHead == "rfile") {
                    if (argc == 2) {
                        if (exist_file(token[1]) != 0) {

                            unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char) * MAX_SHELL_BUFFER);
                            if (buffer != NULL) {
                                memset(buffer, 0, MAX_SHELL_BUFFER);
                                tmpBufferVector.push_back(buffer);
                                read_file(token[1], tmpBufferVector, BLOCKSIZE);
                                stringstream tmpStr;
                                tmpStr << tmpBufferVector[0];
                                shell->append(QString::fromStdString(tmpStr.str()));
                                free(buffer);
                                tmpBufferVector.clear();
                            }
                        }
                        else {
                            output << token[1] << CMD_FILE_CANNOT_FOUND << endl;
                        }
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "pfile") {
                    if (argc == 1) {
                        print_pofts(output);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "sfile") {
                    if (argc == 1) {
                        print_soft(output);
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "exec") {
                    if (argc == 2) {
                        string fileName = token[1];
                        if (exist_file(fileName)) {
                            init_proc(fileName);
                        }
                        else {
                            output << token[1] << CMD_FILE_CANNOT_FOUND << endl;
                        }
                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "compile") {
                    if (argc == 3) {
                        string srcPath = token[1];
                        string outPath = token[2];
                        if (exist_file(srcPath) != 0 && exist_file(outPath) != 0) {
                            if (compile(srcPath, outPath) == -1) {
                                output << cmdHead << COMPILE_FAILED << endl;
                            }
                        }
                        else {
                            output << srcPath << " OR " << outPath << CMD_FILE_CANNOT_FOUND << endl;
                        }

                    }
                    else {
                        output << cmdHead << CMD_USAGE_ERROR << endl;
                    }
                }
                else if (cmdHead == "exit") {
                    // �˳�ϵͳ
                    exit_system();
                }
                else {
                    output << cmdHead << CMD_NOT_FOUND << endl;
                }
                if (isWriting == false) {
                    shell->append(QString::fromStdString(output.str()));
                    shell->append("MicroOS>");
                }
                return true;
            }
            else if (isWriting && k->modifiers() == Qt::ControlModifier) {
                if (k->key() == Qt::Key_S) {
                    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char) * MAX_SHELL_BUFFER);
                    if (buffer != NULL) {
                        isWriting = false;

                        memset(buffer, 0, MAX_SHELL_BUFFER);
                        tmpBufferVector.push_back(buffer);
                        string tmpBufferStr = shell->toPlainText().toStdString();
                        memcpy(buffer, tmpBufferStr.c_str(), tmpBufferStr.length());

                        write_file(token[1], tmpBufferVector, strlen((const char*)buffer));
                        free(buffer);
                        shell->clear();
                        tmpBufferVector.clear();
                        shell->append("MicroOS>");
                    }
                }
                return true;
            }
            else if (k->key() == Qt::Key_Backspace) {
                if (!isWriting) {
                    QString text;
                    QTextStream textStream(&text);

                    text = shell->toPlainText();
                    textStream.seek(text.lastIndexOf("MicroOS>") + 8);
                    QString lastLine = textStream.readLine();
                    if (lastLine.length() == 0) {
                        return true;
                    }
                }
                else {
                }

            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

int MicroOSGUI::get_cmd_token(string& cmd, vector<string>& token)
{

    token.clear();
    cmd = cmd + " ";
    int pos = cmd.find(" ");

    while (pos != cmd.npos) {
        string tmp = cmd.substr(0, pos);
        token.push_back(tmp);

        cmd = cmd.substr(pos + 1, cmd.size());
        pos = cmd.find(" ");
    }
    return token.size();

}
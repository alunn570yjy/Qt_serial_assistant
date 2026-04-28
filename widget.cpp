#include "widget.h"
#include "ui_widget.h"
#include "QSerialPortInfo"
#include "QString"
#include "QMessageBox"

#include "QFileDialog"
#include "QFile"
#include "QTextStream"

#include <QTimer>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //构造函数初始化//
    QStringList serialNamePort;

    serialPort = new QSerialPort(this);

    updateLedStatus(false); // 默认初始状态为关闭


    // 初始化一个定时器对象，用于定时发送
    m_sendTimer = new QTimer(this);
    ui->ascll_send->setChecked(true);
    ui->ascll_receive->setChecked(true);

    // 定时器初始化 UI 状态：默认未勾选模式时，时间框和启动按钮不可用
    ui->Scheduled_time->setEnabled(false);
    ui->Scheduled_Send->setEnabled(false);
    ui->close_Scheduled_Send->setEnabled(false);
    ui->Scheduled_time->setText("1000"); // 默认值

    // 仅允许输入 1 到 999999 的数字
    ui->Scheduled_time->setValidator(new QIntValidator(1, 999999, this));
    //构造函数初始化往上//

    //连接串口信息读取的槽函数
    connect(serialPort,SIGNAL(readyRead()),this,SLOT(serialPortReadyRead_Slot()));
    //连接定时器信号的槽函数
    connect(m_sendTimer, &QTimer::timeout, this, &Widget::on_sendcb_clicked);

//    //连接日志显示缓冲区的槽函数
//    connect(rxMergeTimer, &QTimer::timeout, this, [=](){
//        if (!rxBuffer.isEmpty()) {
//            appendLogToUI("RX", rxBuffer); // 传入类型和缓冲区数据
//            rxBuffer.clear();              // 处理完需要清空缓冲区
//        }
//    });

    // 初始化一个定时器对象，用于日志定时显示
    rxMergeTimer = new QTimer(this);
    rxMergeTimer->setSingleShot(true); // 只触发一次

    // 时间到了，把缓冲区里的东西吐出来显示
    connect(rxMergeTimer, &QTimer::timeout, [this](){
        if(!rxBuffer.isEmpty()){
            appendLogToUI("RX", rxBuffer);
            rxBuffer.clear();
        }
    });



    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
        serialNamePort<<info.portName();
    }
    ui->serialcb->addItems(serialNamePort);
}



Widget::~Widget()
{
    delete ui;
}


// 串口状态标识，识别串口是否开启，红关 绿开
void Widget::updateLedStatus(bool isOpen)
{
    if(isOpen) {
        // 绿色
        ui->lbl_status_led->setStyleSheet(
            "background-color: rgb(0, 255, 0);" // 绿色
            "border-radius: 10px;"
            "border: 1px solid gray;"           // 灰色边框
        );
    } else {
        // 红色
        ui->lbl_status_led->setStyleSheet(
            "background-color: rgb(255, 0, 0);" // 红色
            "border-radius: 10px;"
            "border: 1px solid gray;"           // 灰色边框
        );
    }
}


// 打开串口 根据数据格式与基本参数配置初始化，即打开串口初始化一次
void Widget::on_opencb_clicked()
{
    // 1. 基本参数设置
    serialPort->setPortName(ui->serialcb->currentText());
    serialPort->setBaudRate(ui->baudratecb->currentText().toInt());
    serialPort->setDataBits((QSerialPort::DataBits)ui->datacb->currentText().toInt());

    // 2. 停止位设置
    QString stopText = ui->stopcb->currentText();
    if(stopText == "1")        serialPort->setStopBits(QSerialPort::OneStop);
    else if(stopText == "1.5") serialPort->setStopBits(QSerialPort::OneAndHalfStop);
    else if(stopText == "2")   serialPort->setStopBits(QSerialPort::TwoStop);

    // 3. 校验位设置
    QString checkText = ui->checkcb->currentText().toLower();
    if(checkText == "none")      serialPort->setParity(QSerialPort::NoParity);
    else if(checkText == "even") serialPort->setParity(QSerialPort::EvenParity);
    else if(checkText == "odd")  serialPort->setParity(QSerialPort::OddParity);
    else                         serialPort->setParity(QSerialPort::NoParity);

    // 4. 打开串口并处理反馈
    if(serialPort->open(QIODevice::ReadWrite)) {
        QMessageBox::information(this, "tips", "Serial port opened successfully");
        updateLedStatus(true); // <--- 关联串口指示标志，成功打开变绿

        ui->opencb->setEnabled(false);   // 禁用打开按钮
        ui->serialcb->setEnabled(false); // 锁定串口选择，防止运行中切换
    } else {
        // 使用 errorString() 可以看到具体的系统报错原因
        QMessageBox::warning(this, "tips", "Failed: " + serialPort->errorString());
        updateLedStatus(false); // <--- 关联串口指示标志，失败打开保持默认关闭状态
    }
}


// 关闭串口
void Widget::on_closecb_clicked()
{
    if (serialPort->isOpen()) {
        serialPort->close();
        QMessageBox::information(this, "tips", "Serial port closed successfully!");
        updateLedStatus(false); // <--- 关联串口指示标志，失败打开保持默认关闭状态

        // 恢复界面控件的可点击状态
        ui->opencb->setEnabled(true);
        ui->serialcb->setEnabled(true);
    } else {
        // 如果串口没开，直接提示没开
        QMessageBox::warning(this, "tips", "Serial port is not open.");
    }
}


//// 读取串口传过来的数据，并显示在接收文本框中
//void Widget::serialPortReadyRead_Slot()
//{
//    // 1. 从串口缓冲区读取所有可用字节
//    QByteArray data = serialPort->readAll();

//    // 是否停止显示
//    if (ui->stopShow_cb->isChecked())
//        return;


////    if (ui->log_mode_cb->isChecked()) {
////           // --- 日志模式：进入缓冲区聚合 ---
////           rxBuffer.append(data);

////           // 如果 500ms 定时器没在跑，说明是新一段数据的开始，启动它
////           if (!rxMergeTimer->isActive())
////               rxMergeTimer->start(500);
////     }

////    else{
//        if (!data.isEmpty()) {
//            // 2. 将字节流转换为字符串
//            QString str;
//            if (ui->hex_receive->isChecked()) {
//                 // --- HEX 模式显示 ---
//                 // toHex() 将字节转为 0a 0b 格式，' ' 是分隔符
//                 // toUpper() 将 abc 变成 ABC，更符合工业习惯
//                 str = data.toHex(' ').toUpper().append(' ');
//            }
//            else {
//                 // --- ASCII 模式显示 ---
//                 str = QString::fromLocal8Bit(data);
//            }

//            // 3. 将文字追加到接收文本框
//            // 使用 insertPlainText 而不是 setText，这样不会覆盖旧内容
//            ui->receivecb->moveCursor(QTextCursor::End); // 自动滚动到最下方
//            ui->receivecb->insertPlainText(str);
//        }
////    }

//}


// 读取串口传过来的数据，并显示在接收文本框中
// 这里加入了日志模式的优先判断，日志模式定时500ms接收
void Widget::serialPortReadyRead_Slot()
{
    QByteArray data = serialPort->readAll();
    if (data.isEmpty()) return;
    if (ui->stopShow_cb->isChecked()) return;

    if (ui->log_mode_cb->isChecked()) {
        // --- 日志模式：数据入库，启动/重置计时 ---
        rxBuffer.append(data);

        // 如果计时器没在跑，开启 500ms 倒计时
        if (!rxMergeTimer->isActive()) {
            rxMergeTimer->start(500);
        }
    }
    else {
        // --- 普通模式：直接渲染（保持原有的 HEX/ASCII 判断） ---
        QString str;
        if (ui->hex_receive->isChecked()) {
            str = data.toHex(' ').toUpper().append(' ');
        } else {
            str = QString::fromLocal8Bit(data);
        }
        ui->receivecb->moveCursor(QTextCursor::End);
        ui->receivecb->insertPlainText(str);
    }
}



// 单条发送数据，也作为触发函数关联了自动发送、发送文件之类的其他发送功能
void Widget::on_sendcb_clicked()
{
    // --- 单条数据的校验与发送逻辑 ---
    // 1. 检查串口是否打开
    if (!serialPort->isOpen()) {
        QMessageBox::warning(this, "tips", "Please open serial port first!");
        return;
    }

    // 2. 获取输入框文本
    QString msg = ui->sendEdit->text();
    bool isHexMode = ui->hex_send->isChecked(); // 获取当前是否为HEX模式

    if (msg.isEmpty()){
        QMessageBox::warning(this, "tips", "输入栏数据为空，请先输入数据");
        return; // 空数据不发送
    }
    if (!checkSendData(msg, isHexMode)) {
        return; // 校验不通过，直接退出，不执行发送
    }

    // 3. 转换为字节流并发送
    QByteArray sendData;
    if (isHexMode) {
        sendData = QByteArray::fromHex(msg.toUtf8());
    } else {
        sendData = msg.toLocal8Bit();
    }
    serialPort->write(sendData);


    // --- 【新增】日志模式的发送逻辑 ---
    if (ui->log_mode_cb->isChecked()) {
        // 重要：如果接收缓冲区 rxBuffer 里还有数据没到 500ms，先强制把它刷出来
        // 这样可以保证日志顺序是：[旧RX] -> [新TX]，而不是相反
        if (rxMergeTimer->isActive() && !rxBuffer.isEmpty()) {
            rxMergeTimer->stop();
            appendLogToUI("RX", rxBuffer);
            rxBuffer.clear();
        }

        // B. 记录发送日志
        appendLogToUI("TX", sendData);
    }

}



// 清空发送栏数据
void Widget::on_clear_sendcb_clicked()
{
    // 清空发送数据框的内容
    ui->sendEdit->clear();

    // 清空后让光标重新回到发送框，方便直接开始输入新内容
    ui->sendEdit->setFocus();
}
//清空接收栏数据
void Widget::on_clear_receivecb_clicked()
{
    // 清空接收数据框的内容
    ui->receivecb->clear();
}



// 接收区状态改变槽函数
void Widget::on_hex_receive_toggled(bool checked)
{

    // --- 第一部分：处理日志模式的“冲刷”逻辑 ---
    // 如果日志模式开启，且缓冲区里还有 500ms 内未显示的数据
    if (ui->log_mode_cb->isChecked()) {
        if (rxMergeTimer->isActive() && !rxBuffer.isEmpty()) {
            rxMergeTimer->stop();         // 停止计时
            appendLogToUI("RX", rxBuffer); // 立即按新勾选的格式渲染到 UI
            rxBuffer.clear();             // 清空缓冲区
        }
        // 注意：日志模式下通常不建议对 receivecb 里的历史记录做全局转换，
        // 因为历史记录里含有时间戳字符串，强制转换会导致时间戳变乱码。
        return;
    }

    // --- 第二部分：非日志模式下的全局转换逻辑 ---
    QString str = ui->receivecb->toPlainText(); // 获取接收区所有文本
    if(str.isEmpty()) return;

    if(checked) {
        // 【ASCII -> HEX】
        QByteArray data = str.toLocal8Bit();
        ui->receivecb->setPlainText(data.toHex(' ').toUpper());
    } else {
        // 【HEX -> ASCII】
        QByteArray data = QByteArray::fromHex(str.toUtf8());
        ui->receivecb->setPlainText(QString::fromLocal8Bit(data));
    }
}
// 发送区状态改变槽函数
void Widget::on_hex_send_toggled(bool checked)
{
    QString str = ui->sendEdit->text(); // 获取当前发送框内容
    if(str.isEmpty()) return;

    if(checked) {
        // 【ASCII -> HEX】
        // 例如："123" -> "31 32 33 "
        QByteArray data = str.toLocal8Bit();
        ui->sendEdit->setText(data.toHex(' ').toUpper());
    } else {
        // 【HEX -> ASCII】
        // 例如："31 32 33" -> "123"
        QByteArray data = QByteArray::fromHex(str.toUtf8());
        ui->sendEdit->setText(QString::fromLocal8Bit(data));
    }
}


// 检查发送数据格式是否合法，不为空，不为非法字符或非法格式
bool Widget::checkSendData(QString data, bool isHex)
{
    if (data.isEmpty()) {
        QMessageBox::warning(this, "警告", "发送内容不能为空！");
        return false;
    }

    if (isHex) {
        // 使用正则表达式校验：只允许 0-9, a-f, A-F 和 空格
        // QRegularExpression 需要包含头文件
        QRegularExpression hexRegex("^[0-9a-fA-F\\s]+$");
        if (!hexRegex.match(data).hasMatch()) {
            QMessageBox::critical(this, "格式错误", "HEX模式下只能输入十六进制字符（0-9, A-F）和空格！");
            return false;
        }

        // 进一步校验：去掉空格后，字符个数必须是偶数（每两个字符代表一个字节）
        QString pureHex = data.remove(' ');
        if (pureHex.length() % 2 != 0) {
            QMessageBox::warning(this, "格式错误", "HEX模式下十六进制字符必须成对出现（偶数个）！\n当前字符数：" + QString::number(pureHex.length()));
            return false;
        }
    }

    return true; // 校验通过
}



// 保存当前接收框的数据日志到txt文本
void Widget::on_savefile_clicked()
{
// 1. 获取接收窗口现有的所有文本
    QString content = ui->receivecb->toPlainText();

    // 2. 检查是否有内容可保存
    if (content.isEmpty()) {
        QMessageBox::warning(this, "提示", "接收区内容为空，无需保存！");
        return;
    }

    // 3. 弹出保存文件对话框
    // 参数：父对象, 标题, 默认路径, 过滤器
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "保存接收数据",
                                                    "receivedata.txt",
                                                    "文本文件 (*.txt);;所有文件 (*.*)");

    // 4. 如果用户取消了选择，直接返回
    if (fileName.isEmpty()) {
        return;
    }

    // 5. 执行写文件操作
    QFile file(fileName);
    // 以“只写”和“文本”模式打开，如果文件已存在则覆盖
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content; // 将内容写入流
        file.close();   // 记得关门

        QMessageBox::information(this, "成功", "文件已保存至：\n" + fileName);
    } else {
        QMessageBox::critical(this, "错误", "无法打开文件进行写入！");
    }
}


// DTR模式的控制
void Widget::on_DTR_clicked(bool checked)
{
    // 关键：必须先判断串口是否打开，否则程序可能崩溃
    if (serialPort->isOpen()) {
        serialPort->setDataTerminalReady(checked);
        if (serialPort->isDataTerminalReady())
        QMessageBox::information(this, "信息", "DTR 已开启");
        else
            QMessageBox::information(this, "信息", "DTR 已关闭");
    } else {
        ui->DTR->setChecked(!checked);
        QMessageBox::warning(this, "错误", "请先打开串口后再操作 DTR");
    }
}



// RTS模式的控制
void Widget::on_RTS_clicked(bool checked)
{
    if (!serialPort->isOpen()) {
        ui->RTS->setChecked(false);
        QMessageBox::warning(this, "错误", "请先打开串口后再操作 RTS");
        return;
    }
    serialPort->setRequestToSend(checked);
    if (serialPort->isRequestToSend())
    QMessageBox::information(this, "信息", "RTS 已开启");
    else
        QMessageBox::information(this, "信息", "RTS 已关闭");
}



// 打开txt文本的控制按钮，读取要发送的文档
void Widget::on_openfile_clicked()
{
    // 1. 弹出文件打开对话框
    // 参数说明：父窗口, 标题, 默认起始路径, 文件过滤器
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    "选择要打开的TXT文件",
                                                    "C:/",
                                                    "文本文件 (*.txt);;所有文件 (*.*)");

    // 2. 检查用户是否取消了选择
    if (filePath.isEmpty()) {
        return; // 用户点了取消，直接返回
    }

    // 3. 将绝对路径显示在 QLineEdit 文本框中
    ui->file_address->setText(filePath);

}



// 单击发送文件，检查逻辑是否符合发送要求
void Widget::on_sendfile_clicked()
{
    // 1. 检查串口状态
    if (!serialPort->isOpen()) {
        QMessageBox::warning(this, "警告", "请先打开串口！");
        return;
    }

    // 2. 获取文件路径
    QString filePath = ui->file_address->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要发送的文件路径！");
        return;
    }

    // 3. 打开文件并读取数据
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开文件，请检查文件是否被占用或已删除。");
        return;
    }

    // 4. 读取全部内容
    // 如果是纯文本文件，通常使用 toLocal8Bit() 或 UTF-8
    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.isEmpty()) {
        QMessageBox::information(this, "提示", "文件内容为空。");
        return;
    }

//    // 5. 执行发送逻辑1.0版本
//    // 对于小文件，可以直接写：
//    qint64 bytesWritten = serialPort->write(fileData);

//    if (bytesWritten == -1) {
//        QMessageBox::critical(this, "错误", "文件发送失败！");
//    }else
//        QMessageBox::information(this, "提示", "文件发送成功");

    // 5. 发送逻辑2.0版本 间断发送，若数据量大则分块发送
    int totalSize = fileData.size();
    int sentSize = 0;
    const int chunkSize = 1024; // 每次发1KB
    bool isError = false;

    while (sentSize < totalSize) {
        // 计算本次要发的大小
        int sizeToSend = qMin(chunkSize, totalSize - sentSize);
        QByteArray chunk = fileData.mid(sentSize, sizeToSend);

        // 写入串口
        qint64 len = serialPort->write(chunk);

        if (len < 0) { // 发送出错
            isError = true;
            break;
        }

        // 等待硬件写入完成（防止缓冲区塞满导致程序失去响应）
        if (!serialPort->waitForBytesWritten(1000)) {
            isError = true;
            break;
        }

        sentSize += sizeToSend;

        // 【可选】在界面实时显示进度（如 Label 或 状态栏）
        // ui->statusLabel->setText(QString("正在发送: %1/%2 字节").arg(sentSize).arg(totalSize));

        // 处理界面事件，防止点击按钮时界面由于循环太快而卡死
        QCoreApplication::processEvents();
    }

    // 根据结果给出提示
    if (isError) {
        //发送失败提示
        QMessageBox::critical(this, "发送失败", "在发送过程中发生错误或超时！");
        }
    else if (sentSize >= totalSize) {
        // 发送成功提示
        QMessageBox::information(this, "完成",
                                 QString("文件发送成功！\n共发送：%1 字节").arg(sentSize));
    }
}



// 勾选定时发送模式的checkbox控制
void Widget::on_Scheduled_Sendmode_clicked(bool checked)
{
    // 根据勾选状态解锁或锁定控件
    ui->Scheduled_time->setEnabled(checked);
    ui->Scheduled_Send->setEnabled(checked);
    ui->close_Scheduled_Send->setEnabled(checked);

    // 如果取消勾选，强制停止正在运行的定时器
    if (!checked) {
        m_sendTimer->stop();
    }
}


// 定时发送按钮
void Widget::on_Scheduled_Send_clicked()
{
    // 获取输入框文本
    QString msg = ui->sendEdit->text().trimmed();

    if (!serialPort->isOpen()) {
        QMessageBox::warning(this, "警告", "请先打开串口！");
        return;
    }
    // 定时发送模式是否合法，无文本强制关闭该模式
    if (msg.isEmpty()){
        m_sendTimer->stop(); // 1. 立即停止定时器，防止死循环弹窗
        QMessageBox::warning(this, "警告", "输入框无文本，请先输入文本再点击自动发送");
        return;

    }
    // 获取设定的时间间隔
    int interval = ui->Scheduled_time->text().toInt();
    if (interval <= 0) {
        QMessageBox::warning(this, "提示", "请输入有效的时间间隔(ms)！");
        return;
    }

    // 启动定时器
    m_sendTimer->start(interval);

    // UI 反馈：变灰避免重复点击或提示已开始
    ui->Scheduled_Send->setEnabled(false);
    ui->Scheduled_Sendmode->setEnabled(false); // 发送期间不允许改模式
//    ui->statusBar->showMessage("定时发送中...");
    QMessageBox::information(this, "提示", "自动发送已开启");
}


// 关闭定时发送按钮
void Widget::on_close_Scheduled_Send_clicked()
{
    m_sendTimer->stop();

    // 恢复控件可用性
    ui->Scheduled_Send->setEnabled(true);
    ui->Scheduled_Sendmode->setEnabled(true);
//    ui->statusBar->showMessage("定时发送已停止");
    QMessageBox::information(this, "提示", "自动发送已关闭");
}



// 日志模式的接收框UI展示切换
void Widget::appendLogToUI(QString type, QByteArray data)
{
    if (data.isEmpty()) return;

    // 1. 获取当前精确时间
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");

    // 2. 根据当前 UI 状态进行转换（实现“实时切换”效果）
    QString content;
    if (ui->hex_receive->isChecked()) {
        content = data.toHex(' ').toUpper();
    } else {
        content = QString::fromLocal8Bit(data);
    }

    // 3. 构造日志行
    // [时间][类型] 数据内容 -> 换行
    QString logLine = QString("[%1][%2] %3\n").arg(timeStr).arg(type).arg(content);

    // 4. 写入文本框
    ui->receivecb->moveCursor(QTextCursor::End);
    ui->receivecb->insertPlainText(logLine);
}



// 勾选日志模式是否开启
void Widget::on_log_mode_cb_clicked(bool checked)
{
    if (!checked) {
        // 关闭日志模式时，立即清空缓冲区到屏幕上
        if (!rxBuffer.isEmpty()) {
            rxMergeTimer->stop();
            appendLogToUI("RX", rxBuffer);
            rxBuffer.clear();
        }
        // 可选：在接收区加一条分割线，提示模式切换
        ui->receivecb->insertPlainText("\n--- Logging Mode Disabled ---\n");
    } else {
        ui->receivecb->insertPlainText("\n--- Logging Mode Enabled (500ms Aggregation) ---\n");
    }
}


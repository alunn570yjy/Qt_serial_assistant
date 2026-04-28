#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QString>
#include <QTimer>

#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    QSerialPort *serialPort;


private slots:
    void updateLedStatus(bool isOpen);

    void on_opencb_clicked();

    void on_closecb_clicked();

    void serialPortReadyRead_Slot();

    void on_sendcb_clicked();

    void on_clear_sendcb_clicked();

    void on_clear_receivecb_clicked();

    void on_hex_receive_toggled(bool checked);

    void on_hex_send_toggled(bool checked);

    bool checkSendData(QString data, bool isHex);

    void on_savefile_clicked();

    void on_DTR_clicked(bool checked);

    void on_RTS_clicked(bool checked);

    void on_openfile_clicked();

    void on_sendfile_clicked();

    void on_Scheduled_Sendmode_clicked(bool checked);

    void on_Scheduled_Send_clicked();

    void on_close_Scheduled_Send_clicked();


    void appendLogToUI(QString type, QByteArray data);


    void on_log_mode_cb_clicked(bool checked);

private:
    Ui::Widget *ui;

    QTimer *m_sendTimer; // 定义定时器指针

    QByteArray rxBuffer;
    QTimer *rxMergeTimer;
//    QString lastLogType;
};


#endif // WIDGET_H

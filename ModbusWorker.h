#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <modbus.h> // 引入 libmodbus 头文件

class ModbusWorker : public QObject 
{
    Q_OBJECT

public:
    explicit ModbusWorker(QObject* parent = nullptr);
    ~ModbusWorker() override;

signals:
    /// <summary>
    /// 发送LOG信息，等级，文本
    /// </summary>
    void logMessageReady(const QString& level, const QString& message);

    /// <summary>
    /// 通知连接状态
    /// </summary>
    /// <param name="isConnected">连接状态</param>
    /// <param name="statusText">连接文本</param>
    void pollingStatusChanged(bool isConnected, const QString& statusText);

    /// <summary>
    /// 某站的某行更新当前值和状态
    /// </summary>
    void modbusDataReceived(int stationId, int row, const QString& newValue, const QString& newStatus);

public slots:
    //开始连接
    void startConnection();
    //停止连接
    void stopConnection();
    //更换当前站
    void changeActiveStation(int stationId);

private slots:
    // 定时器触发的轮询动作，轮询访问每个站
    void executePolling();

private:
    modbus_t* m_ctx;          // libmodbus 上下文句柄
    QTimer* m_pollTimer;      // 驱动轮询的定时器

    QList<int> m_stationQueue;
    int m_currentStationIndex;//当前站在m_stationQueue中的下标
    int m_activeStationId;

    // 具体的读取逻辑封装
    void readStation0x01();
    void readStation0x02();
    void readStation0x03();
};
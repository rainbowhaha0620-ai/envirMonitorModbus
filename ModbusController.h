#pragma once
#include <QObject>
#include <QThread>
#include "ModbusWidget.h"
#include "ModbusWorker.h"

class ModbusController : public QObject
{
    Q_OBJECT

public:
    explicit ModbusController(QObject* parent = nullptr);
    ~ModbusController() override;

    // 对外提供一个显示界面的接口
    void showWindow();

private:
    void setupConnections(); // 将庞大的信号槽绑定逻辑封装起来

private:
    ModbusWidget* m_ui;            // UI 界面实例
    ModbusWorker* m_worker;        // 底层逻辑实例
    QThread* m_modbusThread;       // 承载逻辑的子线程
};
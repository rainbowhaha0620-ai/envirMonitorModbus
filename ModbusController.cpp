#include "ModbusController.h"
#include <QMetaObject>

ModbusController::ModbusController(QObject* parent): QObject(parent) 
{
    m_ui = new ModbusWidget();
    m_modbusThread = new QThread(this);
    m_worker = new ModbusWorker(); // 【关键】：这里千万不能传 parent，否则无法 moveToThread
    m_worker->moveToThread(m_modbusThread);

    // 信号与槽的绑定
    setupConnections();
    m_modbusThread->start();
}

ModbusController::~ModbusController() 
{
    if (m_modbusThread->isRunning()) 
    {
        // 安全地通知子线程中的 worker 停止通讯
        QMetaObject::invokeMethod(m_worker, "stopConnection", Qt::QueuedConnection);

        m_modbusThread->quit();      // 通知线程退出
        m_modbusThread->wait();      // 阻塞等待线程完全清理结束
    }

    // 释放 UI 内存
    delete m_ui;
}

void ModbusController::setupConnections() 
{
    // 线程生命周期清理逻辑
    connect(m_modbusThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // UI 指令 -> Worker (跨线程，自动转化为队列连接)
    connect(m_ui, &ModbusWidget::requestConnect, m_worker, &ModbusWorker::startConnection);
    connect(m_ui, &ModbusWidget::requestDisconnect, m_worker, &ModbusWorker::stopConnection);
    connect(m_ui, &ModbusWidget::stationSelectionChanged, m_worker, &ModbusWorker::changeActiveStation);

    // Worker 反馈 -> UI 刷新 (跨线程，自动转化为队列连接)
    connect(m_worker, &ModbusWorker::logMessageReady, m_ui, &ModbusWidget::appendLog);
    connect(m_worker, &ModbusWorker::pollingStatusChanged, m_ui, &ModbusWidget::updateConnectionStatus);
    connect(m_worker, &ModbusWorker::modbusDataReceived, m_ui, &ModbusWidget::onModbusDataReceived);
}

void ModbusController::showWindow() 
{
    if (m_ui) 
    {
        m_ui->show();
    }
}
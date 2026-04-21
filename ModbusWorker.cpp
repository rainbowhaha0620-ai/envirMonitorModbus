#include "ModbusWorker.h"
#include <QDebug>
#include <QThread>

ModbusWorker::ModbusWorker(QObject* parent): QObject(parent), m_ctx(nullptr), m_pollTimer(nullptr),m_currentStationIndex(0),m_activeStationId(0x01) 
{
    m_stationQueue << 0x01 << 0x02 << 0x03;
}

ModbusWorker::~ModbusWorker() 
{
    stopConnection();
}

void ModbusWorker::startConnection() 
{
    // 确保在工作线程中创建定时器 (QTimer 必须与创建它的线程属于同一线程)
    if (!m_pollTimer) 
    {
        m_pollTimer = new QTimer(this);
        connect(m_pollTimer, &QTimer::timeout, this, &ModbusWorker::executePolling);
    }

    // 1. 初始化TCP
    m_ctx = modbus_new_tcp("127.0.0.1", 2002);
    if (m_ctx == nullptr) 
    {
        emit logMessageReady("ERROR", u8"无法分配 libmodbus 上下文");
        return;
    }

    // 设置超时时间 (非常重要，防止阻塞太久)
    modbus_set_response_timeout(m_ctx, 5, 0); // 1秒 0微秒

    emit logMessageReady("INFO", u8"正在通过 libmodbus 尝试连接...");

    // 2. 建立连接
    if (modbus_connect(m_ctx) == -1) 
    {
        QString errStr = QString(u8"连接失败: %1").arg(modbus_strerror(errno));
        emit logMessageReady("ERROR", errStr);
        emit pollingStatusChanged(false, u8"连接失败");

        modbus_free(m_ctx);
        m_ctx = nullptr;
        return;
    }

    emit logMessageReady("INFO", u8"libmodbus 连接成功！启动轮询引擎。");
    emit pollingStatusChanged(true, u8"正在轮询 (500ms)");

    // 3. 启动定时器，开始轮询
    m_pollTimer->start(500);
}

void ModbusWorker::stopConnection() 
{
    if (m_pollTimer && m_pollTimer->isActive()) 
    {
        m_pollTimer->stop();
    }

    if (m_ctx) 
    {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;

        emit logMessageReady("WARN", u8"libmodbus 连接已安全断开");
        emit pollingStatusChanged(false, u8"已断开");
    }
}

void ModbusWorker::changeActiveStation(int stationId) 
{
    m_activeStationId = stationId;
}

void ModbusWorker::executePolling() 
{
    if (!m_ctx) return;

    int stationId = m_stationQueue.at(m_currentStationIndex);

    // 设置当前的从站 ID
    modbus_set_slave(m_ctx, stationId);

    // 根据站号派发不同的读取任务
    switch (stationId) 
    {
    case 0x01: readStation0x01(); break;
    case 0x02: readStation0x02(); break;
    case 0x03: readStation0x03(); break;
    }

    // 循环切换下一个站
    m_currentStationIndex = (m_currentStationIndex + 1) % m_stationQueue.size();
}

void ModbusWorker::readStation0x01() 
{
    uint16_t tab_reg[6];

    // 读取保持寄存器
    int rc = modbus_read_registers(m_ctx, 0, 6, tab_reg);

    if (rc == -1) 
    {
        emit logMessageReady("WARN", QString(u8"站 #01 响应超时: %1").arg(modbus_strerror(errno)));
        return;
    }

    // 解析数据并发给 UI
    // 假设 tab_reg[0]是温度, tab_reg[2]是湿度，tab_reg[4]是露点值(全部放大10倍)
    float temp = tab_reg[0] / 10.0f;
    float hum = tab_reg[2] / 10.0f;

    QString tempStr = QString("%1 °C").arg(temp, 0, 'f', 1);
    QString tempStatus = (temp > 30.0f) ? u8"高温报警" : u8"正常";
    emit modbusDataReceived(0x01, 0, tempStr, tempStatus); // 第0行：温度

    QString humStr = QString("%1 %").arg(hum, 0, 'f', 1);
    emit modbusDataReceived(0x01, 1, humStr, u8"正常");   // 第1行：湿度
 
    float dewPoint = tab_reg[4] / 10.0f;
    QString dewStr = QString("%1 °C").arg(dewPoint, 0, 'f', 1);
    QString dewStatus = (dewPoint < 15.0f) ? u8"低报警" : u8"正常";
    emit modbusDataReceived(0x01, 2, dewStr, dewStatus);  // 第2行：露点值

    emit logMessageReady("RECV", u8"成功读取 站 #01 (温湿度表)");
}

void ModbusWorker::readStation0x02() 
{
    uint16_t tab_reg[4];
    // 读取输入寄存器 (功能码 04), 从地址 3 开始读取 4 个输入寄存器
    int rc = modbus_read_input_registers(m_ctx, 3, 4, tab_reg);

    if (rc == -1) 
    {
        emit logMessageReady("WARN", QString(u8"站 #02 读取失败: %1").arg(modbus_strerror(errno)));
        return;
    }

    //1. 解析 A相电压 (Float32, 占用前两个寄存器)
    float voltage = modbus_get_float_abcd(tab_reg);//大端模式逆向拼回float

    QString volStr = QString("%1 V").arg(voltage, 0, 'f', 1);   // 保留一位小数
    QString volStatus = (voltage > 240.0f) ? u8"高压报警" : u8"正常";
    emit modbusDataReceived(0x02, 0, volStr, volStatus);        //第0行,A相电压

    // 2. 解析 有功功率 (Float32, 占用后两个寄存器)  
    float power = modbus_get_float_abcd(&tab_reg[2]);

    QString pwrStr = QString("%1 kW").arg(power, 0, 'f', 2);    // 功率通常保留两位小数
    QString pwrStatus = (power > 50.0f) ? u8"过载报警" : u8"正常";
    emit modbusDataReceived(0x02, 1, pwrStr, pwrStatus);        //第1行,有功功率

    emit logMessageReady("RECV", u8"成功读取 站 #02 (电量模块: 电压 & 功率)");
}

void ModbusWorker::readStation0x03() 
{
    uint8_t tab_bits[1];
    // 读取线圈状态 (功能码 01)
    int rc = modbus_read_bits(m_ctx, 0, 1, tab_bits);

    if (rc == -1) 
    {
        emit logMessageReady("WARN", QString(u8"站 #03 读取失败: %1").arg(modbus_strerror(errno)));
        return;
    }

    QString stateStr = (tab_bits[0] == 1) ? u8"开启" : u8"关闭";
    emit modbusDataReceived(0x03, 0, stateStr, u8"正常");

    emit logMessageReady("RECV", u8"成功读取 站 #03 (I/O模块)");
}
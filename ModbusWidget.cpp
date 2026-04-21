#include <QDateTime>
#include <QHeaderView>
#include "modbuswidget.h"

ModbusWidget::ModbusWidget(QWidget* parent): QMainWindow(parent) 
{
    setupUI();
    initMockData();
}

ModbusWidget::~ModbusWidget() 
{}

void ModbusWidget::onModbusDataReceived(int stationId, int row, const QString& newValue, const QString& newStatus)
{
    m_dataModel->updateVariableValue(stationId,row, newValue, newStatus);
}

void ModbusWidget::setupUI()
{
    setWindowTitle(u8"智能工业环境监测系统 (Modbus TCP/RTU)");

    // 主中心部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // ================= 1. 顶部工具栏及状态 =================
    QHBoxLayout* topLayout = new QHBoxLayout();
    btnConnect = new QPushButton(u8"建立连接", this);
    btnDisconnect = new QPushButton(u8"断开连接", this);
    btnSettings = new QPushButton(u8"参数设置", this);
    btnExport = new QPushButton(u8"导出报文", this);

    lblStatus = new QLabel(u8"状态: ● 未连接", this);
    lblStatus->setStyleSheet("color: gray; font-weight: bold;");

    topLayout->addWidget(btnConnect);
    topLayout->addWidget(btnDisconnect);
    topLayout->addWidget(btnSettings);
    topLayout->addWidget(btnExport);
    topLayout->addStretch(); 
    topLayout->addWidget(lblStatus);

    mainLayout->addLayout(topLayout);

    // ================= 2. 核心分割器 (上下分割: 数据区 / 日志区) =================
    QSplitter* mainVerticalSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(mainVerticalSplitter);

    // === 2.1 顶部数据区 (左右分割: 站点树 / 变量表) ===
    QSplitter* topHorizontalSplitter = new QSplitter(Qt::Horizontal, mainVerticalSplitter);

    // 左侧：站点列表
    QGroupBox* groupStations = new QGroupBox(u8"站点列表", topHorizontalSplitter);
    QVBoxLayout* treeLayout = new QVBoxLayout(groupStations);
    treeStations = new QTreeWidget(groupStations);
    treeStations->setHeaderHidden(true);
    treeLayout->addWidget(treeStations);
    treeLayout->setContentsMargins(2, 2, 2, 2);

    // 右侧：站点详情与数据表
    QWidget* rightWidget = new QWidget(topHorizontalSplitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    lblStationDetails = new QLabel(u8"站点详情: 请选择站点", rightWidget);
    lblStationDetails->setStyleSheet("background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc;");
    rightLayout->addWidget(lblStationDetails);

    tableViewVariables = new QTableView(rightWidget);
    m_dataModel = new ModbusTableModel(this);
    tableViewVariables->setModel(m_dataModel);

    tableViewVariables->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽自适应
    tableViewVariables->setEditTriggers(QAbstractItemView::NoEditTriggers); // 只读
    tableViewVariables->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选中
    tableViewVariables->setSelectionMode(QAbstractItemView::SingleSelection); // 单选
    tableViewVariables->verticalHeader()->setVisible(false); // 隐藏左侧行号
    rightLayout->addWidget(tableViewVariables);

    // 设置左右面板的初始比例 (大概 1:3)
    topHorizontalSplitter->addWidget(groupStations);
    topHorizontalSplitter->addWidget(rightWidget);
    topHorizontalSplitter->setStretchFactor(0, 1);
    topHorizontalSplitter->setStretchFactor(1, 3);

    // === 2.2 底部日志区 ===
    QGroupBox* groupLogs = new QGroupBox(u8"系统日志", mainVerticalSplitter);
    QVBoxLayout* logLayout = new QVBoxLayout(groupLogs);
    textEditLogs = new QPlainTextEdit(groupLogs);
    textEditLogs->setReadOnly(true);
    textEditLogs->setMaximumBlockCount(1000); // 限制最大行数，防止内存溢出
    logLayout->addWidget(textEditLogs);
    logLayout->setContentsMargins(2, 2, 2, 2);

    // 设置上下区域的初始比例 (大概 3:1)
    mainVerticalSplitter->addWidget(topHorizontalSplitter);
    mainVerticalSplitter->addWidget(groupLogs);
    mainVerticalSplitter->setStretchFactor(0, 3);
    mainVerticalSplitter->setStretchFactor(1, 1);

    // ================= 3. 信号与槽绑定 =================
    connect(btnConnect, &QPushButton::clicked, this, &ModbusWidget::requestConnect);
    connect(btnDisconnect, &QPushButton::clicked, this, &ModbusWidget::requestDisconnect);
    connect(btnSettings, &QPushButton::clicked, this, &ModbusWidget::requestSettings);
    connect(btnExport, &QPushButton::clicked, this, &ModbusWidget::requestExport);
    connect(treeStations, &QTreeWidget::itemClicked, this, &ModbusWidget::onStationTreeItemClicked);
}

void ModbusWidget::initMockData() 
{
    QTreeWidgetItem* factoryItem = new QTreeWidgetItem(treeStations, { u8"工厂 01 生产线" });

    QTreeWidgetItem* station1 = new QTreeWidgetItem(factoryItem, { u8"● 站 #01 温湿度表" });
    station1->setData(0, Qt::UserRole, 0x01); // 绑定 0x01

    QTreeWidgetItem* station2 = new QTreeWidgetItem(factoryItem, { u8"● 站 #02 电量模块" });
    station2->setData(0, Qt::UserRole, 0x02); // 绑定 0x02

    QTreeWidgetItem* station3 = new QTreeWidgetItem(factoryItem, { u8"○ 站 #03 I/O模块" });
    station3->setData(0, Qt::UserRole, 0x03); // 绑定 0x03

    treeStations->expandAll();

    // 初始状态更新
    updateConnectionStatus(true, u8"正在轮询 (50ms)");
    updateStationDetails(u8"#01 恒温控制柜 (127.0.0.1)");

    // 2. 初始化总数据池
    QList<ModbusVariable> allData;

    // 0x01 温湿度表的数据
    allData.append({ 0x01, u8"环境温度", u8"0°C", u8"40000", u8"Float32", u8"正常" });
    allData.append({ 0x01, u8"环境湿度", u8"0%", u8"40002", u8"Float32", u8"正常" });
    allData.append({ 0x01, u8"露点值", u8"0°C", u8"40004", u8"Float32", u8"正常" });

    // 0x02 电量模块的数据
    allData.append({ 0x02, u8"A相电压", u8"0V", u8"30003", u8"Float32", u8"正常" });
    allData.append({ 0x02, u8"有功功率", u8"0kW", u8"30005", u8"Float32", u8"正常" });

    // 0x03 I/O模块的数据
    allData.append({ 0x03, u8"阀门状态", u8"开启", u8"00001", u8"Bool", u8"正常" });

    // 将总数据灌入 Model
    m_dataModel->setVariables(allData);

    // 默认第一个站
    treeStations->setCurrentItem(station1);
    onStationTreeItemClicked(station1, 0);
}

void ModbusWidget::updateConnectionStatus(bool isConnected, const QString& statusText) 
{
    if (isConnected) 
    {
        lblStatus->setText(QString(u8"状态: ● %1").arg(statusText));
        lblStatus->setStyleSheet("color: green; font-weight: bold;");
    }
    else 
    {
        lblStatus->setText(QString(u8"状态: ○ %1").arg(statusText));
        lblStatus->setStyleSheet("color: gray; font-weight: bold;");
    }
}

void ModbusWidget::appendLog(const QString& level, const QString& message) 
{
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logLine = QString("[%1] [%2] %3").arg(timeStr).arg(level).arg(message);

    // 如果是警告或错误，可以通过 HTML 设置颜色
    if (level == "WARN") 
    {
        logLine = QString("<span style='color:#FF8C00;'>%1</span>").arg(logLine);
    }
    else if (level == "ERROR")
    {
        logLine = QString("<span style='color:red;'>%1</span>").arg(logLine);
    }
    else if (level == "RECV") 
    {
        logLine = QString("<span style='color:blue;'>%1</span>").arg(logLine);
    }

    textEditLogs->appendHtml(logLine);
}

void ModbusWidget::updateStationDetails(const QString& details) 
{
    lblStationDetails->setText(QString(u8"站点详情: %1").arg(details));
}

void ModbusWidget::onStationTreeItemClicked(QTreeWidgetItem* item, int column) 
{
    Q_UNUSED(column);

    // 取出绑定的站ID (父节点，取出来的是0)
    bool ok;
    int stationId = item->data(0, Qt::UserRole).toInt(&ok);

    if (ok && stationId > 0) 
    {
        // 1. 表格设置当前站
        m_dataModel->setStationFilter(stationId);

        // 2. 更新右上角的标题，以十六进制
        QString hexId = QString("0x%1").arg(stationId, 2, 16, QChar('0'));
        updateStationDetails(QString(u8"%1 (%2)").arg(item->text(0)).arg(hexId));

        // 3. 将站号发给底层 Modbus 通讯类
        emit stationSelectionChanged(stationId);
    }
}
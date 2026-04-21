#pragma once
#include <QMainWindow>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "ModbusTableModel.h"

class ModbusWidget : public QMainWindow 
{
    Q_OBJECT

public:
    explicit ModbusWidget(QWidget* parent = nullptr);
    ~ModbusWidget() override;

    /// <summary>
    /// 更新连接状态
    /// </summary>
    /// <param name="isConnected">连接状态</param>
    /// <param name="statusText">文本</param>
    void updateConnectionStatus(bool isConnected, const QString& statusText);

    /// <summary>
    /// 写入日志
    /// </summary>
    /// <param name="level">日志等级</param>
    /// <param name="message">日志信息</param>
    void appendLog(const QString& level, const QString& message);

    /// <summary>
    /// 刷新表格上方的站点详情
    /// </summary>
    /// <param name="details">详情内容</param>
    void updateStationDetails(const QString& details);

signals:
    //连接
    void requestConnect();
    //断开连接
    void requestDisconnect();
    //参数设置
    void requestSettings();
    //导出报文
    void requestExport();
    // 通知当前站ID
    void stationSelectionChanged(int stationId);

public slots:
    //站树点击
    void onStationTreeItemClicked(QTreeWidgetItem* item, int column);

    //某个站接收到数据时,刷新表格
    void onModbusDataReceived(int stationId, int row, const QString& newValue, const QString& newStatus);

private:
    //初始化UI
    void setupUI();
    // 初始化数据
    void initMockData(); 

    // 顶部工具栏
    QPushButton* btnConnect;
    QPushButton* btnDisconnect;
    QPushButton* btnSettings;
    QPushButton* btnExport;
    QLabel* lblStatus;

    // 左侧站点列表
    QTreeWidget* treeStations;

    // 右侧内容区
    QLabel* lblStationDetails;
    QTableView* tableViewVariables;
    ModbusTableModel* m_dataModel;

    // 底部系统日志
    QPlainTextEdit* textEditLogs;
};

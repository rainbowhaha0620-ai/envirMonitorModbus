#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QColor>

// 定义单行数据的结构体
struct ModbusVariable {
    int stationId;
    QString name;
    QString value;
    QString address;
    QString type;
    QString status;
};

class ModbusTableModel : public QAbstractTableModel 
{
    Q_OBJECT

public:
    explicit ModbusTableModel(QObject* parent = nullptr);

    // 必须重写的四个基础方法
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // --- 自定义的数据更新接口 ---

    // 1. 初始化或全量替换数据
    void setVariables(const QList<ModbusVariable>& vars);

    // 2. 高频局部刷新：仅更新指定行的数值和状态 (Modbus 轮询时调用这个)
    void updateVariableValue(int stationId, int relativeRow, const QString& newValue, const QString& newStatus);

    // 设置显示的站点 ID，用于过滤表格数据
    void setStationFilter(int stationId);

private:
    QList<ModbusVariable> m_allVariables;     // 存储所有站点的全量数据
    QList<ModbusVariable> m_displayVariables; // 实际在表格中显示的数据（过滤后）
    QStringList m_headers;

    int m_currentStationId = -1;
};
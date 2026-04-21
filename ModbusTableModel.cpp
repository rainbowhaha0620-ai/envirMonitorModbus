#include "ModbusTableModel.h"
#include <QFont>

ModbusTableModel::ModbusTableModel(QObject* parent)
    : QAbstractTableModel(parent) {
    m_headers << u8"变量名" << u8"当前值" << u8"寄存器地址" << u8"数据类型" << u8"状态";
}

int ModbusTableModel::rowCount(const QModelIndex& parent) const 
{
    if (parent.isValid()) return 0;
    return m_displayVariables.size();
}

int ModbusTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_headers.size();
}

QVariant ModbusTableModel::data(const QModelIndex& index, int role) const 
{
    if (!index.isValid() || index.row() >= m_displayVariables.size())
        return QVariant();

    const ModbusVariable& var = m_displayVariables.at(index.row());

    // 处理显示文本
    if (role == Qt::DisplayRole) 
    {
        switch (index.column()) 
        {
            case 0: return var.name;
            case 1: return var.value;
            case 2: return var.address;
            case 3: return var.type;
            case 4: return var.status;
            default: return QVariant();
        }
    }

    // 文本对齐方式
    else if (role == Qt::TextAlignmentRole) 
    {
        return Qt::AlignCenter;
    }
    // 字体颜色 (针对状态列进行报警标红)
    else if (role == Qt::ForegroundRole) 
    {
        if (index.column() == 4) 
        {
            if (var.status.contains(u8"报警")) 
            {
                return QColor(Qt::red);
            }
            else 
            {
                return QColor(Qt::darkGreen);
            }
        }
    }

    return QVariant();
}

QVariant ModbusTableModel::headerData(int section, Qt::Orientation orientation, int role) const 
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) 
    {
        if (section >= 0 && section < m_headers.size()) 
        {
            return m_headers.at(section);
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void ModbusTableModel::setVariables(const QList<ModbusVariable>& vars) 
{
    m_allVariables = vars;
    setStationFilter(-1); // 默认不显示或显示全部，这里假设 -1 为清空或初始状态
}

void ModbusTableModel::updateVariableValue(int stationId, int relativeRow, const QString& newValue, const QString& newStatus)
{
    //寻找并更新大仓库
    int absoluteIndex = -1;
    int matchCount = 0;

    for (int i = 0; i < m_allVariables.size(); ++i)
    {
        if (m_allVariables[i].stationId == stationId) 
        {
            if (matchCount == relativeRow)
            {
                absoluteIndex = i; // 在大仓库中的真实位置
                break;
            }
            matchCount++;
        }
    }

    if (absoluteIndex != -1) 
    {
        m_allVariables[absoluteIndex].value = newValue;
        m_allVariables[absoluteIndex].status = newStatus;
    }
    else 
    {
        return; 
    }

    //判断是否需要刷新界面,如果当前收到的数据，正是用户正在看的站，才需要刷新界面
    if (m_currentStationId == stationId) 
    {
        if (relativeRow >= 0 && relativeRow < m_displayVariables.size()) 
        {
            // 1. 同步更新展示橱窗里的数据
            m_displayVariables[relativeRow].value = newValue;
            m_displayVariables[relativeRow].status = newStatus;

            // 2. 触发 UI 局部重绘,我们只更新第1列(数值)到第4列(状态)
            QModelIndex topLeft = index(relativeRow, 1);
            QModelIndex bottomRight = index(relativeRow, 4);

            // 通知 QTableView 去重新调用 data() 函数读取这几个格子的新数据
            emit dataChanged(topLeft, bottomRight, { Qt::DisplayRole, Qt::ForegroundRole });
        }
    }
}

void ModbusTableModel::setStationFilter(int stationId)
{
    m_currentStationId = stationId;

    beginResetModel();
    m_displayVariables.clear();

    // 遍历所有数据，只挑出符合当前站 ID 的变量
    for (const ModbusVariable& var : m_allVariables)
    {
        if (var.stationId == stationId) 
        {
            m_displayVariables.append(var);
        }
    }
    endResetModel();
}

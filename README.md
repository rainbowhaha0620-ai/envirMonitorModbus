🏭 智能工业环境监测系统 (Modbus Monitoring System)
本项目是一个基于 C++17 和 Qt6 开发的工业级环境与设备监测上位机软件。系统底层采用libmodbus 纯 C 库进行通讯，结合 Qt 的 Model/View 架构 与 Worker-Object 多线程模式，实现了极高并发下的丝滑界面与稳定轮询。

✨ 核心特性
🛡️ 绝对的非阻塞 UI：摒弃 UI 线程直连 Modbus。采用独立子线程 (QThread) 承载 libmodbus 的同步阻塞调用，即使网络断开或从机死机（1秒超时），主界面依然保持 120Hz 级别的丝滑响应。

⚡ 高性能表格渲染：基于 QAbstractTableModel + QTableView 实现了数据展示层。通过双缓存（总数据池 + 当前展示池）和精确坐标的局部重绘 (dataChanged)，完美应对毫秒级的高频轮询，拒绝整表刷新带来的内存碎片与闪烁。

🧩 模块化 MVC 架构：

ModbusController：生命周期管家，负责线程编排与安全退出。
ModbusWidget：纯粹的展示层，只负责画图与触发信号。
ModbusWorker：纯粹的逻辑层，无 UI 依赖，负责协议解析与断线重连。

🌐 自动节点轮询与自愈：内置轮询引擎，支持多站点（温湿度、电量、IO等）交叉轮询。具备完整的超时重发和底层错误捕获机制。

🧮 工业数据解析：内置 Float32 (IEEE 754) 跨寄存器拼接解析。

🛠️ 技术栈
语言: C++17

GUI 框架: Qt 6.x

通讯底层: libmodbus (TCP)

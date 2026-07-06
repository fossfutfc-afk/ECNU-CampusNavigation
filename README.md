# CampusNavigation 校园导航系统

数据结构课程项目——动态图上的校园路线规划与设施分析系统。支持顶点与边的动态增删改、双模式Dijkstra（距离/时间）、时刻约束Dijkstra、必经点Dijkstra、MST、连通分量分析，Brute Force 关键节点/关键边检测。加分项包含 状态 Dijkstra、Tarjan算法、自定义数据和 Web 图形化界面。

## 快速开始

```bash
# 操作目录
cd Project

# 删除目录 需管理员权限
rmdir /s build

# g++ 编译
g++ -std=c++17 -O2 -o build\CampusNavigation.exe src\*.cpp -I include

# cmake 一行编译
mkdir build && cd build && cmake .. -G "MinGW Makefiles" && cmake --build .

# CLI 评测示例
cd data\Must\small_cases\case_01 && ..\..\..\..\build\CampusNavigation.exe < command.txt > my_answer.txt
fc answer.txt my_answer.txt

# 批量测试
g++ -std=c++17 -O2 -o test\testall.exe test\testall.cpp
test\testall.exe

# Web 图形化界面
cd web && pip install flask && python bridge.py
```

> CMake 构建方式、评测命令详解、testall 全部筛选参数、Web 界面功能说明详见 **[用法指南](docs/用法指南.md)**。

## 文档

| 文档 | 说明 |
|------|------|
| [实验报告](docs/实验报告.md) | 数据结构设计、算法分析、测试方案、问题排查 |
| [用法指南](docs/用法指南.md) | 编译、运行、测试框架、Web 图形化界面详细介绍 |
| [拓展任务](docs/拓展任务.md) | SHORTEST_K、自定义数据集、Tarjan、Web 图形化 |
| [创新数据集](docs/创新数据集.md) | 自定义测试数据设计文档 |
| [AI 协作记录](docs/AI协作记录.md) | 关键对话摘录、调试故事、拒绝 AI 建议案例 |

## 项目结构

```
CampusNavigation/
├── CMakeLists.txt
├── src/            # C++ 实现
├── include/        # 头文件
├── data/           # 测试数据（Must / Addition / Comparison / Confrontation / Debug）
├── test/           # 测试框架 testall.cpp
├── web/            # Web 图形化界面（Flask + Leaflet）
├── docs/           # 文档（报告 / 指南 / 需求 / 辅助）
└── build/          # 编译产物（gitignore）
```

## 头文件包含关系

```
                  main.cpp
                  (主函数)
                     │
             CommandProcessor.h
                (命令行处理)
                ╱         ╲ 
               ╱           ╲
            CsvIO.h    GraphAlgorithm.h
           (文件读写)     (图算法扩展)
               ╲            ╱   
                ╲          ╱
                 ╲        ╱    
                  LGraph.h
               (核心图数据结构)
                  ╱       ╲
                 ╱         ╲ 
      GraphElements.h  GraphException.h
       (辅助数据结构)      (错误处理)
```

| 使用者 | 依赖 | 原因 |
|--------|------|------|
| `main.cpp` | `CommandProcessor.h` | 创建图对象和命令处理器 |
| `CommandProcessor.h` | `CsvIO.h` + `GraphAlgorithm.h` | 调用图操作、CSV 读写、算法 |
| `CsvIO.h` | `LGraph.h` | 读写顶点数组，遍历图导出边 |
| `GraphAlgorithm.h` | `LGraph.h` | 算法通过 LGraph 公共接口遍历图 |
| `LGraph.h` | `GraphElements.h` + `GraphException.h` | 顶点与边建模 + 异常传递 |
| `GraphElements.h` | `NULL` | 纯数据 struct，无依赖 |
| `GraphException.h` | `<exception>` | 继承标准异常类 |

> `test/testall.cpp` 独立编译，不依赖项目头文件——通过 `system()` 调用编译好的 `CampusNavigation.exe` 并捕获输出。

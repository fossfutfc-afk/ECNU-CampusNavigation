//
// 动态图上的校园路线规划与设施分析系统
// CommandProcessor.h - 命令解析与分发模块
//

#ifndef CAMPUSNAVIGATION_COMMANDPROCESSOR_H
#define CAMPUSNAVIGATION_COMMANDPROCESSOR_H

#include <string>
#include <sstream>
#include "GraphAlgorithm.h"
#include "CsvIO.h"

namespace Graph {

    class CommandProcessor {
    private:
        LGraph &graph;

        // ==================== 命令处理函数 ====================
        void cmdLoad(std::istringstream &args);
        void cmdSave(std::istringstream &args);
        void cmdQueryPlace(std::istringstream &args);
        void cmdQueryCategory(std::istringstream &args);
        void cmdAdj(std::istringstream &args);
        void cmdAddPlace(std::istringstream &args);
        void cmdDeletePlace(std::istringstream &args);
        void cmdUpdatePlace(std::istringstream &args);
        void cmdAddRoad(std::istringstream &args);
        void cmdDeleteRoad(std::istringstream &args);
        void cmdUpdateRoad(std::istringstream &args);
        void cmdCloseRoad(std::istringstream &args);
        void cmdOpenRoad(std::istringstream &args);
        void cmdComponents();
        void cmdShortest(std::istringstream &args);
        void cmdTimedShortest(std::istringstream &args);
        void cmdMustPass(std::istringstream &args);
        void cmdShortestK(std::istringstream &args);   // 拓展任务 1：共享单车
        void cmdMst();
        void cmdCritical();
        void cmdCriticalTarjan();   // 加分项：Tarjan O(V+E) 版

    public:
        explicit CommandProcessor(LGraph &graph) : graph(graph) {}

        // 处理一条命令行，输出结果到 stdout
        // 返回 false 表示遇到 QUIT 命令，程序应退出
        bool ProcessCommand(const std::string &line);
    };

}

#endif //CAMPUSNAVIGATION_COMMANDPROCESSOR_H

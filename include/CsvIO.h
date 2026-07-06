//
// 动态图上的校园路线规划与设施分析系统
// CsvIO.h - CSV 文件读写模块
//

#ifndef CAMPUSNAVIGATION_CSVIO_H
#define CAMPUSNAVIGATION_CSVIO_H

#include <string>
#include <vector>
#include "LGraph.h"

namespace Graph {

    // 道路原始数据（从 CSV 读入时使用）
    struct RoadRecord {
        std::string from_id;
        std::string to_id;
        int distance;
        int walk_time;
        std::string status;
    };

    namespace CsvIO {
        // 从 CSV 文件读取地点信息
        // 兼容带表头（首行以 place_id 开头则跳过）和不带表头两种格式
        // 格式：place_id,display_name,category,stay_time,open_time,close_time
        std::vector<LocationInfo> ReadPlaces(const std::string &path);

        // 从 CSV 文件读取道路信息
        // 兼容带表头（首行以 from_id 开头则跳过）和不带表头两种格式
        // 格式：from_id,to_id,distance,walk_time,status
        std::vector<RoadRecord> ReadRoads(const std::string &path);

        // 将当前图的地点信息写回 CSV（带表头）
        bool WritePlaces(const std::string &path, const LGraph &graph);

        // 将当前图的道路信息写回 CSV（带表头）
        bool WriteRoads(const std::string &path, const LGraph &graph);
    }
}

#endif //CAMPUSNAVIGATION_CSVIO_H

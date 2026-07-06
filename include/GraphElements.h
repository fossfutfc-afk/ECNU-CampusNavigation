//
// 动态图上的校园路线规划与设施分析系统
// GraphElements.h - 地点与边的数据结构声明
//

#ifndef CAMPUSNAVIGATION_GRAPHELEMENTS_H
#define CAMPUSNAVIGATION_GRAPHELEMENTS_H

#include <string>
#include <utility>

namespace Graph {

    // ==================== 时间校验工具函数 ====================
    // 校验 HH:MM 格式并补齐前导零（如 "8:5" → "08:05"）
    std::string TimeValidator(const std::string &time);

    // ==================== 地点信息 ====================
    struct LocationInfo {
        std::string place_id;
        std::string display_name;
        std::string category;
        int stay_time{0};
        std::string open_time;
        std::string close_time;

        LocationInfo();
        LocationInfo(std::string place_id_, std::string display_name_,
                     std::string category_, int stay_time_,
                     std::string open_time_, std::string close_time_);

        // 自检：open_time ≤ close_time 且 stay_time ≥ 0
        void ValidCheck() const;
    };

    // ==================== 边（道路）信息 ====================
    struct EdgeNode {
        std::string from_id;
        std::string to_id;
        int distance;
        int walk_time;
        std::string status;          /* "open" 或 "closed" */

        EdgeNode();
        EdgeNode(std::string f, std::string t, int d, int w, std::string s);

        // 自检：status ∈ {open, closed}，distance ≥ 0，walk_time ≥ 0
        void ValidCheck() const;
    };

} // Graph

#endif // CAMPUSNAVIGATION_GRAPHELEMENTS_H

//
// 动态图上的校园路线规划与设施分析系统
// GraphElements.cpp - 地点/边数据结构 + 时间校验实现
//

#include "GraphElements.h"
#include "GraphException.h"
#include <sstream>
#include <cctype>

namespace Graph {

    // ==================== 时间校验 ====================

    std::string TimeValidator(const std::string &time) {
        std::stringstream ss(time);
        std::string hour_token, minute_token;

        if (!std::getline(ss, hour_token, ':') || !std::getline(ss, minute_token, ':')) {
            throw GraphException("TimeValidator:时间格式错误,错误时间: <"
                                 + time + "> ,请按照xx:xx或x:x的格式输入");
        }
        std::string remaining;
        if (std::getline(ss, remaining, '\0')) {
            throw GraphException("TimeValidator:时间格式错误,错误时间: <"
                                 + time + "> ,请按照xx:xx或x:x的格式输入");
        }

        auto valid = [&](const std::string& token, int max, const std::string& name) -> void {
            if (token.empty() || token.size() > 2) {
                throw GraphException("TimeValidator: <" + name + "> 长度错误,错误时间 <" + token + ">");
            }
            for (char c : token) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    throw GraphException("TimeValidator: <" + name + "> 包含非数字字符,错误字符 <" 
                                         + std::string(1, c) + ">");
                }
            }
            int val = std::stoi(token);
            if (val < 0 || val > max) {
                throw GraphException("TimeValidator: <" + name + "> 超出范围[0~" 
                                     + std::to_string(max) + "],错误时间 <" + token + ">");
            }
        };
        valid(hour_token, 23, "Hour");
        valid(minute_token, 59, "Minute");

        return ((hour_token.size() == 1ull) ? "0" : "") + hour_token + ":"
             + ((minute_token.size() == 1ull) ? "0" : "") + minute_token;
    }

    // ==================== LocationInfo ====================

    LocationInfo::LocationInfo() : stay_time(0) {}

    LocationInfo::LocationInfo(std::string place_id_, std::string display_name_,
                               std::string category_, int stay_time_,
                               std::string open_time_, std::string close_time_)
            : place_id(std::move(place_id_)),
              display_name(std::move(display_name_)),
              category(std::move(category_)),
              stay_time(stay_time_),
              open_time(std::move(open_time_)),
              close_time(std::move(close_time_)) {}

    void LocationInfo::ValidCheck() const {
        std::string open  = TimeValidator(open_time);
        std::string close = TimeValidator(close_time);
        if (open > close) {
            throw GraphException("LocationInfo::ValidCheck:时间数据不合法, <"
                                 + open_time + "> 时间大于 <" + close_time + "> 时间");
        }
        if (stay_time < 0) {
            throw GraphException("LocationInfo::ValidCheck:停留时间数据不合法,停留时间 <"
                                 + std::to_string(stay_time) + "> 是负数");
        }
    }

    // ==================== EdgeNode ====================

    EdgeNode::EdgeNode() : distance(0), walk_time(0), status("open") {}

    EdgeNode::EdgeNode(std::string f, std::string t, int d, int w, std::string s)
            : from_id(std::move(f)), to_id(std::move(t)),
              distance(d), walk_time(w), status(std::move(s)) {}

    void EdgeNode::ValidCheck() const {
        if (status != "closed" && status != "open") {
            throw GraphException("EdgeNode::ValidCheck:状态数据不合法,状态: <"
                                 + status + "> 不能是open与closed以外的参数");
        }
        if (distance < 0) {
            throw GraphException("EdgeNode::ValidCheck:距离数据不合法,距离: <"
                                 + std::to_string(distance) + "> 是负数");
        }
        if (walk_time < 0) {
            throw GraphException("EdgeNode::ValidCheck:步行时间数据不合法,步行时间: <"
                                 + std::to_string(walk_time) + "> 是负数");
        }
    }

} // Graph

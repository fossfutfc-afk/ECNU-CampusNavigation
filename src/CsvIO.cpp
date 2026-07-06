//
// 动态图上的校园路线规划与设施分析系统
// CsvIO.cpp - CSV 文件读写实现
//

#include "../include/CsvIO.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace Graph {
    namespace CsvIO {

        std::vector<LocationInfo> ReadPlaces(const std::string &path) {
            std::vector<LocationInfo> places{};
            std::ifstream file(path);
            if (!file.is_open()) {
                throw GraphException("CsvIO::ReadPlaces发生错误:path.csv打开失败，请检查路径格式，当前文件路径:" + path);
                return places;
            }

            std::string line;
            bool is_first_line = true;
            while (std::getline(file, line)) {
                // 去除行首尾空白
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), line.end());

                // 跳过空行
                if (line.empty()) continue;

                // 处理表头：首行若以 "place_id" 开头（忽略大小写）则跳过
                if (is_first_line) {
                    is_first_line = false;
                    std::string lower_line = line;
                    std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
                    if (lower_line.find("place_id") == 0) {
                        continue;   // 跳过表头行
                    }
                }

                // 将逗号替换为空格，便于 stringstream 解析
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream iss(line);
                std::string place_id, display_name, category, open_time, close_time;
                int stay_time;

                // 尝试解析五个字段（注意 open_time 和 close_time 为字符串）
                if (!(iss >> place_id >> display_name >> category >> stay_time >> open_time >> close_time)) {
                    throw GraphException("CsvIO::ReadPlaces发生错误:places.csv字段错误，请检查文件格式，当前文件路径"  + path);
                    continue;
                }

                // 构造 LocationInfo 对象并加入结果集
                // 假定 LocationInfo 具有构造函数 LocationInfo(id, name, cat, stay, open, close)
                places.emplace_back(LocationInfo{place_id, display_name, category, stay_time, open_time, close_time});
            }

            return places;
        }

        std::vector<RoadRecord> ReadRoads(const std::string &path) {
            std::vector<RoadRecord> roads;
            std::ifstream file(path);
            if (!file.is_open()) {
                throw GraphException("CsvIO::ReadRoads发生错误:roads.csv打开失败，请检查路径格式，当前文件路径:" + path);
                return roads;
            }

            std::string line;
            bool is_first_line = true;
            while (std::getline(file, line)) {
                // 去除首尾空白
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), line.end());

                // 跳过空行
                if (line.empty()) continue;

                // 处理表头：首行若以 "from_id" 开头（忽略大小写）则跳过
                if (is_first_line) {
                    is_first_line = false;
                    std::string lower_line = line;
                    std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
                    // 检查是否以 "from_id" 开头（逗号前）
                    if (lower_line.find("from_id") == 0) {
                        continue;
                    }
                }

                // 将逗号替换为空格
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream iss(line);
                std::string from_id, to_id, status;
                int distance, walk_time;

                // 解析五个字段
                if (!(iss >> from_id >> to_id >> distance >> walk_time >> status)) {
                    throw GraphException("CsvIO::ReadRoads发生错误:roads.csv字段错误，请检查文件格式，当前文件路径" + path);
                    continue;
                }

                roads.push_back({from_id, to_id, distance, walk_time, status});
            }

            return roads;
        }

        bool WritePlaces(const std::string &path, const LGraph &graph) {
            std::ofstream out(path);
            if (!out.is_open()) {
                throw GraphException("CsvIO::WritePlaces:places_out.csv打开失败，请检查路径格式，当前文件路径:" + path);
                return false;
            }
            // 写入表头
            out << "place_id,display_name,category,stay_time,open_time,close_time\n";
            // 获取所有地点 ID
            std::vector<std::string> ids = graph.AllPlaceIds();
            for (const auto &id : ids) {
                LocationInfo loc = graph.GetVertex(id);   // 获取地点完整信息
                out << loc.place_id << ','
                    << loc.display_name << ','
                    << loc.category << ','
                    << loc.stay_time << ','
                    << loc.open_time << ','
                    << loc.close_time << '\n';
            }
            out.close();
            return true;
        }

        bool WriteRoads(const std::string &path, const LGraph &graph) {
            std::ofstream out(path);
            if (!out.is_open()) {
                throw GraphException("CsvIO::WriteRoads:roads_out.csv打开失败，请检查路径格式，当前文件路径:" + path);
                return false;
            }
            // 写入表头
            out << "from_id,to_id,distance,walk_time,status\n";
            // 获取所有边（包括 closed 边），参数 false 表示包含 closed 状态
            std::vector<EdgeNode> edges = graph.AllEdges();
            for (const auto &edge : edges) {
                out << edge.from_id << ','
                    << edge.to_id << ','
                    << edge.distance << ','
                    << edge.walk_time << ','
                    << edge.status << '\n';
            }
            out.close();
            return true;
        }
    }
}
            // TODO: 从 CSV 读取地点信息
            // 提示：
            //   1. 打开文件，若失败则输出错误信息并返回空 vector
            //   2. 逐行读取，将逗号替换为空格后使用 istringstream 解析
            //   3. 若首行以 "place_id" 开头，则跳过（表头行）
            //   4. 跳过空行
            //   5. 每行解析为：place_id, display_name, category, stay_time, open_time, close_time
            //   6. 将解析结果加入 vector 并返回
                    // TODO: 从 CSV 读取道路信息
            // 提示：
            //   1. 打开文件，若失败则输出错误信息并返回空 vector
            //   2. 逐行读取，将逗号替换为空格后使用 istringstream 解析
            //   3. 若首行以 "from_id" 开头，则跳过（表头行）
            //   4. 跳过空行
            //   5. 每行解析为：from_id, to_id, distance, walk_time, status
            //   6. 将解析结果加入 vector 并返回        void WritePlaces(const std::string &path, const LGraph &graph) {

            // TODO: 将地点信息写回 CSV
            // 提示：
            //   1. 打开文件输出流
            //   2. 写入表头行：place_id,display_name,category,stay_time,open_time,close_time
            //   3. 通过 graph.AllPlaceIds() 获取所有地点 id，逐个调用 graph.GetVertex(id) 取完整信息
            //   4. 每行输出一个地点的完整信息，字段间用逗号分隔
            //   5. 关闭文件

            // TODO: 将道路信息写回 CSV
            // 提示：
            //   1. 打开文件输出流
            //   2. 写入表头行：from_id,to_id,distance,walk_time,status
            //   3. 使用 graph.AllEdges(false) 获取所有边（包括 closed）
            //   4. 输出每条边的 from_id, to_id, distance, walk_time, status
            //   5. 关闭文件

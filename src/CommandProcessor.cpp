//
// 动态图上的校园路线规划与设施分析系统
// CommandProcessor.cpp - 命令解析与分发实现
//

#include "../include/CommandProcessor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <cctype>

namespace Graph {

    bool CommandProcessor::ProcessCommand(const std::string &line) {
        // graph 已被各 cmd* 处理函数使用。

        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) {
            return true;  // 空行，跳过
        }
        if (cmd == "QUIT") {
            return false;
        } else if (cmd == "LOAD") {
            cmdLoad(iss);
        } else if (cmd == "SAVE") {
            cmdSave(iss);
        } else if (cmd == "QUERY_PLACE") {
            cmdQueryPlace(iss);
        } else if (cmd == "QUERY_CATEGORY") {
            cmdQueryCategory(iss);
        } else if (cmd == "ADJ") {
            cmdAdj(iss);
        } else if (cmd == "ADD_PLACE") {
            cmdAddPlace(iss);
        } else if (cmd == "DELETE_PLACE") {
            cmdDeletePlace(iss);
        } else if (cmd == "UPDATE_PLACE") {
            cmdUpdatePlace(iss);
        } else if (cmd == "ADD_ROAD") {
            cmdAddRoad(iss);
        } else if (cmd == "DELETE_ROAD") {
            cmdDeleteRoad(iss);
        } else if (cmd == "UPDATE_ROAD") {
            cmdUpdateRoad(iss);
        } else if (cmd == "CLOSE_ROAD") {
            cmdCloseRoad(iss);
        } else if (cmd == "OPEN_ROAD") {
            cmdOpenRoad(iss);
        } else if (cmd == "COMPONENTS") {
            cmdComponents();
        } else if (cmd == "SHORTEST") {
            cmdShortest(iss);
        } else if (cmd == "TIMED_SHORTEST") {
            cmdTimedShortest(iss);
        } else if (cmd == "MUST_PASS") {
            cmdMustPass(iss);
        } else if (cmd == "SHORTEST_K") {
            cmdShortestK(iss);
        } else if (cmd == "MST") {
            cmdMst();
        } else if (cmd == "CRITICAL") {
            cmdCritical();
        } else if (cmd == "CRITICAL_TARJAN") {
            cmdCriticalTarjan();
        } else {
            std::cout << "ERROR unknown_command" << std::endl;
        }

        return true;
    }

    // ==================== LOAD ====================
        // TODO: 实现 LOAD <places.csv> <roads.csv>
        // 提示：
        //   1. 从 args 读取两个文件路径
        //   2. 用 CsvIO::ReadPlaces 和 CsvIO::ReadRoads 读取数据
        //   3. 重建图（清空旧数据，插入新顶点和边）
        //   4. 成功输出 "OK"，失败输出 "ERROR <reason>"
    void CommandProcessor::cmdLoad(std::istringstream &args) {
        //解析主函数参数
        std::string places_csv_path, roads_csv_path;
        if (!(args>>places_csv_path>>roads_csv_path)) {
            std::cerr<<"CommandProcessor::cmdLoad:路径参数格式错误"<<std::endl;
            return;
        }

        //读取CSV数据
        std::vector<LocationInfo> places_vec;
        std::vector<RoadRecord> roads_vec;
        try {
            places_vec = CsvIO::ReadPlaces(places_csv_path);
            roads_vec = CsvIO::ReadRoads(roads_csv_path);
        } catch (const std::exception &e) {
            std::cerr<<"CommandProcessor::cmdLoad:"<<e.what()<<std::endl;
            return;
        }

        //初始化临时图
        LGraph new_graph;

        //创建平凡图
        for (const auto &loc : places_vec) {
            try {
                new_graph.InsertVertex(loc);
            } catch (const GraphException &e) {
                std::cerr << "CommandProcessor::cmdLoad:" << e.what() << std::endl;
                return;
            }
        }

        //插入边
        for (const auto &r : roads_vec) {
            try {
                new_graph.InsertEdge(r.from_id, r.to_id, r.distance, r.walk_time, r.status);
            } catch (const GraphException &e) {
                std::cerr << "CommandProcessor::cmdLoad:" << e.what() << std::endl;
                return;
            }
        }

        //移动赋值高效交换所有权
        graph = std::move(new_graph);

        std::cout<<"OK"<<std::endl;
        return;      
    }

    // ==================== SAVE ====================
        // TODO: 实现 SAVE <places_out.csv> <roads_out.csv>
        // 提示：
        //   1. 从 args 读取两个输出文件路径
        //   2. 调用 CsvIO::WritePlaces 和 CsvIO::WriteRoads
        //   3. 成功输出 "OK"
    void CommandProcessor::cmdSave(std::istringstream &args) {
        //解析主函数参数
        std::string places_out_csv_path, roads_out_csv_path;
        if (!(args>>places_out_csv_path>>roads_out_csv_path)) {
            std::cerr<<"CommandProcessor::cmdSave:路径参数格式错误"<<std::endl;
            return;
        }

        //写入CSV数据
        try {
            CsvIO::WritePlaces(places_out_csv_path, graph);
            CsvIO::WriteRoads(roads_out_csv_path, graph);
        }
        catch (const GraphException &e){
            std::cerr<<e.what()<<std::endl;
            return;
        }

        std::cout<<"OK"<<std::endl;
        return;      
    }

    // ==================== QUERY_PLACE ====================
        // TODO: 实现 QUERY_PLACE <place_id>
        // 输出格式：PLACE <place_id> <display_name> <category> <stay_time> <open_time> <close_time>
        // 若不存在：ERROR place_not_found
    void CommandProcessor::cmdQueryPlace(std::istringstream &args) {
        std::string place_id;
        if (!(args >> place_id)) {
            std::cerr << "CommandProcessor::cmdQueryPlace:地点参数解析失败" << std::endl;
            return;
        }

        LocationInfo loc;
        try {
            loc = graph.GetVertex(place_id);
        } catch (const GraphException &e) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        std::cout << "PLACE"
                  << " " << loc.place_id
                  << " " << loc.display_name
                  << " " << loc.category
                  << " " << loc.stay_time
                  << " " << loc.open_time
                  << " " << loc.close_time
                  << std::endl;
    }

    
    // ==================== QUERY_CATEGORY ====================
        // TODO: 实现 QUERY_CATEGORY <category>
        // 输出格式：CATEGORY <category> <count> <id1> <id2> ...
        // place_id 按字典序升序输出
        // 若无匹配：CATEGORY <category> 0
    void CommandProcessor::cmdQueryCategory(std::istringstream &args) {
        std::string category;
        if (!(args >> category)) {
            std::cerr << "CommandProcessor::cmdQueryCategory:参数格式错误" << std::endl;
            return;
        }

        std::vector<std::string> place_vec;
        try {
            place_vec = graph.GetPlacesByCategory(category);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdQueryCategory:" << e.what() << std::endl;
            return;
        }

        std::sort(place_vec.begin(), place_vec.end(), LGraph::Compare::GreaterLexPlaces);

        std::cout << "CATEGORY " << category << " " << place_vec.size();
        for (const std::string &id : place_vec) {
            std::cout << " " << id;
        }
        std::cout << std::endl;
        return;
    }

    // ==================== ADJ ====================
        // TODO: 实现 ADJ <place_id>
        // 输出格式：ADJ <place_id> <count> <neighbor1>:<distance>:<walk_time>:<status> ...
        // 邻接点按 place_id 字典序升序输出
        // 若顶点不存在：ERROR place_not_found
    void CommandProcessor::cmdAdj(std::istringstream &args) {
        std::string place_id;
        if (!(args >> place_id)) {
            std::cerr << "CommandProcessor::cmdAdj:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(place_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        std::vector<EdgeNode> edges;
        try {
            edges = graph.GetAdjacentEdges(place_id, false);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdAdj:" << e.what() << std::endl;
            return;
        }

        struct NeighborInfo {
            std::string neighbor_id;
            int distance;
            int walk_time;
            std::string status;
        };
        std::vector<NeighborInfo> neighbors;
        for (const auto &e : edges) {
            std::string neighbor = (e.from_id == place_id) ? e.to_id : e.from_id;
            neighbors.push_back({neighbor, e.distance, e.walk_time, e.status});
        }

        std::sort(neighbors.begin(), neighbors.end(),
                  [](const NeighborInfo &a, const NeighborInfo &b) {
                      return a.neighbor_id < b.neighbor_id;
                  });

        std::cout << "ADJ " << place_id << " " << neighbors.size();
        for (const auto &n : neighbors) {
            std::cout << " " << n.neighbor_id << ":" << n.distance
                      << ":" << n.walk_time << ":" << n.status;
        }
        std::cout << std::endl;
        return;
    }

    // ==================== ADD_PLACE ====================
        // TODO: 实现 ADD_PLACE <place_id> <display_name> <category> <stay_time> <open_time> <close_time>
        // 成功输出 "OK"
        // 若 place_id 已存在：ERROR place_already_exists
    void CommandProcessor::cmdAddPlace(std::istringstream &args) {
        std::string place_id, display_name, category, open_time, close_time;
        int stay_time;
        if (!(args >> place_id >> display_name >> category >> stay_time >> open_time >> close_time)) {
            std::cerr << "CommandProcessor::cmdAddPlace:参数格式错误" << std::endl;
            return;
        }

        if (graph.ExistVertex(place_id)) {
            std::cout << "ERROR place_already_exists" << std::endl;
            return;
        }

        LocationInfo loc(place_id, display_name, category, stay_time, open_time, close_time);
        try {
            graph.InsertVertex(loc);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdAddPlace:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== DELETE_PLACE ====================
        // TODO: 实现 DELETE_PLACE <place_id>
        // 成功输出 "OK"
        // 若不存在：ERROR place_not_found
    void CommandProcessor::cmdDeletePlace(std::istringstream &args) {
        std::string place_id;
        if (!(args >> place_id)) {
            std::cerr << "CommandProcessor::cmdDeletePlace:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(place_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        try {
            graph.DeleteVertex(place_id);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdDeletePlace:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== UPDATE_PLACE ====================
        // TODO: 实现 UPDATE_PLACE <place_id> <field> <value>
        // 成功输出 "OK"
        // 支持的字段：display_name, category, stay_time, open_time, close_time
        // 若顶点不存在：ERROR place_not_found
        // 若字段不支持：ERROR invalid_field
    void CommandProcessor::cmdUpdatePlace(std::istringstream &args) {
        std::string place_id, field, value;
        if (!(args >> place_id >> field >> value)) {
            std::cerr << "CommandProcessor::cmdUpdatePlace:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(place_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        if (field != "display_name" && field != "category" &&
            field != "stay_time" && field != "open_time" && field != "close_time") {
            std::cout << "ERROR invalid_field" << std::endl;
            return;
        }

        try {
            graph.UpdateVertex(place_id, field, value);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdUpdatePlace:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== ADD_ROAD ====================
        // TODO: 实现 ADD_ROAD <from_id> <to_id> <distance> <walk_time> <status>
        // 成功输出 "OK"
        // 若顶点不存在：ERROR place_not_found
        // 若边已存在：ERROR road_already_exists
    void CommandProcessor::cmdAddRoad(std::istringstream &args) {
        std::string from_id, to_id, status;
        int distance, walk_time;
        if (!(args >> from_id >> to_id >> distance >> walk_time >> status)) {
            std::cerr << "CommandProcessor::cmdAddRoad:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        if (graph.ExistEdge(from_id, to_id)) {
            std::cout << "ERROR road_already_exists" << std::endl;
            return;
        }

        try {
            graph.InsertEdge(from_id, to_id, distance, walk_time, status);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdAddRoad:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== DELETE_ROAD ====================
        // TODO: 实现 DELETE_ROAD <from_id> <to_id>
        // 成功输出 "OK"
        // 若边不存在：ERROR road_not_found
    void CommandProcessor::cmdDeleteRoad(std::istringstream &args) {
        std::string from_id, to_id;
        if (!(args >> from_id >> to_id)) {
            std::cerr << "CommandProcessor::cmdDeleteRoad:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistEdge(from_id, to_id)) {
            std::cout << "ERROR road_not_found" << std::endl;
            return;
        }

        try {
            graph.DeleteEdge(from_id, to_id);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdDeleteRoad:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== UPDATE_ROAD ====================
        // TODO: 实现 UPDATE_ROAD <from_id> <to_id> <field> <value>
        // 成功输出 "OK"
        // 支持的字段：distance, walk_time, status
        // 若边不存在：ERROR road_not_found
        // 若字段不支持：ERROR invalid_field
    void CommandProcessor::cmdUpdateRoad(std::istringstream &args) {
        std::string from_id, to_id, field, value;
        if (!(args >> from_id >> to_id >> field >> value)) {
            std::cerr << "CommandProcessor::cmdUpdateRoad:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistEdge(from_id, to_id)) {
            std::cout << "ERROR road_not_found" << std::endl;
            return;
        }

        if (field != "distance" && field != "walk_time" && field != "status") {
            std::cout << "ERROR invalid_field" << std::endl;
            return;
        }

        try {
            graph.UpdateEdge(from_id, to_id, field, value);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdUpdateRoad:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== CLOSE_ROAD ====================
        // TODO: 实现 CLOSE_ROAD <from_id> <to_id>
        // 成功输出 "OK"
        // 若边不存在：ERROR road_not_found
    void CommandProcessor::cmdCloseRoad(std::istringstream &args) {
        std::string from_id, to_id;
        if (!(args >> from_id >> to_id)) {
            std::cerr << "CommandProcessor::cmdCloseRoad:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistEdge(from_id, to_id)) {
            std::cout << "ERROR road_not_found" << std::endl;
            return;
        }

        try {
            graph.CloseRoad(from_id, to_id);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdCloseRoad:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== OPEN_ROAD ====================
        // TODO: 实现 OPEN_ROAD <from_id> <to_id>
        // 成功输出 "OK"
        // 若边不存在：ERROR road_not_found
    void CommandProcessor::cmdOpenRoad(std::istringstream &args) {
        std::string from_id, to_id;
        if (!(args >> from_id >> to_id)) {
            std::cerr << "CommandProcessor::cmdOpenRoad:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistEdge(from_id, to_id)) {
            std::cout << "ERROR road_not_found" << std::endl;
            return;
        }

        try {
            graph.OpenRoad(from_id, to_id);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdOpenRoad:" << e.what() << std::endl;
            return;
        }

        std::cout << "OK" << std::endl;
    }

    // ==================== COMPONENTS ====================
        // TODO: 实现 COMPONENTS
        // 输出格式：COMPONENTS <count> SIZES <s1> <s2> ...
        // 规模按降序输出
    void CommandProcessor::cmdComponents() {
        Algorithm::ComponentsResult result;
        try {
            result = Algorithm::GetConnectedComponents(graph);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdComponents:" << e.what() << std::endl;
            return;
        }

        std::cout << "COMPONENTS " << result.count << " SIZES";
        for (int size : result.sizes) {
            std::cout << " " << size;
        }
        std::cout << std::endl;
    }

    // ==================== SHORTEST ====================
        // TODO: 实现 SHORTEST <from_id> <to_id> <DIST|TIME>
        // 成功输出格式：PATH <DIST|TIME> <total_cost> NODES <id1> <id2> ...
        // 不可达输出：NO_PATH
        // 若顶点不存在：ERROR place_not_found
    void CommandProcessor::cmdShortest(std::istringstream &args) {
        std::string from_id, to_id, mode_str;
        if (!(args >> from_id >> to_id >> mode_str)) {
            std::cerr << "CommandProcessor::cmdShortest:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        Algorithm::PathMode mode;
        if (mode_str == "DIST") {
            mode = Algorithm::DIST;
        } else if (mode_str == "TIME") {
            mode = Algorithm::TIME;
        } else {
            std::cerr << "CommandProcessor::cmdShortest:未知模式 " << mode_str << std::endl;
            return;
        }

        Algorithm::PathResult result;
        try {
            result = Algorithm::GetShortestPath(graph, from_id, to_id, mode);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdShortest:" << e.what() << std::endl;
            return;
        }

        if (!result.reachable) {
            std::cout << "NO_PATH" << std::endl;
            return;
        }

        std::cout << "PATH " << mode_str << " " << result.total_cost << " NODES";
        for (const std::string &id : result.path) {
            std::cout << " " << id;
        }
        std::cout << std::endl;
    }

    // ==================== TIMED_SHORTEST ====================
        // TODO: 实现 TIMED_SHORTEST <from_id> <to_id> <time> <DIST|TIME>
        // 含义：在给定时刻（HH:MM）下，找 from_id 到 to_id 的最短路径
        //   - 只走 status == "open" 的道路
        //   - 路径上（含起点、终点、中间点）所有地点必须在 time 时开放
        //     （即 open_time <= time <= close_time，字符串比较即可）
        // 成功输出格式：PATH <DIST|TIME> <total_cost> NODES <id1> <id2> ...
        // 不可达输出：NO_PATH
        // 若顶点不存在：ERROR place_not_found
    void CommandProcessor::cmdTimedShortest(std::istringstream &args) {
        std::string from_id, to_id, time_str, mode_str;
        if (!(args >> from_id >> to_id >> time_str >> mode_str)) {
            std::cerr << "CommandProcessor::cmdTimedShortest:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        Algorithm::PathMode mode;
        if (mode_str == "DIST") {
            mode = Algorithm::DIST;
        } else if (mode_str == "TIME") {
            mode = Algorithm::TIME;
        } else {
            std::cerr << "CommandProcessor::cmdTimedShortest:未知模式 " << mode_str << std::endl;
            return;
        }

        Algorithm::PathResult result;
        try {
            result = Algorithm::GetTimedShortestPath(graph, from_id, to_id, time_str, mode);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdTimedShortest:" << e.what() << std::endl;
            return;
        }

        if (!result.reachable) {
            std::cout << "NO_PATH" << std::endl;
            return;
        }

        std::cout << "PATH " << mode_str << " " << result.total_cost << " NODES";
        for (const std::string &id : result.path) {
            std::cout << " " << id;
        }
        std::cout << std::endl;
    }

    // ==================== MUST_PASS ====================
        // TODO: 实现 MUST_PASS <from_id> <to_id> <DIST|TIME> <k> <p1> <p2> ... <pk>
        // 成功输出格式：PATH <DIST|TIME> <total_cost> NODES <id1> <id2> ...
        // 不可达输出：NO_PATH
        // 若顶点不存在：ERROR place_not_found
    void CommandProcessor::cmdMustPass(std::istringstream &args) {
        std::string from_id, to_id, mode_str;
        int k;
        if (!(args >> from_id >> to_id >> mode_str >> k)) {
            std::cerr << "CommandProcessor::cmdMustPass:参数格式错误" << std::endl;
            return;
        }

        std::vector<std::string> waypoints;
        for (int i = 0; i < k; ++i) {
            std::string wp;
            if (!(args >> wp)) {
                std::cerr << "CommandProcessor::cmdMustPass:必经点数量不足" << std::endl;
                return;
            }
            waypoints.push_back(wp);
        }

        if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }
        for (const std::string &wp : waypoints) {
            if (!graph.ExistVertex(wp)) {
                std::cout << "ERROR place_not_found" << std::endl;
                return;
            }
        }

        Algorithm::PathMode mode;
        if (mode_str == "DIST") {
            mode = Algorithm::DIST;
        } else if (mode_str == "TIME") {
            mode = Algorithm::TIME;
        } else {
            std::cerr << "CommandProcessor::cmdMustPass:未知模式 " << mode_str << std::endl;
            return;
        }

        Algorithm::PathResult result;
        try {
            result = Algorithm::GetMustPassPath(graph, from_id, to_id, mode, waypoints);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdMustPass:" << e.what() << std::endl;
            return;
        }

        if (!result.reachable) {
            std::cout << "NO_PATH" << std::endl;
            return;
        }

        std::cout << "PATH " << mode_str << " " << result.total_cost << " NODES";
        for (const std::string &id : result.path) {
            std::cout << " " << id;
        }
        std::cout << std::endl;
    }

    // ==================== SHORTEST_K ====================
        // TODO: 实现 SHORTEST_K <from_id> <to_id> <K>（拓展任务 1：共享单车）
        // 成功输出格式：PATH <total_time> K_USED <k_used> NODES <id1> <id2> ... FAST <count> [<u1>-<v1> <u2>-<v2> ...]
        // 不可达输出：NO_PATH
        // 若顶点不存在：ERROR place_not_found
        // 提示：调用 Algorithm::GetShortestKPath(graph, from_id, to_id, K)
    void CommandProcessor::cmdShortestK(std::istringstream &args) {
        std::string from_id, to_id;
        int K;
        if (!(args >> from_id >> to_id >> K)) {
            std::cerr << "CommandProcessor::cmdShortestK:参数格式错误" << std::endl;
            return;
        }

        if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
            std::cout << "ERROR place_not_found" << std::endl;
            return;
        }

        Algorithm::KPathResult result;
        try {
            result = Algorithm::GetShortestKPath(graph, from_id, to_id, K);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdShortestK:" << e.what() << std::endl;
            return;
        }

        if (!result.reachable) {
            std::cout << "NO_PATH" << std::endl;
            return;
        }

        std::cout << "PATH " << result.total_time
                  << " K_USED " << result.k_used
                  << " NODES";
        for (const std::string &id : result.path) {
            std::cout << " " << id;
        }
        std::cout << " FAST " << result.fast_edges.size();
        // 按 (min(u,v), max(u,v)) 字典序排序
        std::sort(result.fast_edges.begin(), result.fast_edges.end(),
                  [](const auto &a, const auto &b) {
                      std::string a_min = std::min(a.first, a.second);
                      std::string a_max = std::max(a.first, a.second);
                      std::string b_min = std::min(b.first, b.second);
                      std::string b_max = std::max(b.first, b.second);
                      if (a_min != b_min) return a_min < b_min;
                      return a_max < b_max;
                  });
        for (const auto &e : result.fast_edges) {
            std::string u = std::min(e.first, e.second);
            std::string v = std::max(e.first, e.second);
            std::cout << " " << u << "-" << v;
        }
        std::cout << std::endl;
    }

    // ==================== MST ====================
        // TODO: 实现 MST
        // 成功输出格式：MST <total_distance> EDGES <u1>-<v1>:<w1> <u2>-<v2>:<w2> ...
        // 边按 (min(u,v), max(u,v)) 字典序排序输出
        // 图不连通输出：DISCONNECTED
    void CommandProcessor::cmdMst() {
        std::vector<EdgeNode> mst_edges;
        try {
            mst_edges = Algorithm::MinimumSpanningTree(graph);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdMst:" << e.what() << std::endl;
            return;
        }

        if (mst_edges.empty()) {
            std::cout << "DISCONNECTED" << std::endl;
            return;
        }

        // 排序：按 (min(u,v), max(u,v)) 字典序
        std::sort(mst_edges.begin(), mst_edges.end(),
                  [](const EdgeNode &a, const EdgeNode &b) {
                      std::string a_min = std::min(a.from_id, a.to_id);
                      std::string a_max = std::max(a.from_id, a.to_id);
                      std::string b_min = std::min(b.from_id, b.to_id);
                      std::string b_max = std::max(b.from_id, b.to_id);
                      if (a_min != b_min) return a_min < b_min;
                      return a_max < b_max;
                  });

        int total_distance = 0;
        for (const EdgeNode &e : mst_edges) {
            total_distance += e.distance;
        }

        std::cout << "MST " << total_distance << " EDGES";
        for (const EdgeNode &e : mst_edges) {
            std::string u = std::min(e.from_id, e.to_id);
            std::string v = std::max(e.from_id, e.to_id);
            std::cout << " " << u << "-" << v << ":" << e.distance;
        }
        std::cout << std::endl;
    }

    // ==================== CRITICAL ====================
        // TODO: 实现 CRITICAL —— 输出关键节点与关键边
        // 输出格式：CRITICAL NODES <count> <id1> <id2> ... EDGES <count> <u1>-<v1> <u2>-<v2> ...
        //   - 关键节点（删去后连通分量数增加的顶点）按 place_id 字典序输出
        //   - 关键边（删去后连通分量数增加的边）按 (min(u,v), max(u,v)) 字典序输出
        // 实现可调用 Algorithm::FindCriticalNodesAndEdges(graph)
    void CommandProcessor::cmdCritical() {
        Algorithm::CriticalResult result;
        try {
            result = Algorithm::FindCriticalNodesAndEdges(graph);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdCritical:" << e.what() << std::endl;
            return;
        }

        std::sort(result.critical_nodes.begin(), result.critical_nodes.end());

        std::sort(result.critical_edges.begin(), result.critical_edges.end(),
                  [](const std::pair<std::string, std::string> &a,
                     const std::pair<std::string, std::string> &b) {
                      std::string a_min = std::min(a.first, a.second);
                      std::string a_max = std::max(a.first, a.second);
                      std::string b_min = std::min(b.first, b.second);
                      std::string b_max = std::max(b.first, b.second);
                      if (a_min != b_min) return a_min < b_min;
                      return a_max < b_max;
                  });

        std::cout << "CRITICAL"
                  << " NODES " << result.critical_nodes.size();
        for (const std::string &id : result.critical_nodes) {
            std::cout << " " << id;
        }
        std::cout << " EDGES " << result.critical_edges.size();
        for (const auto &e : result.critical_edges) {
            std::string u = std::min(e.first, e.second);
            std::string v = std::max(e.first, e.second);
            std::cout << " " << u << "-" << v;
        }
        std::cout << std::endl;
    }

    // ==================== CRITICAL_TARJAN（加分项）====================
        // 与 CRITICAL 输出格式完全相同，内部使用 Tarjan O(V+E) 算法
    void CommandProcessor::cmdCriticalTarjan() {
        Algorithm::CriticalResult result;
        try {
            result = Algorithm::FindCriticalTarjan(graph);
        } catch (const GraphException &e) {
            std::cerr << "CommandProcessor::cmdCriticalTarjan:" << e.what() << std::endl;
            return;
        }

        std::sort(result.critical_nodes.begin(), result.critical_nodes.end());

        std::sort(result.critical_edges.begin(), result.critical_edges.end(),
                  [](const std::pair<std::string, std::string> &a,
                     const std::pair<std::string, std::string> &b) {
                      std::string a_min = std::min(a.first, a.second);
                      std::string a_max = std::max(a.first, a.second);
                      std::string b_min = std::min(b.first, b.second);
                      std::string b_max = std::max(b.first, b.second);
                      if (a_min != b_min) return a_min < b_min;
                      return a_max < b_max;
                  });

        std::cout << "CRITICAL"
                  << " NODES " << result.critical_nodes.size();
        for (const std::string &id : result.critical_nodes) {
            std::cout << " " << id;
        }
        std::cout << " EDGES " << result.critical_edges.size();
        for (const auto &e : result.critical_edges) {
            std::string u = std::min(e.first, e.second);
            std::string v = std::max(e.first, e.second);
            std::cout << " " << u << "-" << v;
        }
        std::cout << std::endl;
    }

}

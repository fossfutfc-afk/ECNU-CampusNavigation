//
// 动态图上的校园路线规划与设施分析系统
// LGraph.cpp - 图 ADT 实现（内部存储结构由你自行设计）
//

#include "../include/LGraph.h"
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstdlib>

namespace Graph {
    /*-------------------------------------构造函数部分-----------------------------------------*/
    LGraph::LGraph(bool directed) : directed(directed),
                                    edge_count(0){}
    // 其余构造与析构均由编译器自动生成


    /*--------------------------------------常规方法--------------------------------------------*/

    int LGraph::VertexCount() const
    {
        return static_cast<int>(vertices.size());// size_t(ull)->int 参考AI意见,防止数据溢出,即使该规模下不会发生
    }

    int LGraph::EdgesCount() const {
        return edge_count;
    }

    bool LGraph::ExistVertex(const std::string &place_id) const {
        return (vertices.find(place_id) != vertices.end());
    }

    bool LGraph::ExistEdge(const std::string &from_id, const std::string &to_id) const {
        auto it_from = adj.find(from_id);
        if (it_from == adj.end()) return false;
        return it_from->second.find(to_id) != it_from->second.end();
    }

    bool LGraph::ExistTimeVertex(const std::string &place_id, const std::string &time) const {
        if (ExistVertex(place_id) && (TimeValidator(vertices.find(place_id)->second.open_time) > TimeValidator(time) || //由短路性,不会发生对尾迭代器的错误解析
            TimeValidator(vertices.find(place_id)->second.close_time) < TimeValidator(time))) {
            return false;
        }
        return true;
    }


    /*--------------------------------------顶点操作--------------------------------------------*/

    void LGraph::InsertVertex(const LocationInfo &vertex_info) {
        const std::string &id = vertex_info.place_id;
        if (ExistVertex(id)) {
            throw GraphException("LGraph::InsertVertex:顶点 <" + id + "> 已存在");
        }
        vertex_info.ValidCheck();
        // 存入顶点表
        vertices[id] = vertex_info;
        // 维护类别索引
        category_index[vertex_info.category].push_back(id);
    }

    void LGraph::DeleteVertex(const std::string &place_id) {
        auto it_vertex = vertices.find(place_id);
        if (it_vertex == vertices.end()) {
            throw GraphException("LGraph::DeleteVertex:顶点 <" + place_id + "> 不存在");
        }

        // 从类别索引中移除
        const std::string &cat = it_vertex->second.category;
        auto &vec = category_index[cat];
        vec.erase(std::remove(vec.begin(), vec.end(), place_id), vec.end());
        if (vec.empty()) {
            category_index.erase(cat);
        }

        // 统计要删除的逻辑边（pair 去重，类型层面消除歧义）
        using EdgeKey = std::pair<std::string, std::string>;
        struct EdgeKeyHash {
            size_t operator()(const EdgeKey &p) const {
                size_t h1 = std::hash<std::string>{}(p.first);
                size_t h2 = std::hash<std::string>{}(p.second);
                return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
            }
        };
        std::unordered_set<EdgeKey, EdgeKeyHash> logic_edges;

        auto it_adj = adj.find(place_id);
        if (it_adj != adj.end()) {
            for (const auto &pair : it_adj->second) {
                const std::string &nb = pair.first;
                if (directed) {
                    logic_edges.insert({place_id, nb});
                } else {
                    if (place_id < nb)
                        logic_edges.insert({place_id, nb});
                    else
                        logic_edges.insert({nb, place_id});
                }
            }
        }

        if (directed) {
            for (auto &outer : adj) {
                const std::string &src = outer.first;
                if (src == place_id) continue;
                if (outer.second.find(place_id) != outer.second.end()) {
                    logic_edges.insert({src, place_id});
                }
            }
        }

        // 先扣除逻辑边数
        edge_count -= static_cast<int>(logic_edges.size());

        // 删除自己的邻接表
        adj.erase(place_id);
        // 删除其他顶点中指向place_id的边（无向图也要清反向）
        for (auto &outer : adj) {
            outer.second.erase(place_id);
        }
        // 清理内层map为空的外层key
        for (auto it = adj.begin(); it != adj.end(); ) {
            if (it->second.empty()) {
                it = adj.erase(it);
            } else {
                ++it;
            }
        }

        // 删除顶点
        vertices.erase(it_vertex);
    }

    void LGraph::UpdateVertex(const std::string &place_id, 
                              const std::string &field, 
                              const std::string &value) {
        auto it = vertices.find(place_id);
        if (it == vertices.end()) {
            throw GraphException("LGraph::UpdateVertex:顶点 <" + place_id + "> 未找到");
        }

        LocationInfo& loc = it->second;

        if (field == "display_name") {
            loc.display_name = value;
        } else if (field == "category") {
            // 从旧类别索引移除
            std::string old_cat = loc.category;
            auto &old_vec = category_index[old_cat];
            old_vec.erase(std::remove(old_vec.begin(), old_vec.end(), place_id), old_vec.end());
            if (old_vec.empty()) {
                category_index.erase(old_cat);
            }
            // 更新类别
            loc.category = value;
            // 加入新类别索引
            category_index[value].push_back(place_id);
        } else if (field == "stay_time") {
            loc.stay_time = std::stoi(value);
        } else if (field == "open_time") {
            loc.open_time = TimeValidator(value);
        } else if (field == "close_time") {
            loc.close_time = TimeValidator(value);
        } else {
            throw GraphException("LGraph::UpdateVertex:未知顶点参量 <" + field + ">");
        }
        it->second.ValidCheck();
        return;
    }

    LocationInfo LGraph::GetVertex(const std::string &place_id) const {
        auto it = vertices.find(place_id);
        if (it == vertices.end()) {
            throw GraphException("LGraph::GetVertex:顶点 <" + place_id + "> 未找到");
        }
        return it->second;
    }

    /*---------------------------------------边操作--------------------------------------------*/

    void LGraph::InsertEdge(const std::string &from_id, const std::string &to_id,
                            int distance, int walk_time, const std::string &status) {
        if (!ExistVertex(from_id)) {
            throw GraphException("LGraph::InsertEdge:起点 <" + from_id + "> 不存在");
        }
        if (!ExistVertex(to_id)) {
            throw GraphException("LGraph::InsertEdge:终点 <" + to_id + "> 不存在");
        }
        if (ExistEdge(from_id, to_id)) {
            throw GraphException("LGraph::InsertEdge:边 <" + from_id + ">   ---->   <" + to_id + "> 已存在");
        }
        // 用实际数据构造 EdgeNode 后校验，避免 operator[] 就地创建默认对象
        EdgeNode newEdge(from_id, to_id, distance, walk_time, status);
        newEdge.ValidCheck();

        // 插入主方向（move 避免 string 深拷贝）
        adj[from_id][to_id] = std::move(newEdge);
        if (!directed) {
            // 无向图同时插入反向
            adj[to_id][from_id] = EdgeNode(to_id, from_id, distance, walk_time, status);
        }

        // 逻辑边数+1（无向图虽存两条记录,但只计一条边）
        ++edge_count;
    }

    void LGraph::DeleteEdge(const std::string &from_id, const std::string &to_id) {
        if (!ExistEdge(from_id, to_id)) {
            throw GraphException("LGraph::DeleteEdge:边 <" + from_id + ">   ---->   <" + to_id + "> 不存在");
        }

        // 删除主方向
        adj[from_id].erase(to_id);
        if (adj[from_id].empty()) {
            adj.erase(from_id);
        }

        if (!directed) {
            // 删除反向
            adj[to_id].erase(from_id);
            if (adj[to_id].empty()) {
                adj.erase(to_id);
            }
        }

        --edge_count;
    }


void LGraph::UpdateEdge(const std::string &from_id, const std::string &to_id,
                        const std::string &field, const std::string &value) {
    if (!ExistEdge(from_id, to_id)) {
        throw GraphException("LGraph::UpdateEdge:边 <" + from_id + ">   ---->   <" + to_id + "> 不存在");
    }
    adj[from_id][to_id].ValidCheck();

    // 获取主方向边的引用
    EdgeNode &edge = adj[from_id][to_id];

    if (field == "distance") {
        int new_dist = std::stoi(value);
        edge.distance = new_dist;
        if (!directed) adj[to_id][from_id].distance = new_dist;
    } else if (field == "walk_time") {
        int new_time = std::stoi(value);
        edge.walk_time = new_time;
        if (!directed) adj[to_id][from_id].walk_time = new_time;
    } else if (field == "status") {
        if (value != "open" && value != "closed") {
            throw GraphException("LGraph::UpdateEdge:不能有'open'或'close'以外的状态");
        }
        edge.status = value;
        if (!directed) adj[to_id][from_id].status = value;
    } else {
        throw GraphException("LGraph::UpdateEdge:未知边参量 <" + field + ">");
    }
}

    EdgeNode LGraph::GetEdge(const std::string &from_id, const std::string &to_id) const {
        auto it_from = adj.find(from_id);
        if (it_from == adj.end()) {
            throw GraphException("LGraph::GetEdge:起点 <" + from_id + "> 不存在邻接边");
        }
        auto it_to = it_from->second.find(to_id);
        if (it_to == it_from->second.end()) {
            throw GraphException("LGraph::GetEdge:边 <" + from_id + ">   ---->   <" + to_id + "> 不存在");
        }
        return it_to->second;
    }

    /*------------------------------------道路状态变更-----------------------------------------*/

    void LGraph::CloseRoad(const std::string &from_id, const std::string &to_id) {
        UpdateEdge(from_id, to_id, "status", "closed");
    }

    void LGraph::OpenRoad(const std::string &from_id, const std::string &to_id) {
        UpdateEdge(from_id, to_id, "status", "open");
    }

    /*--------------------------------------高级查询-------------------------------------------*/

    std::vector<std::string> LGraph::AllPlaceIds() const {
        std::vector<std::string> ids;
        ids.reserve(vertices.size());
        for (const auto &pair : vertices) {
            ids.emplace_back(pair.first);
        }
        return ids;
    }
    std::vector<EdgeNode> LGraph::AllEdges(bool only_open) const {
        std::vector<EdgeNode> result;
        result.reserve(edge_count);  // 最大不超过逻辑边数
        for (const auto &from_pair : adj) {
            const std::string &from = from_pair.first;
            for (const auto &to_pair : from_pair.second) {
                // 无向图去重：仅保留from < to的边
                if (!directed && from >= to_pair.first) continue;

                const EdgeNode &edge = to_pair.second;
                if (!only_open || edge.status == "open") {
                    result.emplace_back(edge);
                }
            }
        }
        return result;
    }

    std::vector<EdgeNode> LGraph::GetAdjacentEdges(const std::string &place_id, bool only_open) const {
        auto it = adj.find(place_id);
        if (it == adj.end()) return {};

        std::vector<EdgeNode> edges;
        edges.reserve(it->second.size());  // 最多所有邻边
        for (const auto &pair : it->second) {
            if ((!only_open || pair.second.status == "open")) {
                edges.emplace_back(pair.second);
            }
        }
        return edges;
    }

    std::vector<std::string> LGraph::GetPlacesByCategory(const std::string &category) const {
        auto it = category_index.find(category);
        if (it != category_index.end()) {
            return it->second;  // 直接返回，由编译器 RVO/移动语义优化
        }
        return {};
    }



    /*---------------------------------------方向操作--------------------------------------------*/
    bool LGraph::IsDirected() const {
        return directed;
    }

    void LGraph::ReverseDirection()
    {
        if (!directed) {
            // 无向 → 有向：每条逻辑边拆成两条有向边，边数翻倍
            edge_count *= 2;
            directed = true;
            return;
        } else {
            // 有向 → 无向：为每条有向边补上反向边
            for (const auto &v1 : vertices) {
                const std::string &from_id = v1.first;
                auto it = adj.find(from_id);
                if (it == adj.end()) continue;

                // 遍历 from_id 的所有出边，为每条补反向
                for (const auto &v2 : it->second) {
                    const std::string &to_id = v2.first;
                    // 若反向边尚不存在，则创建（from_id / to_id 交换，其余字段不变）
                    if (adj[to_id].find(from_id) == adj[to_id].end()) {
                        EdgeNode rev = v2.second;
                        std::swap(rev.from_id, rev.to_id);
                        adj[to_id][from_id] = rev;
                    }
                }
            }
            // 双向存储后每条逻辑边只计一次
            edge_count /= 2;
            directed = false;
        }
    }


    /*---------------------------------------比较函数--------------------------------------------*/
    bool LGraph::Compare::GreaterLexPlaces(const std::string &a, const std::string &b)
    {
        return a < b;
    }

    bool LGraph::Compare::GreaterLexEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.from_id != b.from_id) {
            return a.from_id < b.from_id;
        } else if (a.to_id != b.to_id){
            return a.to_id < b.to_id;
        } else {
            return a.status > b.status;
        }
    }
    bool LGraph::Compare::GreaterDisEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        } else {
            return a.status > b.status;
        }
    }
    bool LGraph::Compare::GreaterWkTEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.walk_time != b.walk_time) {
            return a.walk_time < b.walk_time;
        } else {
            return a.status > b.status;
        }
    }
    bool LGraph::Compare::LessenLexPlaces(const std::string &a, const std::string &b)
    {
        return a > b;
    }
    bool LGraph::Compare::LessenLexEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.from_id != b.from_id) {
            return a.from_id > b.from_id;
        } else if (a.to_id != b.to_id) {
            return a.to_id > b.to_id;
        } else {
            return a.status > b.status;
        }
    }
    bool LGraph::Compare::LessenDisEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.distance != b.distance) {
            return a.distance > b.distance;
        } else {
            return a.status > b.status;
        }
    }
    bool LGraph::Compare::LessenWkTEdgenode(const EdgeNode &a, const EdgeNode &b)
    {
        if (a.walk_time != b.walk_time) {
            return a.walk_time > b.walk_time;
        } else {
            return a.status > b.status;
        }
    }
}

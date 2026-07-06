//
// 动态图上的校园路线规划与设施分析系统
// LGraph.h - 图 ADT 公共接口
//
// 本头文件只声明图应当支持的公共操作，**不规定内部存储结构**。
// 请在 private 区自行设计你的数据结构，并在报告中说明：
//   - 你为什么选这种结构？
//   - 考虑过哪些替代方案？权衡是什么？
//   - 各个操作（增删改查、遍历、查邻接）的时间/空间复杂度？
//

#ifndef LGRAPH_LGRAPH_H
#define LGRAPH_LGRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "GraphElements.h"
#include "GraphException.h"

namespace Graph {
    class LGraph {
    private:
        using v_map = std::unordered_map<std::string, LocationInfo>;
        using adj_map = std::unordered_map<std::string, 
                        std::unordered_map<std::string, EdgeNode>>;
        using cat_map = std::unordered_map<std::string, std::vector<std::string>>;
        
        bool directed;
        v_map vertices;
        adj_map adj;
        cat_map category_index;
        int edge_count = 0;

    public:
        // ==================== 内存管理 ====================
        explicit LGraph(bool directed = false);
        LGraph& operator=(const LGraph&) = default;
        LGraph(LGraph&&) noexcept = default;
        LGraph& operator=(LGraph&&) noexcept = default;
        ~LGraph() = default;

        // ==================== 基础操作 ====================
        int VertexCount() const;
        int EdgesCount() const;
        bool ExistVertex(const std::string &place_id) const;
        bool ExistEdge(const std::string &from_id, const std::string &to_id) const;
        bool ExistTimeVertex(const std::string &place_id, const std::string &time) const;// 判断一个顶点在某时间是否开启
        

        // ==================== 顶点操作 ====================
        void InsertVertex(const LocationInfo &vertex_info);
        void DeleteVertex(const std::string &place_id);
        void UpdateVertex(const std::string &place_id,
                          const std::string &field, const std::string &value);
        LocationInfo GetVertex(const std::string &place_id) const;

        // ==================== 边操作 ====================
        void InsertEdge(const std::string &from_id, const std::string &to_id,
                        int distance, int walk_time, const std::string &status);
        void DeleteEdge(const std::string &from_id, const std::string &to_id);
        void UpdateEdge(const std::string &from_id, const std::string &to_id,
                        const std::string &field, const std::string &value);
        EdgeNode GetEdge(const std::string &from_id, const std::string &to_id) const;

        // ==================== 道路状态 ====================
        void CloseRoad(const std::string &from_id, const std::string &to_id);
        void OpenRoad(const std::string &from_id, const std::string &to_id);

        // ==================== 遍历 / 高级查询 ====================
        // 下面这些方法服务于 Algorithm / CsvIO 等模块，避免它们直接访问你的私有存储。
        std::vector<std::string> AllPlaceIds() const;
            // 返回当前图中所有存在的地点 id（顺序由你决定）

        std::vector<EdgeNode> AllEdges(bool only_open = true) const;
        // 返回当前图中所有边（无向图中每条边只出现一次）
        // only_open = true 时仅返回 status == "open" 的边

        std::vector<EdgeNode> GetAdjacentEdges(const std::string &place_id, bool only_open = true) const;
            // 返回某地点的所有邻接边（详细信息，包括 status）

        std::vector<std::string> GetPlacesByCategory(const std::string &category) const;
            // 返回某类别下的所有 place_id

        // ======================= 方向操作 ========================
        bool IsDirected() const;
        void ReverseDirection();

        // ==================== 内置比较函数 ====================
        class Compare{
        public:
            //用于std::sort的默认内置函数
            static bool GreaterLexPlaces(const std::string &a, const std::string &b);//按照地点名字典升序
            static bool GreaterLexEdgenode(const EdgeNode &a, const  EdgeNode &b);//按照from/to地点名升序,open先于close
            static bool GreaterDisEdgenode(const EdgeNode &a, const  EdgeNode &b);//按照权重升序,open先于close
            static bool GreaterWkTEdgenode(const EdgeNode &a, const  EdgeNode &b);//按照耗时升序,open先于close

            static bool LessenLexPlaces(const std::string &a, const std::string &b);//按照地点名字典降序
            static bool LessenLexEdgenode(const EdgeNode &a, const EdgeNode &b);//按照from/to地点名降序,open先于close
            static bool LessenDisEdgenode(const EdgeNode &a, const  EdgeNode &b);//按照权重降序,open先于close
            static bool LessenWkTEdgenode(const EdgeNode &a, const  EdgeNode &b);//按照耗时降序,open先于close

        };
    };
}
#endif  // LGRAPH_LGRAPH_H

        // ==================================================
        // TODO: 在此处自行设计你的图数据结构
        // ==================================================
        // 可考虑的存储方案（不限于）：
        //   1. 邻接表：vector + list / vector + vector / unordered_map + list ...
        //   2. 邻接矩阵：二维 vector / 一维 vector 模拟二维
        //   3. 边表 + 顶点索引：单一 vector<EdgeNode> 配合 id→索引 的 map
        //   4. 以 place_id 为键的嵌套 hash map（string→string→EdgeNode）
        //   5. 其它你自创的组合
        //
        // 设计时请思考：
        //   - 如何快速通过 place_id 找到顶点？
        //   - 如何支持高效的增删边？
        //   - 删除顶点后，相关资源如何处理（立即回收 / 逻辑删除 / slot 回收）？
        //   - 无向图中同一条边如何避免重复存储，同时又能快速双向查询？
        // ==================================================

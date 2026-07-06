//
// 动态图上的校园路线规划与设施分析系统
// Algorithm.cpp - 图算法实现
//

#include "../include/GraphAlgorithm.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace Graph {
    namespace Algorithm {

        // ==================== A. 连通分量分析 ====================
            // TODO: 计算图中（仅考虑 open 边）的连通分量个数和各分量规模
            // 提示:
            //   1. 使用 BFS 或 DFS 遍历图
            //   2. 只遍历 status == "open" 的边
            //   3. 从 graph.AllPlaceIds() 获取当前存在的顶点
            //   4. 每次从一个未访问的顶点出发,标记所有可达顶点为同一分量
            //   5. 统计每个分量的大小,最后按降序排列
        ComponentsResult GetConnectedComponents(const LGraph &graph) {
            //仅支持无向图,有向图直接抛出异常
            if (graph.IsDirected()) {
                throw GraphException("GraphAlgorithm::GetConnectedComponents:此算法不支持有向图");
                return ComponentsResult{0, {}};
            }

            //获取顶点信息
            std::vector<std::string> vertices = graph.AllPlaceIds();
            if (vertices.empty()) {
                return ComponentsResult{0, {}};
            }

            //维护visited不相交集与分量大小动态数组
            std::unordered_set<std::string> visited;
            std::vector<int> component_sizes;

            //BFS核心
            for (const auto &start : vertices) {
                if (visited.find(start) != visited.end()) continue;

                std::queue<std::string> q;
                q.push(start);
                visited.insert(start);
                int size = 0;

                while (!q.empty()) {
                    std::string cur = q.front();
                    q.pop();
                    ++size;

                    //自行修改后的GetAdjacentEdges函数默认只提供open边
                    std::vector<EdgeNode> neighbors = graph.GetAdjacentEdges(cur);
                    for (const auto &e : neighbors) {
                        const std::string &nb = e.to_id;
                        //走过的顶点加入集合
                        if (visited.find(nb) == visited.end()) {
                            visited.insert(nb);
                            q.push(nb);
                        }
                    }
                }
                component_sizes.push_back(size);
            }

            std::sort(component_sizes.begin(), component_sizes.end(), std::greater<int>());
            return ComponentsResult{static_cast<int>(component_sizes.size()), component_sizes};
        }
        


        // ==================== B. 最短路径 ====================
            // TODO: 使用 Dijkstra 算法计算最短路径
            // 提示:
            //   1. 根据 mode 选择边权:DIST 使用 distance,TIME 使用 walk_time
            //   2. 只走 status == "open" 的边
            //   3. 使用优先队列（小顶堆）优化
            //   4. 维护 prev 数组以回溯完整路径
            //   5. 若不可达,返回 reachable = false 的 PathResult
            //   6. 路径中存储的是 place_id 序列（不是内部索引）
        PathResult GetShortestPath(const LGraph &graph,
                                   const std::string &from_id,
                                   const std::string &to_id,
                                   PathMode mode) {
            // 起点或终点不存在,判断不可达
            if (!graph.ExistVertex(from_id) || !graph.ExistVertex(to_id)) {
                return PathResult{0, {}, false};
            }

            std::unordered_map<std::string, int> dist;
            std::unordered_map<std::string, std::string> prev;
            dist[from_id] = 0;

            // 维护小根堆,降低时间复杂度
            using P = std::pair<int, std::string>;
            std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
            pq.push({0, from_id});

            while (!pq.empty()) {
                int d = pq.top().first;
                std::string u = pq.top().second;
                pq.pop();

                auto it_d = dist.find(u); if (it_d == dist.end() || d != it_d->second) continue;
                if (u == to_id) break;

                auto edges = graph.GetAdjacentEdges(u);
                // Dijkstra核心
                for (const auto &e : edges) {
                    const std::string &v = e.to_id;
                    int weight = (mode == PathMode::TIME) ? e.walk_time : e.distance;

                    if (dist.find(v) == dist.end()) dist[v] = INT_MAX;
                    // dist[u]==INT_MAX 时跳过，避免 INT_MAX+weight 整数溢出回绕
                    if (dist[u] != INT_MAX && dist[u] + weight < dist[v]) {
                        dist[v] = dist[u] + weight;
                        prev[v] = u;
                        pq.push({dist[v], v});
                    }
                }
            }

            // 找不到终点或到终点权重为无穷,判断不可达
            if (dist.find(to_id) == dist.end() || dist[to_id] == INT_MAX) {
                return PathResult{0, {}, false};
            }

            // 回溯路径
            std::vector<std::string> path;
            for (std::string cur = to_id; cur != from_id; cur = prev[cur]) {
                path.push_back(cur);
            }
            path.push_back(from_id);
            std::reverse(path.begin(), path.end());

            return PathResult{dist[to_id], std::move(path), true};
        }

        // ==================== B'. 时刻约束最短路径 ====================
            // TODO: 在给定时刻 time（HH:MM）下求最短路径
            // 提示:
            //   1. 判断某地点在 time 时是否开放:open_time <= time <= close_time（字符串比较即可）
            //   2. 只允许使用"此时开放"的地点作为路径上的节点（包括起点和终点）
            //   3. 只走 status == "open" 的边
            //   4. 余下和普通 Dijkstra 相同；可以复用 GetShortestPath 的实现思路
            //   5. 若起点或终点此时未开放,直接返回不可达
        PathResult GetTimedShortestPath(const LGraph &graph,
                                        const std::string &from_id,
                                        const std::string &to_id,
                                        const std::string &time,
                                        PathMode mode) {
            // 起点或终点不存在,或开放时间不符,判断不可达
            if (!graph.ExistTimeVertex(from_id, time) ||
                !graph.ExistTimeVertex(to_id, time)) {
                return PathResult();
            }

            std::unordered_map<std::string, int> dist;
            std::unordered_map<std::string, std::string> prev;
            dist[from_id] = 0;

            using P = std::pair<int, std::string>;
            std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
            pq.push({0, from_id});

            while (!pq.empty()) {
                int d = pq.top().first;
                std::string u = pq.top().second;
                pq.pop();

                auto it_d = dist.find(u); if (it_d == dist.end() || d != it_d->second) continue;
                if (u == to_id) break;

                // 获取所有开放邻接边
                auto edges = graph.GetAdjacentEdges(u, true);
                for (const auto &e : edges) {
                    const std::string &v = e.to_id;
                    // 邻居顶点必须在给定时刻开放
                    if (!graph.ExistTimeVertex(v, time)) continue;

                    int weight = (mode == PathMode::TIME) ? e.walk_time : e.distance;
                    if (dist.find(v) == dist.end()) dist[v] = INT_MAX;
                    if (dist[u] != INT_MAX && dist[u] + weight < dist[v]) {
                        dist[v] = dist[u] + weight;
                        prev[v] = u;
                        pq.push({dist[v], v});
                    }
                }
            }

            if (dist.find(to_id) == dist.end() || dist[to_id] == INT_MAX)
                return PathResult();

            // 回溯路径
            std::vector<std::string> path;
            for (std::string cur = to_id; cur != from_id; cur = prev[cur])
                path.push_back(cur);
            path.push_back(from_id);
            std::reverse(path.begin(), path.end());

            return PathResult{dist[to_id], std::move(path), true};        
        }

        // ==================== C. 必经点路径规划 ====================
            // TODO: 计算必经点路径
            // 提示:
            //   1. 将路径拆分为多段:from -> w1 -> w2 -> ... -> wk -> to
            //   2. 对每段调用 GetShortestPath
            //   3. 如果任意一段不可达,则整条路径不可达
            //   4. 拼接所有段的路径（注意去掉重复的中间点）
            //   5. 累加所有段的代价作为总代价
        PathResult GetMustPassPath(const LGraph &graph,
                                   const std::string &from_id,
                                   const std::string &to_id,
                                   PathMode mode,
                                   const std::vector<std::string> &waypoints) {
            // 构建完整节点序列:起点 → 必经点 → 终点
            std::vector<std::string> stops;
            stops.push_back(from_id);
            stops.insert(stops.end(), waypoints.begin(), waypoints.end());
            stops.push_back(to_id);

            int total_cost = 0;
            std::vector<std::string> full_path;

            for (int i = 0; i + 1 < stops.size(); ++i) {
                const std::string &cur = stops[i];
                const std::string &next = stops[i + 1];

                PathResult seg = GetShortestPath(graph, cur, next, mode);
                if (!seg.reachable) {
                    return PathResult();   // 任意段不可达,整条路径不可达
                }

                total_cost += seg.total_cost;

                if (i == 0) {
                    // 第一段:直接添加整条路径
                    full_path = std::move(seg.path);
                } else {
                    // 后续段:跳过第一个点（与前一段终点重复）
                    full_path.insert(full_path.end(),
                                    seg.path.begin() + 1,
                                    seg.path.end());
                }
            }
            return PathResult{total_cost, std::move(full_path), true};
        }

        // ==================== D. 最小生成树 ====================
            // TODO: 计算最小生成树（在 Kruskal 与 Prim 中任选其一实现）
            //
            // 选项一:Kruskal + DSU
            //   1. 获取所有 status == "open" 的边,按 distance 升序排序
            //   2. 使用并查集 DSU 维护连通性
            //   3. 依次尝试加入边,若两端不在同一集合则加入 MST
            //
            // 选项二:Prim
            //   1. 从任一顶点出发,维护"已加入 MST 的顶点集合 U"
            //   2. 每次从 U 与剩余顶点之间的边中选距离最小的加入 MST
            //   3. 重复直至 U 包含所有顶点
            //
            // 共同收尾:若最终 MST 边数 != 顶点数 - 1（图不连通）,返回空 vector
            //
            // 报告中需说明你选了哪种、为什么、复杂度是多少。
        std::vector<EdgeNode> MinimumSpanningTree(const LGraph &graph) {
            // MST 仅定义在无向图上
            if (graph.IsDirected()) {
                throw GraphException("GraphAlgorithm::MinimumSpanningTree:此算法不支持有向图");
            }

            auto vertices = graph.AllPlaceIds();
            if (vertices.empty()) return {};

            std::unordered_set<std::string> visited;
            std::vector<EdgeNode> mst;

            // 小顶堆：边按 distance 排序，存储 (权重, from, to)
            struct EdgeInfo {
                int weight;
                std::string from;
                std::string to;
                bool operator>(const EdgeInfo &o) const {
                    if (weight != o.weight) {
                        return weight > o.weight;
                    } else if (from != o.from) {
                        return from > o.from;
                    } else {
                        return to > o.to;
                    }
                }
            };
            std::priority_queue<EdgeInfo, std::vector<EdgeInfo>, std::greater<EdgeInfo>> pq;

            // 从第一个顶点开始
            std::string start = vertices[0];
            visited.insert(start);
            for (const auto &e : graph.GetAdjacentEdges(start, true)) {
                pq.push({e.distance, start, e.to_id});
            }

            while (!pq.empty() && mst.size() < vertices.size() - 1) {
                auto [w, from, to] = pq.top();
                pq.pop();

                if (visited.count(to)) continue;   // 终点已加入 MST，忽略

                // 加入 MST
                visited.insert(to);
                // 注意 EdgeNode 构造需要完整字段；这里仅用核心信息，其他字段可补充默认值
                mst.push_back(EdgeNode(from, to, w, 0, "open"));  // walk_time 可留 0，MST 只需距离

                // 将新顶点的开放邻接边入堆
                for (const auto &e : graph.GetAdjacentEdges(to, true)) {
                    if (!visited.count(e.to_id)) {
                        pq.push({e.distance, to, e.to_id});
                    }
                }
            }

            // 图不连通 → 返回空
            if (mst.size() != vertices.size() - 1) {
                return {};
            }
            return mst;
        }
        

        // ==================== E. 关键节点 / 关键边分析 ====================
            // TODO: 找出"删去后会让图的连通分量数增加"的顶点和边
            //
            // 推荐方法（基于 PPT 第 3 章讲过的 BFS / DFS）:
            //   1. 先调用 GetConnectedComponents 算出原图的分量数 baseline
            //   2. 关键节点:对每个顶点 v
            //        a. 临时把 v 视作"被删除"（连同其所有相邻边一起忽略）
            //        b. 对剩余子图重新 BFS / DFS 计算连通分量数
            //        c. 若分量数 > baseline,则 v 是关键节点
            //   3. 关键边:对每条 status=="open" 的边 e
            //        a. 临时把 e 视作"closed"
            //        b. 重新计算连通分量数
            //        c. 若分量数 > baseline,则 e 是关键边
            //
            // 注意事项:
            //   - 仅考虑 status == "open" 的边和当前存在的顶点
            //   - 这种"临时删除"通常用一个"被屏蔽集合"参数控制 BFS/DFS 即可,
            //     不必真的修改图
            //   - 复杂度:O(V·(V+E)) + O(E·(V+E))
            //   - 千节点规模可接受；想优化的同学请看加分项中的 Tarjan 版本

        CriticalResult FindCriticalNodesAndEdges(const LGraph &graph) {
            if (graph.IsDirected()) {
                throw GraphException("GraphAlgorithm::FindCriticalNodesAndEdges:该算法不支持有向图");
            }

            // ---- 辅助函数: 在"屏蔽某顶点集合"下计算连通分量数 ----
            // 被屏蔽的顶点视作不存在——不从它出发 BFS，也不通过任何边到达它
            // 这等价于"临时删除该顶点连同其所有相邻边"
            auto CountCCWithBlocked = [&](
                const LGraph &g,
                const std::unordered_set<std::string> &blocked) {

                auto verts = g.AllPlaceIds();
                std::unordered_set<std::string> visited;
                int comps = 0;

                for (const auto &start : verts) {
                    if (visited.count(start)) continue;
                    if (blocked.count(start)) continue;   // 起点被屏蔽,跳过

                    std::queue<std::string> q;
                    q.push(start);
                    visited.insert(start);

                    while (!q.empty()) {
                        std::string u = q.front(); q.pop();
                        for (const auto &e : g.GetAdjacentEdges(u)) {
                            const std::string &v = e.to_id;
                            if (blocked.count(v)) continue;  // 邻居被屏蔽,不可达
                            if (!visited.count(v)) {
                                visited.insert(v);
                                q.push(v);
                            }
                        }
                    }
                    ++comps;
                }
                return comps;
            };

            // ---- 辅助函数: 在"关闭某一条边"下计算连通分量数 ----
            // 用 pair<string,string> 规范化 (min,max)，避免字符串拼接歧义
            using EdgeKey = std::pair<std::string, std::string>;

            auto CountCCWithClosedEdge = [&](
                const LGraph &g,
                const EdgeKey &closed_edge) {

                auto verts = g.AllPlaceIds();
                std::unordered_set<std::string> visited;
                int comps = 0;

                for (const auto &start : verts) {
                    if (visited.count(start)) continue;

                    std::queue<std::string> q;
                    q.push(start);
                    visited.insert(start);

                    while (!q.empty()) {
                        std::string u = q.front(); q.pop();
                        for (const auto &e : g.GetAdjacentEdges(u)) {
                            const std::string &v = e.to_id;
                            // 规范化 (min, max) 后与目标边比较
                            EdgeKey cur = (u < v) ? EdgeKey(u, v) : EdgeKey(v, u);
                            if (cur == closed_edge) continue;
                            if (!visited.count(v)) {
                                visited.insert(v);
                                q.push(v);
                            }
                        }
                    }
                    ++comps;
                }
                return comps;
            };

            // 1. 基线: 全部顶点 + 全部 open 边的连通分量数
            int base_count = GetConnectedComponents(graph).count;

            // 2. 收集所有顶点和所有 open 边（供后续逐个测试）
            std::vector<std::string> all_vertices = graph.AllPlaceIds();
            std::vector<EdgeNode> all_edges = graph.AllEdges();

            CriticalResult result;

            // ===== 关键节点: 屏蔽每个顶点,重算连通分量数 =====
            for (const auto &v : all_vertices) {
                std::unordered_set<std::string> blocked = {v};
                int new_count = CountCCWithBlocked(graph, blocked);
                if (new_count > base_count) {
                    result.critical_nodes.emplace_back(v);
                }
            }

            // ===== 关键边: 逐条边视作 closed,重算连通分量数 =====
            for (const auto &e : all_edges) {
                EdgeKey key = (e.from_id < e.to_id)
                    ? EdgeKey(e.from_id, e.to_id)
                    : EdgeKey(e.to_id, e.from_id);
                int new_count = CountCCWithClosedEdge(graph, key);
                if (new_count > base_count) {
                    result.critical_edges.emplace_back(e.from_id, e.to_id);
                }
            }

            return result;
        }

        // ==================== F. Tarjan 算法求割点与桥 ====================
            // 用显式栈迭代模拟 DFS，一次遍历同时求出所有割点和桥
            // 复杂度 O(V+E)，仅考虑 status == "open" 的边
        CriticalResult FindCriticalTarjan(const LGraph &graph) {
            if (graph.IsDirected()) {
                throw GraphException("GraphAlgorithm::FindCriticalTarjan:该算法不支持有向图");
            }

            std::vector<std::string> vertices = graph.AllPlaceIds();
            int n = static_cast<int>(vertices.size());
            if (n == 0) return {};

            // ---- 1. 建立 place_id <-> 索引的双向映射 ----
            // 字符串 id 不便于数组操作，统一转为整数索引
            std::unordered_map<std::string, int> id_to_idx;
            for (int i = 0; i < n; ++i)
                id_to_idx[vertices[i]] = i;

            // ---- 2. Tarjan 核心数据结构 ----
            std::vector<int> dfn(n, 0);          // 发现时间戳，0 = 未访问
            std::vector<int> low(n, 0);          // 能追溯到的最早 dfn
            std::vector<bool> is_art(n, false);  // 割点标记
            std::vector<std::pair<std::string, std::string>> bridges;  // 桥集合
            std::vector<int> child_cnt(n, 0);    // 每个节点在 DFS 树中的子节点数
            int timer = 0;

            // ---- 3. 显式栈帧 ----
            // 每帧记录 (当前节点, 父节点, 当前正处理的邻边索引)
            struct Frame {
                int u;       // 当前节点索引
                int parent;  // DFS 树中的父节点索引，-1 表示根
                int idx;     // 下次要处理的邻边在缓存列表中的下标
            };
            std::vector<Frame> stack;

            // ---- 4. 预处理：为每个节点缓存其 open 邻居列表 ----
            // 这样迭代时只需按索引取 neighbor，避免反复调用 GetAdjacentEdges
            std::vector<std::vector<int>> neighbors(n);
            for (int i = 0; i < n; ++i) {
                auto edges = graph.GetAdjacentEdges(vertices[i], true);
                for (const auto &e : edges) {
                    std::string nb = (e.from_id == vertices[i]) ? e.to_id : e.from_id;
                    auto it = id_to_idx.find(nb);
                    if (it != id_to_idx.end())
                        neighbors[i].push_back(it->second);
                }
            }

            // ---- 5. 主循环：对每个未访问的连通分量启动 DFS ----
            for (int start = 0; start < n; ++start) {
                if (dfn[start] != 0) continue;   // 已经访问过

                // 5a. 压入起点（根节点）
                stack.push_back({start, -1, 0});
                while (!stack.empty()) {
                    Frame &top = stack.back();    // 栈顶 = 当前正在处理的节点
                    int u = top.u;
                    int parent = top.parent;

                    if (top.idx == 0) {
                        // 第一次进入这个节点：设置时间戳
                        dfn[u] = low[u] = ++timer;
                    }

                    // 5b. 检查是否还有未处理的邻边
                    if (top.idx < static_cast<int>(neighbors[u].size())) {
                        int v = neighbors[u][top.idx];  // 取出下一条邻边
                        ++top.idx;                       // 指针前进

                        if (v == parent) continue;       // 跳过直接父节点（无向图回边）

                        if (dfn[v] == 0) {
                            // ---- 树边：v 未被访问，递归进入 ----
                            ++child_cnt[u];              // u 多了一个 DFS 子节点
                            stack.push_back({v, u, 0});  // 切换到子节点
                        } else {
                            // ---- 回边：v 已经访问过（是 u 的祖先）----
                            low[u] = std::min(low[u], dfn[v]);
                        }
                    } else {
                        // 5c. u 的所有邻边已处理完毕，即将"返回"父节点
                        stack.pop_back();  // 弹出 u（此时 stack.back() 是 parent 的帧）

                        if (parent != -1) {
                            // 把 u 的信息回传给父节点
                            low[parent] = std::min(low[parent], low[u]);

                            // 割点判定（非根节点）：
                            //   标准 Tarjan 递归版中，根节点由 children >= 2 单独判定。
                            //   迭代版中需显式判断 parent 是否为根：若 parent 的 parent == -1，
                            //   则 parent 是根，跳过 low[v] >= dfn[u] 判定。
                            bool parent_is_root = stack.back().parent == -1;
                            if (!parent_is_root && low[u] >= dfn[parent])
                                is_art[parent] = true;

                            // 桥判定（对根和非根均适用）：
                            //   low[u] > dfn[parent] ⇔ u 的子树无法绕过 parent 到达外部
                            if (low[u] > dfn[parent]) {
                                std::string a = vertices[parent];
                                std::string b = vertices[u];
                                if (a < b)
                                    bridges.emplace_back(a, b);
                                else
                                    bridges.emplace_back(b, a);
                            }
                        }
                    }
                }

                // 5d. 当前连通分量的根节点割点判定：
                //     根在 DFS 树中有 >= 2 个子树 → 去掉根后这些子树彼此断开
                if (child_cnt[start] >= 2)
                    is_art[start] = true;
            }

            // ---- 6. 整理结果 ----
            CriticalResult result;
            for (int i = 0; i < n; ++i)
                if (is_art[i])
                    result.critical_nodes.push_back(vertices[i]);

            result.critical_edges = std::move(bridges);
            return result;
        }

        // ==================== G. K 券最短路径（拓展任务 1：共享单车 / 分层图） ====================
            // 基于状态空间 Dijkstra：
            //   状态 = (place_id, tickets_used)，共 (K+1) × V 个状态
            //   转移：
            //     - 普通：cost += walk_time,            tickets 不变
            //     - 加速：cost += ceil(walk_time / 3),   tickets + 1（需 tickets < K）
            //   复杂度：O(K·(V+E)·log(K·V))
        KPathResult GetShortestKPath(const LGraph &graph,
                                     const std::string &from_id,
                                     const std::string &to_id,
                                     int K) {
            using State = std::pair<std::string, int>;   // (place_id, tickets_used)

            struct StateHash {
                size_t operator()(const State &s) const {
                    size_t h1 = std::hash<std::string>{}(s.first);
                    size_t h2 = std::hash<int>{}(s.second);
                    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
                }
            };

            // 前驱信息
            // 用于 Dijkstra 结束后从目标状态回溯完整路径和用券记录
            // prev:  通过 current 状态可追溯到的前一个状态
            // from:  这条边上靠近 current 的那个端点
            // to:    这条边上远离 current 的那个端点（即前驱要到达的节点）
            // fast:  从 prev 到 current 的转移是否使用了加速券
            struct Prev {
                State prev;            // 前一个状态
                std::string from;      // 边起点（靠近 prev）
                std::string to;        // 边终点（靠近 current）
                bool fast;             // 此边是否用了加速券
            };

            // 最短距离表 + 前驱表
            // 键：State = (place_id, tickets_used)
            // 值：到达该状态的最小总耗时 / 该状态的前驱
            // 改用 unordered_map：O(1) 平均查找，vs map 的 O(log N)
            // 状态空间 K×V ≤ 11×V，散列表碰撞开销可忽略
            std::unordered_map<State, int, StateHash> dist;
            std::unordered_map<State, Prev, StateHash> prev;
            dist[{from_id, 0}] = 0;        // 起点：未用券，耗时 0

            // 小顶堆（Dijkstra 优先队列）
            // 元素：(当前总耗时, 当前状态)
            // std::greater 使堆顶为最小值 → 小顶堆
            using PQEntry = std::pair<int, State>;
            std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;
            pq.push({0, {from_id, 0}});

            // Dijkstra 主循环
            while (!pq.empty()) {
                // 取出当前代价最小的未处理状态
                int cur_cost = pq.top().first;
                auto cur_state = pq.top().second;
                pq.pop();

                // 懒惰删除
                // 惰性删除：堆顶的旧条目若与 dist 中记录不一致则跳过。
                // 用 find() 而非 operator[]，避免 key 不存在时静默创建 int(0)
                auto it_d = dist.find(cur_state);
                if (it_d == dist.end() || cur_cost != it_d->second) continue;

                const auto &[cur_node, cur_tickets] = cur_state;

                // 遍历邻接边
                // 仅展开 status == "open" 的边
                auto edges = graph.GetAdjacentEdges(cur_node, true);
                for (const auto &e : edges) {
                    // 无向图：边的 from/to 方向不确定，手动提取邻居
                    std::string neighbor = (e.from_id == cur_node) ? e.to_id : e.from_id;
                    int w = e.walk_time;

                    // 不消耗券，耗时加 walk_time，tickets 不变
                    {
                        State ns_normal = {neighbor, cur_tickets};
                        int nc_normal = cur_cost + w;
                        auto it = dist.find(ns_normal);
                        if (it == dist.end() || nc_normal < it->second) {
                            // 发现更优路径 → 更新
                            dist[ns_normal] = nc_normal;
                            prev[ns_normal] = {cur_state, cur_node, neighbor, false};
                            pq.push({nc_normal, ns_normal});
                        }
                    }

                    // 转移 B：加速通行
                    // 消耗 1 张券，耗时加 ceil(walk_time / 3)，tickets + 1
                    // 前提：当前已用券数 < K（即还有券可用）
                    if (cur_tickets < K) {
                        int fast_cost = (w + 2) / 3;     // ceil(w / 3)，整数无浮点误差
                        State ns_fast = {neighbor, cur_tickets + 1};
                        int nc_fast = cur_cost + fast_cost;
                        auto it = dist.find(ns_fast);
                        if (it == dist.end() || nc_fast < it->second) {
                            dist[ns_fast] = nc_fast;
                            prev[ns_fast] = {cur_state, cur_node, neighbor, true};
                            pq.push({nc_fast, ns_fast});
                        }
                    }
                }
            }

            // 结果提取
            // 在所有 (to_id, t) 状态中找代价最小的（t 可为 0..K）
            KPathResult result;
            result.reachable = false;
            result.total_time = 0;
            result.k_used = 0;

            State best_state;
            int best_cost = -1;                        // -1 = 未找到可达状态
            for (int t = 0; t <= K; ++t) {
                State st = {to_id, t};
                auto it = dist.find(st);
                if (it != dist.end() && (best_cost == -1 || it->second < best_cost)) {
                    best_cost = it->second;
                    best_state = st;
                }
            }

            if (best_cost == -1) return result;        // 任何券数都无法到达终点

            result.reachable = true;
            result.total_time = best_cost;
            result.k_used = best_state.second;         // 最优方案实际用券数

            // 路径回溯
            // 从目标状态反向追溯 prev 链到起点
            std::vector<std::string> rpath;             // 反向收集的路径
            State cur = best_state;
            while (cur != State{from_id, 0}) {
                Prev &p = prev[cur];
                rpath.push_back(p.to);                 // 收集节点（逆序）
                if (p.fast) {
                    // 记录用券的边，统一规范化为 (min, max)
                    std::string u = std::min(p.from, p.to);
                    std::string v = std::max(p.from, p.to);
                    result.fast_edges.emplace_back(u, v);
                }
                cur = p.prev;                          // 继续回溯
            }
            rpath.push_back(from_id);                  // 补上起点

            // 翻转得到正向路径
            // 回溯时是从终点 → 起点反向走的，这里翻转为正向
            result.path.assign(rpath.rbegin(), rpath.rend());
            return result;
        }

    }// Algorithm
}// Graph


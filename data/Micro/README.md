# 微型数据集

## 一、Micro — 微型数据集

本目录包含两个自行设计的微型数据集，覆盖真实场景（上海地铁）和纯数学图（骑士图），V ≤ 24、E ≤ 31，便于手工验证和快速调试。

| 数据集 | V | E（open） | E（closed） | 特点 |
|--------|---|----------|------------|------|
| `shanghai_metro` | 24 | 28 | 3 | 真实换乘网络、时间窗约束、多环结构 |
| `knight_graph` | 16 | 24 | 0 | 数学构造、等权边、二部图 |

---

### 1. shanghai_metro — 上海地铁网络

#### 数据来源

基于上海地铁 2 号线、4 号线、6 号线、9 号线真实站点和线路拓扑构造。站间距离根据公开地图资料估算，运营时间参考各线路首末班车时刻表。

#### 建模设计

- **顶点**：24 个站点，`place_id` 为 `S001~S024`，`category` 标注所属线路，换乘站用 `|` 分隔（如 `Line2|Line4|Line6|Line9`）
- **边**：31 条轨道连接，其中 3 条为 `closed`（模拟施工关闭），`distance` 和 `walk_time` 独立设置
- **时间窗**：各站点 `open_time`/`close_time` 不同，`S022` 23:10 关闭、`S023` 23:20 关闭
- **换乘枢纽**：`S006`（人民广场，三线）、`S009`（世纪大道，四线）

#### 命令序列

```
LOAD → COMPONENTS → SHORTEST(DIST) → SHORTEST(TIME)
→ TIMED_SHORTEST(23:00) → MUST_PASS(S006,S009) → MST → CRITICAL → SHORTEST_K(K=3) → QUIT
```

#### 预期结果

| 命令 | 关键输出 |
|------|---------|
| COMPONENTS | 1 分量，24 节点 |
| SHORTEST S001→S024 | DIST=39900, TIME=49 |
| TIMED_SHORTEST 23:00 | NO_PATH（S022/S023 已关闭） |
| MUST_PASS S006,S009 | 经人民广场和世纪大道绕行 |
| MST | 总距 55900，23 条边 |
| CRITICAL | 9 割点（S002~S023 链上节点），9 桥 |
| SHORTEST_K K=3 | TIME=33, K_USED=3 |

---

### 2. knight_graph — 骑士图（4×4 棋盘）

#### 数据来源

自行构造的数学图，基于国际象棋骑士走法规则。4×4 棋盘上，若骑士能从一格跳到另一格则连边。

*值得一提的是，此处默认为`国际象棋`，假如采用`中国象棋`，则可以引入`蹩马腿`，在图上表现为某一条边被`临时关闭`*

#### 建模设计

- **顶点**：16 个格子（`R1C1~R4C4`），类别统一 `Chessboard`
- **边**：24 条，所有权重 = 1（等权），全部 `open`（*有蹩马腿则可以有closed边*）
- **时间窗**：消除时间维度干扰，聚焦图结构
- **二部图性质**：骑士每步改变格子颜色

#### 命令序列

```
LOAD → COMPONENTS → SHORTEST(DIST) → SHORTEST(TIME) → MST → CRITICAL → QUIT
```

#### 预期结果

| 命令 | 关键输出 |
|------|---------|
| COMPONENTS | 1 分量，16 节点 |
| SHORTEST R1C1→R4C4 | DIST=TIME=2 |
| MST | 总距 15，15 条边 |
| CRITICAL | NODES 0 EDGES 0（无割点/桥） |

---

### 检验覆盖

| 算法 | shanghai_metro | knight_graph |
|------|:--:|:--:|
| COMPONENTS | ✅ | ✅ |
| SHORTEST (DIST/TIME) | ✅ | ✅ |
| TIMED_SHORTEST | ✅ | — |
| MUST_PASS | ✅ | — |
| MST | ✅ | ✅ |
| CRITICAL | ✅（链式割点桥） | ✅（零结果） |
| SHORTEST_K | ✅ | — |
| 时间窗约束 | ✅ | — |
| closed 边 | ✅ | — |

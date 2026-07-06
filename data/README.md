# 测试数据 v2

按"必做"和"拓展"两个目录组织。每个 case 含 `places.csv`、`roads.csv`、`command.txt`、`answer.txt` 四个文件。

`answer.txt` 是**参考输出**，作为学生程序的依据。由于本项目接口与输出格式不强制（学生可自选 CLI / Web / GUI 等），`answer.txt` 不要求字符级一致，只要语义吻合即可。

---

## 必做任务测试数据 `必做/`

| 路径 | 节点 | 边 | 主测点 |
|------|------|----|--------|
| `小例` 系列 | | | |
| `必做/small_cases/case_01` | 6 | 8 | 数据维护 + 查询 + 双模式 SHORTEST + COMPONENTS + MST + CRITICAL |
| `必做/small_cases/case_02` | 8 | 9 | CLOSE_ROAD/OPEN_ROAD 影响下的 COMPONENTS / SHORTEST，DELETE_PLACE |
| `必做/small_cases/case_03` | 7 | 10 | 双模式 SHORTEST + MUST_PASS（DIST/TIME 两次） + MST + CRITICAL |
| `必做/small_cases/case_04` | 8 | 10 | TIMED_SHORTEST 专项（混合时间窗：全开 / 早 / 午 / 晚） |
| `中例` 系列（来自旧 cases.zip 的拓扑 + 转新 schema） | | | |
| `必做/medium_cases/chain_flower_100` | 100 | 99 | 链 + 花（树状）拓扑 |
| `必做/medium_cases/grid_100` | 100 | 180 | 10×10 网格 |
| `必做/medium_cases/ring_100` | 100 | 100 | 100 节点环 |
| `必做/medium_cases/star_100` | 100 | 99 | 星形（中心点是关键节点） |
| `必做/medium_cases/random_sparse_100` | 100 | 150 | 随机稀疏 |
| `必做/medium_cases/mixed_time_100` | 100 | 200 | 混合时间窗，TIMED_SHORTEST 在 10:00 / 15:00 / 20:00 三个时刻对比 |
| `大例` 系列 | | | |
| `必做/large_cases/chain_1000` | 1000 | 999 | 链 1000 节点：998 关键节点 + 999 关键边，验证 1000+ 不超时 |
| `必做/large_cases/random_sparse_10000` | 10000 | ~15000 | **从旧 cases.zip 复用**的大规模稀疏图，压力测试 |
| 历史数据 | | | |
| `必做/sample_ecnu` | 87 | 26 | 旧版 ECNU 校园数据迁移版 |

### 必做使用的命令格式

| 命令 | 形式 |
|------|------|
| SHORTEST | `SHORTEST <from> <to> <DIST\|TIME>` |
| TIMED_SHORTEST | `TIMED_SHORTEST <from> <to> <time> <DIST\|TIME>` |
| MUST_PASS | `MUST_PASS <from> <to> <DIST\|TIME> <k> <p1> ... <pk>` |

---

## 拓展任务 1 测试数据 

只用于"拓展任务 1：共享单车"，命令名 **`SHORTEST_K`**（与必做的 SHORTEST 区分开）。

| 路径 | 节点 | 边 | 主测点 |
|------|------|----|--------|
| `拓展_分层图/small/case_layered_01` | 5 | 5 | 小例：覆盖不同 K 取值 |
| `拓展_分层图/medium/random_sparse_100` | 100 | 150 | 随机稀疏，K=0/3/5/10 多档 |
| `拓展_分层图/medium/grid_100` | 100 | 180 | 网格，K=0/3/8 |
| `拓展_分层图/large/random_sparse_1000` | 1000 | 2000 | 大规模分层图压力测试 |

---

## 拓扑特性提醒

- **chain（链）**：N 节点 → N-2 个关键节点 + N-1 条关键边；MST 就是链本身；连通分量 = 1
- **ring（环）**：无关键节点、无关键边；MST 任删一条边即可
- **grid（网格）**：内部点不是关键节点（绕一圈）
- **star（星形）**：中心点是唯一的关键节点；所有边都是关键边
- **chain 中跑 SHORTEST(端到端)**：路径长度 = 全部 N-1 条边权重之和
- **TIMED_SHORTEST 起点或终点未开放** → `NO_PATH`

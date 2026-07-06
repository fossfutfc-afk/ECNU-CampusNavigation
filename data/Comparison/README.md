# 算法对比

## 二、Comparison — 算法对比

> 每组两份**完全相同的 CSV 数据**，仅 `command.txt` 不同——一个调 `CRITICAL`（朴素 BFS），一个调 `CRITICAL_TARJAN`（Tarjan）。运行 `testall` 时两者并排显示，可直接对比耗时。

### 2.1 k80_naive / k80_tarjan

| 属性 | 值 |
|------|-----|
| 拓扑 | 完全图 K₈₀ |
| V / E | 80 / 3,160 |
| 命令 | `CRITICAL` / `CRITICAL_TARJAN` |
| 朴素时长 | 1.8s |
| Tarjan时常 | 0.04s |
| 加速比 | 45× |

完全图中不存在割点/桥。朴素 BFS 需要对 80 个顶点 + 3160 条边各做一次 BFS，共 3240 次遍历；相比之下 Tarjan 一次 DFS 即出结果。

### 2.2 grid_naive / grid_tarjan

| 属性 | 值 |
|------|-----|
| 拓扑 | 32×32 网格 |
| V / E | ~ 1 / 2 |
| 命令 | `CRITICAL` / `CRITICAL_TARJAN` |
| 朴素时长 | 1.7s |
| Tarjan 时长  | 0.04s |
| 加速比 | 42× |

网格图内部节点有替代路径，无割点。1,024 + 1,984 = 3,008 次 BFS 遍历。

---


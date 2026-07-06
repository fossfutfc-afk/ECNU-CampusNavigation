"""
CampusNavigation Web Bridge v2
每个 API 请求启动独立的 CampusNavigation.exe（避免管道死锁），
发送完整命令序列（LOAD + 算法 + QUIT），捕获全部输出。
"""
import subprocess as sp
import json, math, os, re

from flask import Flask, request, jsonify

app = Flask(__name__, static_folder="static")

@app.after_request
def no_cache(response):
    response.headers['Cache-Control'] = 'no-store, no-cache, must-revalidate'
    return response

EXE     = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "build", "CampusNavigation.exe"))
TESTALL = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "build", "testall.exe"))
DATA    = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "data"))

# ---------------------------------------------------------------------------
# 1. 命令执行 — 每次启动新进程，避免管道死锁
# ---------------------------------------------------------------------------
def run_session(commands, timeout=30):
    """
    向 exe 发送一串命令（每行一条），捕获 stdout，返回字符串列表。
    """
    stdin_text = "\n".join(commands) + "\n"
    try:
        p = sp.Popen(
            [EXE],
            stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.PIPE,
            text=True, encoding="utf-8",
            cwd=os.path.dirname(EXE)
        )
        out, err = p.communicate(input=stdin_text, timeout=timeout)
        if err:
            print(f"[run_session] stderr: {err[:200]}", flush=True)
        return [l for l in out.split("\n") if l.strip()] if out else []
    except sp.TimeoutExpired:
        p.kill()
        return []
    except Exception as e:
        return [f"ERROR: {e}"]

# ---------------------------------------------------------------------------
# 2. 解析函数
# ---------------------------------------------------------------------------
def parse_shortest(lines):
    for line in lines:
        if line.startswith("PATH "):
            parts = line.split()
            mode = parts[1]; cost = int(parts[2])
            ni = parts.index("NODES")
            return {"mode": mode, "cost": cost, "path": parts[ni+1:]}
    return None

def parse_mst(lines):
    for line in lines:
        if line.startswith("MST "):
            parts = line.split()
            total = int(parts[1])
            edges = []
            for ep in parts[3:]:
                uv, w = ep.rsplit(":", 1)
                u, v = uv.split("-", 1)
                edges.append({"u": u, "v": v, "w": int(w)})
            return {"total": total, "edges": edges}
    return None

def parse_critical(lines):
    for line in lines:
        if line.startswith("CRITICAL "):
            parts = line.split()
            ni = parts.index("NODES"); ei = parts.index("EDGES")
            nc = int(parts[ni+1]); ec = int(parts[ei+1])
            nodes = parts[ni+2:ei] if nc > 0 else []
            estr = parts[ei+2:] if ec > 0 else []
            edges = []
            for es in estr:
                u, v = es.split("-", 1)
                edges.append({"u": u, "v": v})
            return {"critical_nodes": nodes, "critical_edges": edges}
    return None

def parse_components(lines):
    for line in lines:
        if line.startswith("COMPONENTS "):
            parts = line.split()
            cnt = int(parts[1])
            si = parts.index("SIZES") if "SIZES" in parts else -1
            sizes = [int(x) for x in parts[si+1:]] if si >= 0 else []
            return {"count": cnt, "sizes": sizes}
    return None

# ---------------------------------------------------------------------------
# 3. 布局引擎
# ---------------------------------------------------------------------------
positions = {}

def auto_layout(place_ids):
    global positions
    n = len(place_ids)
    cols = math.ceil(math.sqrt(n))
    positions.clear()
    for i, pid in enumerate(sorted(place_ids)):
        positions[pid] = {"x": (i % cols) * 55 + 30, "y": (i // cols) * 55 + 30}

# ---------------------------------------------------------------------------
# 4. 数据收集
# ---------------------------------------------------------------------------
MAX_EDGES = 100000  # Web 端加载上限，超过拒绝

def count_lines(path):
    """快速统计文件行数（不含表头）"""
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return max(0, sum(1 for _ in f) - 1)
    except Exception:
        return 0

def collect_graph(data_dir):
    """加载数据并收集全图信息"""
    pf = os.path.normpath(os.path.join(DATA, data_dir, "places.csv"))
    rf = os.path.normpath(os.path.join(DATA, data_dir, "roads.csv"))

    # 超大图拦截
    edge_count = count_lines(rf)
    if edge_count > MAX_EDGES:
        return {
            "vertices": [],
            "edges": [],
            "bad_edges": [],
            "has_bad_data": False,
            "components": {"count": 0, "sizes": []},
            "rejected": True,
            "reject_reason": f"数据集过大（{edge_count} 条边 > {MAX_EDGES} 上限），Web 端不支持加载。请使用 CLI 或 testall。"
        }

    # 从 CSV 读取顶点信息（过滤非法数据）
    vertices = {}
    if os.path.exists(pf):
        with open(pf, "r", encoding="utf-8") as f:
            next(f, None)
            for line in f:
                line = line.strip()
                if not line: continue
                parts = line.split(",")
                if len(parts) < 6: continue
                try:
                    stay = int(parts[3])
                except ValueError:
                    continue
                if stay < 0: continue                        # 非法停留时间
                op, cl = parts[4].strip(), parts[5].strip()
                if op > cl: continue                         # open > close
                pid = parts[0]
                vertices[pid] = {
                    "id": pid, "name": parts[1], "category": parts[2],
                    "stay_time": stay, "open_time": op, "close_time": cl
                }

    auto_layout(list(vertices.keys()))

    # 读取边（合法/非法分开，非法边标红但不参与算法）
    edges = []
    bad_edges = []
    has_bad = False
    if os.path.exists(rf):
        with open(rf, "r", encoding="utf-8") as f:
            next(f, None)
            for line in f:
                line = line.strip()
                if not line: continue
                parts = line.split(",")
                if len(parts) < 5: continue
                try:
                    dist = int(parts[2])
                    walk = int(parts[3])
                except ValueError:
                    continue
                status = parts[4].strip()
                bad = (dist < 0 or walk < 0 or status not in ("open", "closed"))
                edge = {
                    "from": parts[0], "to": parts[1],
                    "distance": dist, "walk_time": walk,
                    "status": status, "illegal": bad
                }
                if bad:
                    bad_edges.append(edge)
                    has_bad = True
                else:
                    edges.append(edge)

    # 连通分量通过 exe 获取
    out = run_session([f"LOAD {pf} {rf}", "COMPONENTS", "QUIT"])
    comps = {"count": 0, "sizes": []}
    for line in out:
        if line.startswith("COMPONENTS "):
            comps = parse_components([line]) or comps


    return {
        "vertices": [
            {"id": k, **v, "x": positions[k]["x"], "y": positions[k]["y"]}
            for k, v in vertices.items()
        ],
        "edges": edges,
        "bad_edges": bad_edges,
        "has_bad_data": has_bad,
        "components": comps
    }

# ---------------------------------------------------------------------------
# 5. API 路由
# ---------------------------------------------------------------------------

def load_cmd(data_dir):
    """返回指定数据集的 LOAD 命令（不加引号——istringstream>>不能去引号）"""
    pf = os.path.normpath(os.path.join(DATA, data_dir, "places.csv"))
    rf = os.path.normpath(os.path.join(DATA, data_dir, "roads.csv"))
    return f"LOAD {pf} {rf}"

@app.before_request
def log_request():
    print(f"[REQUEST] {request.method} {request.path}", flush=True)

@app.route("/")
def index():
    return app.send_static_file("index.html")

@app.route("/api/load", methods=["POST"])
def api_load():
    global _current_dir
    _current_dir = request.json.get("dir", "Must/sample_ecnu")
    data = collect_graph(_current_dir)
    data["_ts"] = __import__('time').time()  # 时间戳验证新鲜度
    return jsonify(data)

@app.route("/api/graph", methods=["GET"])
def api_graph():
    if _current_dir is None:
        return jsonify({"error": "请先加载数据"}), 400
    data = collect_graph(_current_dir)
    return jsonify(data)

@app.route("/api/shortest", methods=["GET"])
def api_shortest():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    out = run_session([lc, f'SHORTEST {request.args["from"]} {request.args["to"]} {request.args.get("mode","DIST")}', "QUIT"])
    r = parse_shortest(out)
    return jsonify(r or {"error": "NO_PATH"})

@app.route("/api/mst", methods=["GET", "POST"])
def api_mst():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    raw = run_session([lc, "MST", "QUIT"])
    r = parse_mst(raw)
    return jsonify(r if r else {"error": "DISCONNECTED"})

@app.route("/api/timed_shortest", methods=["GET"])
def api_timed_shortest():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    out = run_session([lc, f'TIMED_SHORTEST {request.args["from"]} {request.args["to"]} {request.args.get("time","10:00")} {request.args.get("mode","DIST")}', "QUIT"])
    r = parse_shortest(out)
    return jsonify(r or {"error": "NO_PATH"})

@app.route("/api/shortest_k", methods=["GET"])
def api_shortest_k():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    k = request.args.get("k", "3")
    out = run_session([lc, f'SHORTEST_K {request.args["from"]} {request.args["to"]} {k}', "QUIT"])
    for line in out:
        if line.startswith("PATH "):
            parts = line.split()
            total_time = parts[1]
            ku_idx = parts.index("K_USED")
            k_used = parts[ku_idx + 1]
            nodes_idx = parts.index("NODES")
            fast_idx = parts.index("FAST")
            fast_count = int(parts[fast_idx + 1])
            path = parts[nodes_idx+1:fast_idx]
            fast_edges = []
            for i in range(fast_count):
                uv = parts[fast_idx + 2 + i]
                u, v = uv.split("-", 1)
                fast_edges.append({"u": u, "v": v})
            return jsonify({"total_time": int(total_time), "k_used": int(k_used),
                           "path": path, "fast_edges": fast_edges})
    return jsonify({"error": "NO_PATH"})

@app.route("/api/critical_tarjan", methods=["GET"])
def api_critical_tarjan():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    out = run_session([lc, "CRITICAL_TARJAN", "QUIT"])
    r = parse_critical(out)
    return jsonify(r if r else {"critical_nodes":[],"critical_edges":[]})

@app.route("/api/critical", methods=["GET"])
def api_critical():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    raw = run_session([lc, "CRITICAL", "QUIT"])
    r = parse_critical(raw)
    if r is None:
        r = {"critical_nodes":[],"critical_edges":[], "_dbg": f"dir={data_dir} cmd={lc} raw={raw}"}
    return jsonify(r)

@app.route("/api/must_pass", methods=["GET"])
def api_must_pass():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    from_id = request.args["from"]
    to_id = request.args["to"]
    mode = request.args.get("mode", "DIST")
    wps = request.args.get("wps", "").split()
    k = len(wps)
    out = run_session([lc, f'MUST_PASS {from_id} {to_id} {mode} {k} {" ".join(wps)}', "QUIT"])
    r = parse_shortest(out)
    return jsonify(r or {"error": "NO_PATH"})

@app.route("/api/components", methods=["GET"])
def api_components():
    data_dir = request.args.get("dir", "Must/sample_ecnu")
    lc = load_cmd(data_dir)
    out = run_session([lc, "COMPONENTS", "QUIT"])
    return jsonify(parse_components(out))

@app.route("/api/testall", methods=["POST"])
def api_testall():
    try:
        p = sp.run(
            [TESTALL],
            capture_output=True, text=True, encoding="utf-8", timeout=120,
            cwd=os.path.dirname(TESTALL)
        )
        clean = re.sub(r'\x1b\[[0-9;]*m', '', p.stdout)
        return jsonify({"output": clean})
    except sp.TimeoutExpired:
        return jsonify({"output": "testall 超时 (120s)"})
    except Exception as e:
        return jsonify({"output": f"testall 运行失败: {e}\n检查 {TESTALL} 是否存在"})

@app.route("/api/datasets", methods=["GET"])
def api_datasets():
    dirs = []
    for root, subdirs, files in os.walk(DATA):
        if "command.txt" in files:
            rel = os.path.relpath(root, DATA).replace("\\", "/")
            dirs.append(rel)
    dirs.sort()
    return jsonify(dirs)

# ---------------------------------------------------------------------------
# 6. 启动
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    print(f"CampusNavigation Web Bridge")
    print(f"  EXE : {EXE}  (存在: {os.path.exists(EXE)})")
    print(f"  DATA: {DATA}")

    # 启动时预生成所有数据集的图数据，写入 JS 文件
    all_data = {}
    for root, dirs, files in os.walk(DATA):
        if "command.txt" in files:
            rel = os.path.relpath(root, DATA).replace("\\", "/")
            try:
                g = collect_graph(rel)
                all_data[rel] = g
                print(f"  {rel}: V={len(g['vertices'])} E={len(g['edges'])}")
            except Exception as e:
                print(f"  {rel}: SKIP ({e})")

    js_path = os.path.join(os.path.dirname(__file__), "static", "graph_data.js")
    with open(js_path, "w", encoding="utf-8") as f:
        f.write("const ALL_GRAPHS = ")
        f.write(json.dumps(all_data, ensure_ascii=False))
        f.write(";\n")
    print(f"  已生成 graph_data.js ({len(all_data)} 个数据集)")
    print(f"  打开 http://localhost:8080")
    app.run(host="127.0.0.1", port=8080, debug=False)

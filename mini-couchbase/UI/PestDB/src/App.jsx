/* eslint-disable no-unused-vars */
import React, { useState, useEffect, useMemo, useRef } from "react";
import {
  AreaChart, Area, LineChart, Line, BarChart, Bar, XAxis, YAxis,
  CartesianGrid, Tooltip, ResponsiveContainer, Legend,
} from "recharts";
import {
  Database, Search, Plus, RefreshCw, Terminal, HardDrive, Activity,
  Gauge, FileText, Settings as SettingsIcon, ChevronRight, X, Pencil,
  Trash2, Play, CheckCircle2, AlertTriangle, Circle, Layers, Zap,
  Clock, ArrowDown, LayoutDashboard, FolderOpen, TrendingUp, Timer,
  ShieldCheck, ChevronDown,
} from "lucide-react";

/* ============================== DESIGN TOKENS ============================== */
const C = {
  bg: "#0A0E14",
  surface: "#0F1520",
  surface2: "#141C29",
  surface3: "#1A2331",
  border: "#212C3B",
  borderLight: "#2B3849",
  text: "#E7ECF3",
  textDim: "#8C98AB",
  textFaint: "#5C6A80",
  accent: "#4E8CFF",
  accentDim: "#4E8CFF33",
  cyan: "#2DD4E8",
  green: "#34D399",
  yellow: "#F0B94D",
  red: "#F0576B",
  purple: "#A78BFA",
};


const NAV_ITEMS = [
  { id: "dashboard", label: "Dashboard", icon: LayoutDashboard },
  { id: "explorer", label: "Data Explorer", icon: FolderOpen },
  { id: "console", label: "Query Console", icon: Terminal },
  { id: "storage", label: "Storage Engine", icon: Layers },
  { id: "performance", label: "Performance", icon: TrendingUp },
  { id: "benchmark", label: "Benchmark", icon: Zap },
  { id: "wal", label: "WAL & Recovery", icon: ShieldCheck },
  { id: "settings", label: "Settings", icon: SettingsIcon },
];
const mono = { fontFamily: "'IBM Plex Mono','JetBrains Mono',ui-monospace,monospace" };
const sans = { fontFamily: "'Inter',system-ui,sans-serif" };

function fmtNum(n) {
  return n.toLocaleString("en-US");
}
function fmtBytes(mb) {
  if (mb >= 1024) return (mb / 1024).toFixed(2) + " GB";
  return mb.toFixed(1) + " MB";
}
function useMockData() {
  const [data] = useState(() => {
    const now = Date.now();

    const opsOverTime = Array.from({ length: 24 }, (_, i) => {
      const t = new Date(now - (23 - i) * 5 * 60000);

      const label =
        t.getHours().toString().padStart(2, "0") +
        ":" +
        t.getMinutes().toString().padStart(2, "0");

      const wobble =
        Math.sin(i / 2.3) * 3200 +
        Math.sin(i / 5) * 1500;

      return {
        time: label,
        put: Math.round(
          28000 + wobble + (Math.random() - 0.5) * 2000
        ),
        get: Math.round(
          14700 + wobble * 0.6 + (Math.random() - 0.5) * 1200
        ),
        del: Math.round(
          27000 + wobble * 0.9 + (Math.random() - 0.5) * 1800
        ),
      };
    });

    const readWrite = opsOverTime.map((d) => ({
      time: d.time,
      reads: d.get,
      writes: d.put + d.del,
    }));

    const latencyTrend = Array.from({ length: 24 }, (_, i) => ({
      time: opsOverTime[i].time,
      p50: Math.round(
        28 + Math.sin(i / 3) * 6 + Math.random() * 4
      ),
      p95: Math.round(
        56 + Math.sin(i / 3) * 10 + Math.random() * 8
      ),
      p99: Math.round(
        100 + Math.sin(i / 3) * 18 + Math.random() * 14
      ),
    }));

    const storageGrowth = Array.from(
      { length: 14 },
      (_, i) => ({
        day: "Day " + (i + 1),
        sizeMB: Math.round(
          180 + i * 34 + Math.sin(i) * 12
        ),
      })
    );

    const keyTypes = [
      "string",
      "string",
      "string",
      "json",
      "string",
      "int",
    ];

    const prefixes = [
      "user",
      "session",
      "config",
      "order",
      "cache",
      "product",
    ];

    const keysList = Array.from({ length: 42 }, (_, i) => {
      const p = prefixes[i % prefixes.length];
      const id = 1000 + i;

      const values = {
        user: "Harsha Gowda",
        session: "active",
        config: "dark",
        order: '{"status":"shipped","total":249.5}',
        cache: "cached_response_blob",
        product: "SSD 1TB NVMe",
      };

      const type = keyTypes[i % keyTypes.length];
      const val = values[p];

      return {
        key: `${p}:${id}`,
        value: val,
        type,
        size: (val.length * 1.4 + 12).toFixed(0) + " B",

        modified: new Date(
          now - i * 634000
        ).toLocaleString("en-US", {
          month: "short",
          day: "numeric",
          hour: "2-digit",
          minute: "2-digit",
        }),

        location:
          i % 5 === 0
            ? "MemTable (active)"
            : `SSTable #${(i % 6) + 1}`,
      };
    });

    return {
      opsOverTime,
      readWrite,
      latencyTrend,
      storageGrowth,
      keysList,
    };
  });

  return data;
}
/* ============================== SHARED UI ============================== */
function Card({ children, style, className = "", padded = true }) {
  return (
    <div
      className={className}
      style={{
        background: C.surface,
        border: `1px solid ${C.border}`,
        borderRadius: 8,
        padding: padded ? 16 : 0,
        ...style,
      }}
    >
      {children}
    </div>
  );
}

function PageHeader({ title, subtitle, right }) {
  return (
    <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", marginBottom: 20 }}>
      <div>
        <h1 style={{ ...sans, fontSize: 20, fontWeight: 600, color: C.text, letterSpacing: -0.2 }}>{title}</h1>
        {subtitle && <p style={{ ...sans, fontSize: 13, color: C.textDim, marginTop: 4 }}>{subtitle}</p>}
      </div>
      {right}
    </div>
  );
}

function StatusDot({ color, label }) {
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
      <span style={{ width: 7, height: 7, borderRadius: 999, background: color, boxShadow: `0 0 6px ${color}` }} />
      <span style={{ ...sans, fontSize: 12, color: C.textDim, fontWeight: 500 }}>{label}</span>
    </div>
  );
}

function MetricCard({ label, value, unit, icon: Icon, accent, delta }) {
  return (
    <Card style={{ display: "flex", flexDirection: "column", gap: 10 }}>
      <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
        <span style={{ ...sans, fontSize: 11.5, color: C.textDim, fontWeight: 500, textTransform: "uppercase", letterSpacing: 0.4 }}>
          {label}
        </span>
        {Icon && (
          <div style={{ width: 26, height: 26, borderRadius: 6, background: (accent || C.accent) + "1F", display: "flex", alignItems: "center", justifyContent: "center" }}>
            <Icon size={14} color={accent || C.accent} />
          </div>
        )}
      </div>
      <div style={{ display: "flex", alignItems: "baseline", gap: 6 }}>
        <span style={{ ...mono, fontSize: 24, fontWeight: 600, color: C.text }}>{value}</span>
        {unit && <span style={{ ...sans, fontSize: 12, color: C.textFaint }}>{unit}</span>}
      </div>
      {delta && (
        <span style={{ ...sans, fontSize: 11.5, color: delta.startsWith("+") ? C.green : C.red }}>{delta} vs last hour</span>
      )}
    </Card>
  );
}

function ChartShell({ title, height = 220, children }) {
  return (
    <Card>
      <div style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text, marginBottom: 12 }}>{title}</div>
      <div style={{ width: "100%", height }}>
        <ResponsiveContainer width="100%" height="100%">{children}</ResponsiveContainer>
      </div>
    </Card>
  );
}

const tooltipStyle = {
  background: C.surface3, border: `1px solid ${C.borderLight}`, borderRadius: 6,
  fontSize: 12, ...mono, color: C.text,
};
const axisStyle = { fontSize: 10.5, fill: C.textFaint, ...mono };

/* ============================== SIDEBAR ============================== */
function Sidebar({ page, setPage }) {
  return (
    <div style={{ width: 232, minWidth: 232, background: C.surface, borderRight: `1px solid ${C.border}`, display: "flex", flexDirection: "column", height: "100vh" }}>
      <div style={{ padding: "20px 18px 16px", borderBottom: `1px solid ${C.border}` }}>
        <div style={{ display: "flex", alignItems: "center", gap: 9 }}>
          <div style={{ width: 30, height: 30, borderRadius: 7, background: C.accent, display: "flex", alignItems: "center", justifyContent: "center" }}>
            <Database size={16} color="#081020" strokeWidth={2.4} />
          </div>
          <div>
            <div style={{ ...sans, fontWeight: 700, fontSize: 15.5, color: C.text, letterSpacing: -0.2 }}>MDDB</div>
          </div>
        </div>
        <div style={{ ...sans, fontSize: 10.5, color: C.textFaint, marginTop: 8, lineHeight: 1.4 }}>
          High-Performance Key-Value Database
        </div>
      </div>

      <div style={{ flex: 1, padding: "12px 10px", overflowY: "auto" }}>
        {NAV_ITEMS.map((item) => {
          const active = page === item.id;
          const Icon = item.icon;
          return (
            <button
              key={item.id}
              onClick={() => setPage(item.id)}
              style={{
                width: "100%", display: "flex", alignItems: "center", gap: 10,
                padding: "8px 10px", marginBottom: 2, borderRadius: 6, border: "none",
                background: active ? C.accentDim : "transparent",
                cursor: "pointer", textAlign: "left",
              }}
            >
              <Icon size={15.5} color={active ? C.accent : C.textDim} />
              <span style={{ ...sans, fontSize: 13, color: active ? C.text : C.textDim, fontWeight: active ? 600 : 500 }}>
                {item.label}
              </span>
              {active && <span style={{ marginLeft: "auto", width: 3, height: 14, borderRadius: 2, background: C.accent }} />}
            </button>
          );
        })}
      </div>

      <div style={{ padding: 14, borderTop: `1px solid ${C.border}` }}>
        <div style={{ background: C.surface2, border: `1px solid ${C.border}`, borderRadius: 7, padding: 11 }}>
          <StatusDot color={C.green} label="Connection: ONLINE" />
          <div style={{ ...mono, fontSize: 10.5, color: C.textFaint, marginTop: 7 }}>MDDB v1.0</div>
        </div>
      </div>
    </div>
  );
}

/* ============================== DASHBOARD PAGE ============================== */
function DashboardPage({ data }) {
  return (
    <div>
      <PageHeader
        title="MDDB Dashboard"
        subtitle="Cluster overview and real-time operational metrics"
        right={<StatusDot color={C.green} label="● ONLINE" />}
      />

      <div style={{ display: "grid", gridTemplateColumns: "repeat(6, 1fr)", gap: 12, marginBottom: 14 }}>
        <MetricCard label="Total Keys" value="184,302" icon={Database} />
        <MetricCard label="Active MemTable" value="6,214" unit="entries" icon={Layers} accent={C.cyan} />
        <MetricCard label="SSTable Count" value="18" icon={HardDrive} accent={C.purple} />
        <MetricCard label="WAL Size" value="42.8" unit="MB" icon={FileText} accent={C.yellow} />
        <MetricCard label="Total Storage" value="2.34" unit="GB" icon={HardDrive} accent={C.accent} />
        <MetricCard label="Compactions" value="127" unit="completed" icon={CheckCircle2} accent={C.green} />
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12, marginBottom: 14 }}>
        <MetricCard label="PUT Throughput" value="28,000" unit="ops/sec" icon={Zap} accent={C.accent} delta="+4.2%" />
        <MetricCard label="GET Throughput" value="14,700" unit="ops/sec" icon={Zap} accent={C.cyan} delta="+1.8%" />
        <MetricCard label="DELETE Throughput" value="27,000" unit="ops/sec" icon={Zap} accent={C.red} delta="-0.6%" />
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12, marginBottom: 14 }}>
        <MetricCard label="P50 Latency" value="29" unit="µs" icon={Timer} accent={C.green} />
        <MetricCard label="P95 Latency" value="57" unit="µs" icon={Timer} accent={C.yellow} />
        <MetricCard label="P99 Latency" value="100" unit="µs" icon={Timer} accent={C.red} />
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12, marginBottom: 12 }}>
        <ChartShell title="Operations Per Second">
          <AreaChart data={data.opsOverTime}>
            <defs>
              <linearGradient id="putGrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={C.accent} stopOpacity={0.35} />
                <stop offset="100%" stopColor={C.accent} stopOpacity={0} />
              </linearGradient>
            </defs>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={5} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Area type="monotone" dataKey="put" stroke={C.accent} fill="url(#putGrad)" strokeWidth={2} dot={false} name="PUT" />
          </AreaChart>
        </ChartShell>

        <ChartShell title="Read vs Write Operations">
          <BarChart data={data.readWrite}>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={5} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Legend wrapperStyle={{ fontSize: 11, ...sans, color: C.textDim }} />
            <Bar dataKey="reads" stackId="a" fill={C.cyan} name="Reads" radius={[0, 0, 0, 0]} />
            <Bar dataKey="writes" stackId="a" fill={C.accent} name="Writes" radius={[2, 2, 0, 0]} />
          </BarChart>
        </ChartShell>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12 }}>
        <ChartShell title="Latency Trend (P50 / P95 / P99)">
          <LineChart data={data.latencyTrend}>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={5} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Legend wrapperStyle={{ fontSize: 11, ...sans, color: C.textDim }} />
            <Line type="monotone" dataKey="p50" stroke={C.green} strokeWidth={2} dot={false} name="P50 (µs)" />
            <Line type="monotone" dataKey="p95" stroke={C.yellow} strokeWidth={2} dot={false} name="P95 (µs)" />
            <Line type="monotone" dataKey="p99" stroke={C.red} strokeWidth={2} dot={false} name="P99 (µs)" />
          </LineChart>
        </ChartShell>

        <ChartShell title="Storage Growth">
          <AreaChart data={data.storageGrowth}>
            <defs>
              <linearGradient id="storGrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={C.purple} stopOpacity={0.35} />
                <stop offset="100%" stopColor={C.purple} stopOpacity={0} />
              </linearGradient>
            </defs>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="day" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={2} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Area type="monotone" dataKey="sizeMB" stroke={C.purple} fill="url(#storGrad)" strokeWidth={2} dot={false} name="Storage (MB)" />
          </AreaChart>
        </ChartShell>
      </div>
    </div>
  );
}

/* ============================== DATA EXPLORER PAGE ============================== */
function DataExplorerPage({ data }) {
  const [search, setSearch] = useState("");
  const [selected, setSelected] = useState(null);
  const [rows, setRows] = useState(data.keysList);

  const filtered = rows.filter((r) => r.key.toLowerCase().includes(search.toLowerCase()));

  return (
    <div style={{ display: "flex", gap: 12, height: "100%" }}>
      <div style={{ flex: 1, minWidth: 0 }}>
        <PageHeader title="Data Explorer" subtitle={`${fmtNum(rows.length)} keys shown \u00b7 184,302 total in keyspace`} />

        <div style={{ display: "flex", gap: 10, marginBottom: 12 }}>
          <div style={{ flex: 1, display: "flex", alignItems: "center", gap: 8, background: C.surface, border: `1px solid ${C.border}`, borderRadius: 7, padding: "8px 12px" }}>
            <Search size={14} color={C.textFaint} />
            <input
              value={search}
              onChange={(e) => setSearch(e.target.value)}
              placeholder="Search keys..."
              style={{ ...mono, fontSize: 13, background: "transparent", border: "none", outline: "none", color: C.text, width: "100%" }}
            />
          </div>
          <button style={btnPrimary}><Plus size={14} /> Add Document</button>
          <button style={btnGhost} onClick={() => setRows([...data.keysList])}><RefreshCw size={14} /> Refresh</button>
        </div>

        <Card padded={false}>
          <div style={{ maxHeight: 560, overflowY: "auto" }}>
            <table style={{ width: "100%", borderCollapse: "collapse" }}>
              <thead>
                <tr style={{ position: "sticky", top: 0, background: C.surface2, zIndex: 1 }}>
                  {["KEY", "VALUE", "TYPE", "SIZE", "LAST MODIFIED"].map((h) => (
                    <th key={h} style={{ ...sans, textAlign: "left", fontSize: 10.5, color: C.textFaint, fontWeight: 600, letterSpacing: 0.5, padding: "10px 14px", borderBottom: `1px solid ${C.border}` }}>{h}</th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {filtered.map((row) => (
                  <tr
                    key={row.key}
                    onClick={() => setSelected(row)}
                    style={{ cursor: "pointer", background: selected?.key === row.key ? C.surface3 : "transparent" }}
                    onMouseEnter={(e) => { if (selected?.key !== row.key) e.currentTarget.style.background = C.surface2; }}
                    onMouseLeave={(e) => { if (selected?.key !== row.key) e.currentTarget.style.background = "transparent"; }}
                  >
                    <td style={{ ...mono, fontSize: 12.5, color: C.cyan, padding: "9px 14px", borderBottom: `1px solid ${C.border}`, whiteSpace: "nowrap" }}>{row.key}</td>
                    <td style={{ ...mono, fontSize: 12.5, color: C.text, padding: "9px 14px", borderBottom: `1px solid ${C.border}`, maxWidth: 240, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{row.value}</td>
                    <td style={{ padding: "9px 14px", borderBottom: `1px solid ${C.border}` }}>
                      <span style={{ ...mono, fontSize: 10.5, color: C.textDim, background: C.surface3, padding: "2px 7px", borderRadius: 4 }}>{row.type}</span>
                    </td>
                    <td style={{ ...mono, fontSize: 12, color: C.textDim, padding: "9px 14px", borderBottom: `1px solid ${C.border}` }}>{row.size}</td>
                    <td style={{ ...mono, fontSize: 12, color: C.textFaint, padding: "9px 14px", borderBottom: `1px solid ${C.border}` }}>{row.modified}</td>
                  </tr>
                ))}
                {filtered.length === 0 && (
                  <tr><td colSpan={5} style={{ padding: 40, textAlign: "center", ...sans, fontSize: 13, color: C.textFaint }}>No keys match "{search}"</td></tr>
                )}
              </tbody>
            </table>
          </div>
        </Card>
      </div>

      {selected && (
        <div style={{ width: 320, minWidth: 320 }}>
          <Card>
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 14 }}>
              <span style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text }}>Document Details</span>
              <button onClick={() => setSelected(null)} style={{ background: "none", border: "none", cursor: "pointer" }}>
                <X size={15} color={C.textFaint} />
              </button>
            </div>

            <DetailRow label="Key" value={selected.key} mono />
            <DetailRow label="Value" value={selected.value} mono wrap />
            <DetailRow label="Type" value={selected.type} />
            <DetailRow label="Size" value={selected.size} />
            <DetailRow label="Last Modified" value={selected.modified} />
            <DetailRow label="Storage Location" value={selected.location} mono />

            <div style={{ display: "flex", gap: 8, marginTop: 16 }}>
              <button style={{ ...btnGhost, flex: 1, justifyContent: "center" }}><Pencil size={13} /> Edit</button>
              <button style={{ ...btnDanger, flex: 1, justifyContent: "center" }}><Trash2 size={13} /> Delete</button>
            </div>
          </Card>
        </div>
      )}
    </div>
  );
}

function DetailRow({ label, value, mono: isMono, wrap }) {
  return (
    <div style={{ marginBottom: 12 }}>
      <div style={{ ...sans, fontSize: 10.5, color: C.textFaint, textTransform: "uppercase", letterSpacing: 0.4, marginBottom: 3 }}>{label}</div>
      <div style={{ ...(isMono ? mono2 : sans), fontSize: 12.5, color: C.text, wordBreak: wrap ? "break-word" : "normal", background: C.surface2, border: `1px solid ${C.border}`, borderRadius: 5, padding: "6px 9px" }}>{value}</div>
    </div>
  );
}
const mono2 = { fontFamily: "'IBM Plex Mono','JetBrains Mono',ui-monospace,monospace" };

const btnPrimary = { ...sans, display: "flex", alignItems: "center", gap: 6, fontSize: 12.5, fontWeight: 600, color: "#081020", background: C.accent, border: "none", borderRadius: 7, padding: "8px 14px", cursor: "pointer" };
const btnGhost = { ...sans, display: "flex", alignItems: "center", gap: 6, fontSize: 12.5, fontWeight: 500, color: C.textDim, background: C.surface2, border: `1px solid ${C.border}`, borderRadius: 7, padding: "8px 14px", cursor: "pointer" };
const btnDanger = { ...sans, display: "flex", alignItems: "center", gap: 6, fontSize: 12.5, fontWeight: 500, color: C.red, background: C.red + "15", border: `1px solid ${C.red}44`, borderRadius: 7, padding: "8px 14px", cursor: "pointer" };

/* ============================== QUERY CONSOLE PAGE ============================== */
function QueryConsolePage() {
  const [history, setHistory] = useState([]);
  const [input, setInput] = useState("");
  const [loading, setLoading] = useState(false);

  const bottomRef = useRef(null);
  const inputRef = useRef(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({
      behavior: "smooth",
    });
  }, [history]);


  async function execute() {

    const command = input.trim();

    if (!command || loading) return;


    setLoading(true);


    const parts = command.split(/\s+/);

    const operation = parts[0].toUpperCase();

    let out = "";
    let ok = true;


    try {

      // =========================
      // PUT
      // PUT key value
      // =========================

      if (operation === "PUT") {

        if (parts.length < 3) {

          throw new Error(
            "ERR wrong arguments for 'PUT'"
          );
        }


        const key = parts[1];


        // Allows values with spaces
        const value = parts
          .slice(2)
          .join(" ");


        const response = await fetch(
          "http://localhost:8080/api/put",
          {
            method: "POST",

            headers: {
              "Content-Type": "application/json",
            },

            body: JSON.stringify({
              key,
              value,
            }),
          }
        );


        if (!response.ok) {

          const text = await response.text();

          throw new Error(
            text || "PUT request failed"
          );
        }


        const data = await response.json();

        out = data.status === "OK"
          ? "OK"
          : JSON.stringify(data);
      }


      // =========================
      // GET
      // GET key
      // =========================

      else if (operation === "GET") {

        if (parts.length !== 2) {

          throw new Error(
            "ERR wrong arguments for 'GET'"
          );
        }


        const key = parts[1];


        const response = await fetch(
          `http://localhost:8080/api/get?key=${encodeURIComponent(key)}`
        );


        if (!response.ok) {

          const text = await response.text();

          throw new Error(
            text || "GET request failed"
          );
        }


        const data = await response.json();


        // If backend returns empty value
        // show Redis-style (nil)

        out =
          data.value === ""
            ? "(nil)"
            : data.value;
      }


      // =========================
      // DELETE
      // DELETE key
      // =========================

      else if (operation === "DELETE") {

        if (parts.length !== 2) {

          throw new Error(
            "ERR wrong arguments for 'DELETE'"
          );
        }


        const key = parts[1];


        const response = await fetch(
          "http://localhost:8080/api/delete",
          {
            method: "DELETE",

            headers: {
              "Content-Type": "application/json",
            },

            body: JSON.stringify({
              key,
            }),
          }
        );


        if (!response.ok) {

          const text = await response.text();

          throw new Error(
            text || "DELETE request failed"
          );
        }


        const data = await response.json();


        out = data.status === "OK"
          ? "OK"
          : JSON.stringify(data);
      }


      // =========================
      // UNKNOWN COMMAND
      // =========================

      else {

        throw new Error(
          `ERR unknown command '${operation}'`
        );
      }

    }


    catch (error) {

      ok = false;

      out = error.message;


      // If backend is not running

      if (
        error instanceof TypeError &&
        error.message.includes("fetch")
      ) {

        out =
          "ERR Cannot connect to MDDB backend. Is the server running on port 8080?";
      }
    }


    // =========================
    // ADD RESULT TO CONSOLE
    // =========================

    setHistory((previous) => [

      ...previous,

      {
        cmd: command,
        out,
        ok,
      },

    ]);


    setInput("");

    setLoading(false);

    inputRef.current?.focus();
  }


  return (

    <div>

      <PageHeader

        title="Query Console"

        subtitle="Interactive MDDB command interface"

        right={

          <div style={{ display: "flex", gap: 8 }}>

            <button
              style={btnGhost}
              onClick={() => setHistory([])}
            >

              <Trash2 size={13} />

              Clear Console

            </button>

          </div>

        }

      />


      <Card
        padded={false}
        style={{ overflow: "hidden" }}
      >


        {/* =========================
            TERMINAL HEADER
        ========================= */}

        <div

          style={{

            display: "flex",

            alignItems: "center",

            gap: 8,

            padding: "9px 14px",

            borderBottom: `1px solid ${C.border}`,

            background: C.surface2,

          }}

        >

          <span
            style={{
              width: 10,
              height: 10,
              borderRadius: 999,
              background: C.red,
            }}
          />

          <span
            style={{
              width: 10,
              height: 10,
              borderRadius: 999,
              background: C.yellow,
            }}
          />

          <span
            style={{
              width: 10,
              height: 10,
              borderRadius: 999,
              background: C.green,
            }}
          />


          <span

            style={{

              ...mono,

              fontSize: 11.5,

              color: C.textFaint,

              marginLeft: 8,

            }}

          >

            mddb-console — connected to localhost:8080

          </span>

        </div>



        {/* =========================
            TERMINAL BODY
        ========================= */}

        <div

          style={{

            padding: 16,

            height: 420,

            overflowY: "auto",

            background: "#080B11",

          }}

          onClick={() =>
            inputRef.current?.focus()
          }

        >


          {history.map((h, i) => (

            <div
              key={i}
              style={{ marginBottom: 10 }}
            >

              <div

                style={{

                  ...mono,

                  fontSize: 13,

                  color: C.cyan,

                }}

              >

                <span style={{ color: C.green }}>

                  MDDB &gt;{" "}

                </span>

                {h.cmd}

              </div>


              <div

                style={{

                  ...mono,

                  fontSize: 13,

                  color:
                    h.ok
                      ? C.textDim
                      : C.red,

                  marginTop: 2,

                  paddingLeft: 4,

                }}

              >

                {h.out}

              </div>

            </div>

          ))}



          {/* =========================
              INPUT
          ========================= */}

          <div

            style={{

              display: "flex",

              alignItems: "center",

              ...mono,

              fontSize: 13,

            }}

          >

            <span style={{ color: C.green }}>

              MDDB &gt;

            </span>


            <input

              ref={inputRef}

              autoFocus

              disabled={loading}

              value={input}

              onChange={(e) =>
                setInput(e.target.value)
              }

              onKeyDown={(e) => {

                if (e.key === "Enter") {

                  execute();

                }

              }}

              style={{

                background: "transparent",

                border: "none",

                outline: "none",

                color: C.text,

                marginLeft: 6,

                flex: 1,

                ...mono,

                fontSize: 13,

              }}

              placeholder={
                loading
                  ? "Executing..."
                  : "PUT key value | GET key | DELETE key"
              }

            />

          </div>


          <div ref={bottomRef} />

        </div>



        {/* =========================
            TERMINAL FOOTER
        ========================= */}

        <div

          style={{

            display: "flex",

            justifyContent: "space-between",

            alignItems: "center",

            padding: "10px 14px",

            borderTop: `1px solid ${C.border}`,

            background: C.surface2,

          }}

        >

          <span

            style={{

              ...sans,

              fontSize: 11,

              color: C.textFaint,

            }}

          >

            Connected commands · {history.length} commands run this session

          </span>


          <button

            style={{

              ...btnPrimary,

              opacity: loading ? 0.6 : 1,

            }}

            disabled={loading}

            onClick={execute}

          >

            <Play size={13} />

            {loading
              ? "Executing..."
              : "Execute"}

          </button>

        </div>

      </Card>

    </div>

  );
}
/* ============================== STORAGE ENGINE PAGE ============================== */
function PipelineStage({ label, sub, active }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
      <div style={{
        padding: "10px 18px", borderRadius: 7, background: active ? C.accentDim : C.surface2,
        border: `1px solid ${active ? C.accent + "88" : C.border}`, minWidth: 170, textAlign: "center",
      }}>
        <div style={{ ...mono, fontSize: 12.5, fontWeight: 600, color: active ? C.accent : C.text }}>{label}</div>
        {sub && <div style={{ ...sans, fontSize: 10.5, color: C.textFaint, marginTop: 2 }}>{sub}</div>}
      </div>
    </div>
  );
}

function StorageEnginePage() {
  const stages = [
    ["CLIENT REQUEST", null],
    ["WAL", "append-only log"],
    ["ACTIVE MEMTABLE", "in-memory, sorted"],
    ["IMMUTABLE MEMTABLE", "rotated, read-only"],
    ["ASYNC FLUSH", "background thread"],
    ["SSTABLE", "persisted on disk"],
  ];
  return (
    <div>
      <PageHeader title="Storage Engine" subtitle="Live internal architecture of the MDDB write path" />

      <Card style={{ marginBottom: 14 }}>
        <div style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text, marginBottom: 16 }}>Write Path Pipeline</div>
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
          {stages.map((s, i) => (
            <React.Fragment key={s[0]}>
              <PipelineStage label={s[0]} sub={s[1]} active={i === 2} />
              {i < stages.length - 1 && <ArrowDown size={16} color={C.textFaint} />}
            </React.Fragment>
          ))}
        </div>
      </Card>

      <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 12 }}>
        <StatBlock title="Write-Ahead Log" icon={FileText} accent={C.yellow}>
          <StatLine label="File size" value="42.8 MB" />
          <StatLine label="Total operations" value="100,000" />
          <StatLine label="Recovery status" value="READY" valueColor={C.green} />
        </StatBlock>

        <StatBlock title="MemTable" icon={Layers} accent={C.cyan}>
          <StatLine label="Current entries" value="6,214" />
          <StatLine label="Maximum entries" value="10,000" />
          <StatLine label="Memory usage" value="18.4 MB" />
          <div style={{ marginTop: 6 }}>
            <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 4 }}>
              <span style={{ ...sans, fontSize: 10.5, color: C.textFaint }}>Fill</span>
              <span style={{ ...mono, fontSize: 10.5, color: C.text }}>62%</span>
            </div>
            <div style={{ height: 5, borderRadius: 3, background: C.surface3 }}>
              <div style={{ width: "62%", height: "100%", borderRadius: 3, background: C.cyan }} />
            </div>
          </div>
        </StatBlock>

        <StatBlock title="SSTables" icon={HardDrive} accent={C.purple}>
          <StatLine label="Number of SSTables" value="18" />
          <StatLine label="Total disk size" value="2.34 GB" />
          <StatLine label="Bloom filters" value="Active" valueColor={C.green} />
          <StatLine label="Indexes" value="Active" valueColor={C.green} />
        </StatBlock>

        <StatBlock title="Compaction" icon={RefreshCw} accent={C.accent}>
          <StatLine label="Status" value="Idle" valueColor={C.textDim} />
          <StatLine label="Completed" value="127" />
          <StatLine label="Pending jobs" value="0" />
        </StatBlock>
      </div>
    </div>
  );
}

function StatBlock({ title, icon: Icon, accent, children }) {
  return (
    <Card>
      <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 12 }}>
        <div style={{ width: 24, height: 24, borderRadius: 6, background: accent + "1F", display: "flex", alignItems: "center", justifyContent: "center" }}>
          <Icon size={13} color={accent} />
        </div>
        <span style={{ ...sans, fontSize: 12.5, fontWeight: 600, color: C.text }}>{title}</span>
      </div>
      {children}
    </Card>
  );
}
function StatLine({ label, value, valueColor }) {
  return (
    <div style={{ display: "flex", justifyContent: "space-between", padding: "5px 0", borderBottom: `1px solid ${C.border}` }}>
      <span style={{ ...sans, fontSize: 11.5, color: C.textDim }}>{label}</span>
      <span style={{ ...mono, fontSize: 12, color: valueColor || C.text, fontWeight: 500 }}>{value}</span>
    </div>
  );
}

/* ============================== PERFORMANCE PAGE ============================== */
function PerfOpCard({ op, color, throughput, avg, p50, p95, p99 }) {
  return (
    <Card>
      <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 12 }}>
        <span style={{ width: 8, height: 8, borderRadius: 999, background: color }} />
        <span style={{ ...sans, fontSize: 13, fontWeight: 700, color: C.text, letterSpacing: 0.3 }}>{op}</span>
      </div>
      <div style={{ ...mono, fontSize: 22, fontWeight: 600, color: C.text }}>{throughput}<span style={{ fontSize: 12, color: C.textFaint, fontWeight: 400 }}> ops/sec</span></div>
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 8, marginTop: 14 }}>
        <StatLine label="Average" value={avg} />
        <StatLine label="P50" value={p50} />
        <StatLine label="P95" value={p95} />
        <StatLine label="P99" value={p99} />
      </div>
    </Card>
  );
}

function PerformancePage({ data }) {
  return (
    <div>
      <PageHeader title="Performance" subtitle="Live latency and throughput monitoring across operation types" />
      <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12, marginBottom: 14 }}>
        <PerfOpCard op="PUT" color={C.accent} throughput="28,000" avg="35 µs" p50="29 µs" p95="57 µs" p99="100 µs" />
        <PerfOpCard op="GET" color={C.cyan} throughput="14,700" avg="67 µs" p50="57 µs" p95="106 µs" p99="146 µs" />
        <PerfOpCard op="DELETE" color={C.red} throughput="27,000" avg="36 µs" p50="28 µs" p95="56 µs" p99="106 µs" />
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12, marginBottom: 12 }}>
        <ChartShell title="Throughput Over Time">
          <LineChart data={data.opsOverTime}>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={5} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Legend wrapperStyle={{ fontSize: 11, ...sans, color: C.textDim }} />
            <Line type="monotone" dataKey="put" stroke={C.accent} strokeWidth={2} dot={false} name="PUT" />
            <Line type="monotone" dataKey="get" stroke={C.cyan} strokeWidth={2} dot={false} name="GET" />
            <Line type="monotone" dataKey="del" stroke={C.red} strokeWidth={2} dot={false} name="DELETE" />
          </LineChart>
        </ChartShell>

        <ChartShell title="P99 Latency Trend">
          <AreaChart data={data.latencyTrend}>
            <defs>
              <linearGradient id="p99Grad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor={C.red} stopOpacity={0.3} />
                <stop offset="100%" stopColor={C.red} stopOpacity={0} />
              </linearGradient>
            </defs>
            <CartesianGrid stroke={C.border} vertical={false} />
            <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} interval={5} />
            <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
            <Tooltip contentStyle={tooltipStyle} />
            <Area type="monotone" dataKey="p99" stroke={C.red} fill="url(#p99Grad)" strokeWidth={2} dot={false} name="P99 (µs)" />
          </AreaChart>
        </ChartShell>
      </div>

      <ChartShell title="Latency Distribution (P50 / P95 / P99)" height={240}>
        <BarChart data={data.latencyTrend.filter((_, i) => i % 3 === 0)}>
          <CartesianGrid stroke={C.border} vertical={false} />
          <XAxis dataKey="time" tick={axisStyle} axisLine={{ stroke: C.border }} tickLine={false} />
          <YAxis tick={axisStyle} axisLine={false} tickLine={false} width={40} />
          <Tooltip contentStyle={tooltipStyle} />
          <Legend wrapperStyle={{ fontSize: 11, ...sans, color: C.textDim }} />
          <Bar dataKey="p50" fill={C.green} name="P50" radius={[2, 2, 0, 0]} />
          <Bar dataKey="p95" fill={C.yellow} name="P95" radius={[2, 2, 0, 0]} />
          <Bar dataKey="p99" fill={C.red} name="P99" radius={[2, 2, 0, 0]} />
        </BarChart>
      </ChartShell>
    </div>
  );
}

/* ============================== BENCHMARK PAGE ============================== */
function BenchmarkPage() {
  const [ops, setOps] = useState(100000);
  const [memtableSize, setMemtableSize] = useState(1000);
  const [workload, setWorkload] = useState({ put: true, get: true, del: true });
  const [running, setRunning] = useState(false);
  const [result, setResult] = useState({
    total: 100000, execMs: 3571, throughput: 28000, avg: "35 µs", p50: "29 µs", p95: "57 µs", p99: "100 µs",
  });

  function runBenchmark() {
    setRunning(true);
    setTimeout(() => {
      const tp = Math.round(24000 + Math.random() * 8000);
      setResult({
        total: ops,
        execMs: Math.round((ops / tp) * 1000),
        throughput: tp,
        avg: (30 + Math.random() * 12).toFixed(0) + " µs",
        p50: (24 + Math.random() * 10).toFixed(0) + " µs",
        p95: (48 + Math.random() * 16).toFixed(0) + " µs",
        p99: (90 + Math.random() * 24).toFixed(0) + " µs",
      });
      setRunning(false);
    }, 1400);
  }

  const chartData = [
    { name: "PUT", throughput: 28000 },
    { name: "GET", throughput: 14700 },
    { name: "DELETE", throughput: 27000 },
  ];

  return (
    <div>
      <PageHeader title="Benchmark" subtitle="Configure and run synthetic workloads against MDDB" />
      <div style={{ display: "grid", gridTemplateColumns: "300px 1fr", gap: 12 }}>
        <Card>
          <div style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text, marginBottom: 14 }}>Configuration</div>

          <FieldLabel label="Operations" />
          <NumberInput value={ops} onChange={setOps} />

          <FieldLabel label="MemTable Size" />
          <NumberInput value={memtableSize} onChange={setMemtableSize} />

          <FieldLabel label="Workload" />
          {["put", "get", "del"].map((w) => (
            <label key={w} style={{ display: "flex", alignItems: "center", gap: 8, padding: "6px 0", cursor: "pointer" }}>
              <input type="checkbox" checked={workload[w]} onChange={() => setWorkload({ ...workload, [w]: !workload[w] })} style={{ accentColor: C.accent }} />
              <span style={{ ...mono, fontSize: 12.5, color: C.text, textTransform: "uppercase" }}>{w === "del" ? "DELETE" : w}</span>
            </label>
          ))}

          <button onClick={runBenchmark} disabled={running} style={{ ...btnPrimary, width: "100%", justifyContent: "center", marginTop: 16, opacity: running ? 0.6 : 1 }}>
            <Play size={14} /> {running ? "Running..." : "Run Benchmark"}
          </button>
        </Card>

        <div>
          <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12, marginBottom: 12 }}>
            <MetricCard label="Total Operations" value={fmtNum(result.total)} icon={Activity} />
            <MetricCard label="Execution Time" value={result.execMs} unit="ms" icon={Clock} accent={C.cyan} />
            <MetricCard label="Throughput" value={fmtNum(result.throughput)} unit="ops/sec" icon={Zap} accent={C.accent} />
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 12, marginBottom: 12 }}>
            <MetricCard label="Average Latency" value={result.avg} icon={Gauge} accent={C.green} />
            <MetricCard label="P50" value={result.p50} icon={Timer} accent={C.green} />
            <MetricCard label="P95" value={result.p95} icon={Timer} accent={C.yellow} />
            <MetricCard label="P99" value={result.p99} icon={Timer} accent={C.red} />
          </div>
          <ChartShell title="Throughput by Operation" height={220}>
            <BarChart data={chartData} layout="vertical">
              <CartesianGrid stroke={C.border} horizontal={false} />
              <XAxis type="number" tick={axisStyle} axisLine={false} tickLine={false} />
              <YAxis type="category" dataKey="name" tick={axisStyle} axisLine={false} tickLine={false} width={60} />
              <Tooltip contentStyle={tooltipStyle} />
              <Bar dataKey="throughput" fill={C.accent} radius={[0, 3, 3, 0]} />
            </BarChart>
          </ChartShell>
        </div>
      </div>
    </div>
  );
}

function FieldLabel({ label }) {
  return <div style={{ ...sans, fontSize: 10.5, color: C.textFaint, textTransform: "uppercase", letterSpacing: 0.4, marginTop: 12, marginBottom: 6 }}>{label}</div>;
}
function NumberInput({ value, onChange }) {
  return (
    <input
      type="number"
      value={value}
      onChange={(e) => onChange(Number(e.target.value))}
      style={{ ...mono, width: "100%", background: C.surface2, border: `1px solid ${C.border}`, borderRadius: 6, padding: "8px 10px", color: C.text, fontSize: 13, outline: "none" }}
    />
  );
}

/* ============================== WAL & RECOVERY PAGE ============================== */
function WalRecoveryPage() {
  const steps = ["Application Crash", "Read WAL", "Replay Operations", "Restore MemTable", "Database Online"];
  return (
    <div>
      <PageHeader title="WAL & Recovery" subtitle="Write-ahead log health and crash recovery simulation" />
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 12, marginBottom: 14 }}>
        <Card>
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", marginBottom: 14 }}>
            <span style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text }}>WAL Status</span>
            <span style={{ display: "flex", alignItems: "center", gap: 6, background: C.green + "1A", border: `1px solid ${C.green}44`, borderRadius: 5, padding: "3px 9px" }}>
              <CheckCircle2 size={12} color={C.green} />
              <span style={{ ...sans, fontSize: 11, fontWeight: 600, color: C.green }}>HEALTHY</span>
            </span>
          </div>
          <StatLine label="WAL File" value="benchmark_data/wal.log" />
          <StatLine label="Operations logged" value="100,000" />
          <StatLine label="Recovery status" value="READY" valueColor={C.green} />
          <StatLine label="Last checkpoint" value="2 min ago" />
        </Card>

        <Card>
          <div style={{ ...sans, fontSize: 13, fontWeight: 600, color: C.text, marginBottom: 14 }}>Recovery Simulation</div>
          <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
            {steps.map((s, i) => (
              <React.Fragment key={s}>
                <div style={{ display: "flex", alignItems: "center", gap: 10, padding: "8px 10px", borderRadius: 6, background: C.surface2, border: `1px solid ${C.border}` }}>
                  <span style={{ width: 18, height: 18, borderRadius: 999, background: C.accentDim, display: "flex", alignItems: "center", justifyContent: "center", ...mono, fontSize: 10, color: C.accent, fontWeight: 700 }}>{i + 1}</span>
                  <span style={{ ...mono, fontSize: 12, color: C.text }}>{s}</span>
                </div>
                {i < steps.length - 1 && <div style={{ alignSelf: "center" }}><ArrowDown size={13} color={C.textFaint} /></div>}
              </React.Fragment>
            ))}
          </div>
        </Card>
      </div>
    </div>
  );
}

/* ============================== SETTINGS PAGE ============================== */
function SettingsPage() {
  const rows = [
    ["Data directory", "./benchmark_data"],
    ["MemTable max entries", "10,000"],
    ["Immutable MemTable queue size", "2"],
    ["Bloom filter false-positive rate", "1%"],
    ["Compaction trigger", "4 SSTables"],
    ["WAL sync policy", "fsync per batch"],
    ["API port", "8080"],
  ];
  return (
    <div>
      <PageHeader title="Settings" subtitle="Engine configuration (read-only preview)" />
      <Card padded={false}>
        {rows.map(([label, value], i) => (
          <div key={label} style={{ display: "flex", justifyContent: "space-between", padding: "13px 16px", borderBottom: i < rows.length - 1 ? `1px solid ${C.border}` : "none" }}>
            <span style={{ ...sans, fontSize: 13, color: C.textDim }}>{label}</span>
            <span style={{ ...mono, fontSize: 12.5, color: C.text }}>{value}</span>
          </div>
        ))}
      </Card>
    </div>
  );
}

/* ============================== APP ============================== */
export default function App() {
  const [page, setPage] = useState("dashboard");
  const data = useMockData();

  const pages = {
    dashboard: <DashboardPage data={data} />,
    explorer: <DataExplorerPage data={data} />,
    console: <QueryConsolePage />,
    storage: <StorageEnginePage />,
    performance: <PerformancePage data={data} />,
    benchmark: <BenchmarkPage />,
    wal: <WalRecoveryPage />,
    settings: <SettingsPage />,
  };

  return (
    <div style={{ display: "flex", height: "100vh", background: C.bg, ...sans }}>
      <style>{`
        * { box-sizing: border-box; }
        input:focus { outline: none; }
        ::-webkit-scrollbar { width: 8px; height: 8px; }
        ::-webkit-scrollbar-track { background: transparent; }
        ::-webkit-scrollbar-thumb { background: ${C.borderLight}; border-radius: 4px; }
      `}</style>
      <Sidebar page={page} setPage={setPage} />
      <div style={{ flex: 1, overflowY: "auto", padding: 24 }}>
        {pages[page]}
      </div>
    </div>
  );
}
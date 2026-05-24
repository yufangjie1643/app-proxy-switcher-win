#pragma once
#include <string>

inline std::wstring BuildWebViewHtml() {
    return std::wstring(LR"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Codex 代理启动器</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #f4f7fb;
      --panel: #ffffff;
      --panel-soft: #f8fafc;
      --text: #162033;
      --muted: #5e6b82;
      --line: #d7dfeb;
      --primary: #146ef5;
      --primary-strong: #0d58c7;
      --ok: #12845a;
      --warn: #b86b00;
      --danger: #a53b34;
      --shadow: 0 14px 34px rgba(24, 39, 66, 0.12);
      font-family: "Segoe UI Variable", "Segoe UI", "Microsoft YaHei UI", sans-serif;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      color: var(--text);
      background: var(--bg);
    }

    .app {
      min-height: 100vh;
      padding: 24px;
      display: grid;
      grid-template-rows: auto auto 1fr;
      gap: 14px;
    }

    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 18px;
    }

    .title {
      display: flex;
      align-items: center;
      gap: 13px;
    }

    .mark {
      width: 40px;
      height: 40px;
      border-radius: 8px;
      display: grid;
      place-items: center;
      background: #146ef5;
      color: #fff;
      font-weight: 700;
      letter-spacing: 0;
      box-shadow: 0 10px 24px rgba(20, 110, 245, 0.22);
    }

    h1 {
      margin: 0;
      font-size: 22px;
      line-height: 1.2;
      font-weight: 650;
      letter-spacing: 0;
    }

    .subtitle {
      margin-top: 3px;
      color: var(--muted);
      font-size: 13px;
    }

    .mode-pill {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: #fff;
      color: var(--muted);
      font-size: 13px;
      white-space: nowrap;
    }

    .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--ok);
    }

    main {
      display: grid;
      grid-template-columns: minmax(0, 1fr) 240px;
      gap: 14px;
      align-items: stretch;
    }

    .panel {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 8px;
      box-shadow: var(--shadow);
      overflow: hidden;
    }

    .system-warning {
      display: none;
      padding: 11px 13px;
      border: 1px solid #f1c987;
      border-radius: 8px;
      background: #fff8eb;
      color: #7d4b00;
      font-size: 13px;
      line-height: 1.45;
    }

    .system-warning.show {
      display: block;
    }

    .status {
      padding: 18px;
      display: grid;
      gap: 14px;
    }

    .guide {
      padding: 14px;
      border: 1px solid #cfe0fa;
      border-radius: 8px;
      background: #f2f7ff;
      display: grid;
      gap: 10px;
    }

    .guide-title {
      font-weight: 700;
      font-size: 15px;
    }

    .steps {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 8px;
    }

    .step {
      padding: 10px;
      min-height: 62px;
      border-radius: 8px;
      background: #fff;
      border: 1px solid #d8e6fb;
      font-size: 13px;
      line-height: 1.35;
    }

    .row {
      display: grid;
      gap: 6px;
    }

    .label {
      color: var(--muted);
      font-size: 12px;
      font-weight: 600;
      letter-spacing: 0;
      text-transform: uppercase;
    }

    .value {
      font-size: 16px;
      line-height: 1.42;
      overflow-wrap: anywhere;
      word-break: break-word;
    }

    .path {
      padding: 13px 14px;
      border-radius: 8px;
      background: var(--panel-soft);
      border: 1px solid var(--line);
      font-family: "Cascadia Mono", Consolas, monospace;
      font-size: 13px;
      line-height: 1.45;
    }

    .actions {
      padding: 12px;
      display: grid;
      gap: 9px;
      background: var(--panel);
      align-content: start;
    }

    button {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 8px;
      padding: 11px 12px;
      min-height: 46px;
      background: #fff;
      color: var(--text);
      font: inherit;
      font-size: 14px;
      font-weight: 600;
      text-align: center;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: transform 120ms ease, border-color 120ms ease, background 120ms ease, box-shadow 120ms ease;
    }

    button:hover {
      transform: translateY(-1px);
      border-color: rgba(20, 110, 245, 0.36);
      box-shadow: 0 10px 24px rgba(27, 42, 71, 0.10);
    }

    button:active {
      transform: translateY(0);
    }

    .primary {
      border-color: transparent;
      background: linear-gradient(135deg, var(--primary), var(--primary-strong));
      color: #fff;
      box-shadow: 0 16px 30px rgba(20, 110, 245, 0.22);
    }

    .secondary {
      background: #f8fafc;
    }

    .quiet {
      min-height: 40px;
      font-size: 13px;
      background: #fff;
    }

    .danger {
      color: var(--danger);
      background: #fff8f7;
      border-color: #f0c9c6;
    }

    .toast {
      position: fixed;
      left: 28px;
      right: 28px;
      bottom: 22px;
      padding: 12px 14px;
      border-radius: 8px;
      color: #fff;
      background: rgba(23, 32, 51, 0.92);
      box-shadow: 0 18px 40px rgba(0, 0, 0, 0.20);
      opacity: 0;
      transform: translateY(8px);
      transition: opacity 150ms ease, transform 150ms ease;
      pointer-events: none;
      font-size: 13px;
    }

    .toast.show {
      opacity: 1;
      transform: translateY(0);
    }
)HTML") + LR"HTML(

    @media (max-width: 760px) {
      .app { padding: 16px; }
      header { align-items: flex-start; flex-direction: column; }
      main { grid-template-columns: 1fr; }
      .steps { grid-template-columns: 1fr; }
      .actions { grid-template-columns: 1fr 1fr; }
      .actions .primary { grid-column: 1 / -1; }
      .actions .danger { grid-column: 1 / -1; }
    }

    @media (max-width: 420px) {
      .actions { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="app">
    <header>
      <div class="title">
        <div class="mark">CP</div>
        <div>
          <h1>Codex 代理启动器</h1>
          <div class="subtitle">为 AI 编程工具选择直连或代理启动</div>
        </div>
      </div>
      <div class="mode-pill"><span class="dot" id="foundDot"></span><span id="foundText">检测中</span></div>
    </header>

    <div class="system-warning" id="systemWarning">
      系统代理正在影响全局网络。用完请点击右侧“关闭系统代理”，避免影响浏览器和其他程序。
    </div>

    <main>
      <section class="panel status">
        <div class="guide">
          <div class="guide-title">第一次使用，按这 3 步走</div>
          <div class="steps">
            <div class="step">1. 点击“检测代理”，确认本机代理端口可用。</div>
            <div class="step">2. 看“程序路径”，确认已经找到要启动的 Agent。</div>
            <div class="step">3. 普通用户建议点击“代理启动”。</div>
          </div>
        </div>
        <div class="row">
          <div class="label">应用</div>
          <div class="value" id="appName">-</div>
        </div>
        <div class="row">
          <div class="label">代理</div>
          <div class="value" id="proxyUrl">-</div>
        </div>
        <div class="row">
          <div class="label">模式</div>
          <div class="value" id="modeText">-</div>
        </div>
        <div class="row">
          <div class="label">代理检测</div>
          <div class="value" id="proxyCheckText">-</div>
        </div>
        <div class="row">
          <div class="label">最近操作</div>
          <div class="value" id="lastActionText">-</div>
        </div>
        <div class="row">
          <div class="label">程序路径</div>
          <div class="value path" id="pathText">-</div>
        </div>
      </section>

      <aside class="panel actions">
        <button class="primary" data-command="launchProxy">代理启动</button>
        <button data-command="launchNative">原生启动</button>
        <button class="secondary" data-command="checkProxy">检测代理</button>
        <button class="secondary" data-command="rescan">重新扫描</button>
        <button class="secondary" data-command="switchApp">切换应用</button>
        <button class="secondary" data-command="switchMode">切换模式</button>
        <button class="quiet" data-command="configureProxy">配置代理</button>
        <button class="quiet" data-command="openConfigDir">配置目录</button>
        <button class="danger" id="disableProxy" data-command="disableSystemProxy">关闭系统代理</button>
      </aside>
    </main>
  </div>
  <div class="toast" id="toast"></div>

  <script>
    const send = (command) => window.chrome.webview.postMessage({ command });
    const text = (id, value) => { document.getElementById(id).textContent = value || "-"; };
    let toastTimer = 0;

    function showToast(message) {
      const el = document.getElementById("toast");
      el.textContent = message;
      el.classList.add("show");
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => el.classList.remove("show"), 2600);
    }

    function applyStatus(data) {
      text("appName", `${data.appName} [${data.appType}]`);
      text("proxyUrl", data.proxyUrl);
      text("modeText", data.modeText);
      text("proxyCheckText", data.proxyCheckText);
      text("lastActionText", data.lastActionText);
      text("pathText", data.pathText);
      text("foundText", data.found ? "已找到目标程序" : "未找到目标程序");
      document.getElementById("foundDot").style.background = data.found ? "#139a5c" : "#c77900";
      document.getElementById("disableProxy").style.display = data.canDisableSystemProxy ? "block" : "none";
      document.getElementById("systemWarning").classList.toggle("show", !!data.systemProxyActive);
      const proxyBtn = document.querySelector('[data-command="launchProxy"]');
      proxyBtn.textContent = data.proxyButtonText || "代理启动";
    }

    document.querySelectorAll("button[data-command]").forEach((button) => {
      button.addEventListener("click", () => send(button.dataset.command));
    });

    window.chrome.webview.addEventListener("message", (event) => {
      const data = event.data;
      if (!data || !data.type) return;
      if (data.type === "status") applyStatus(data);
      if (data.type === "toast") showToast(data.message);
    });

    send("ready");
  </script>
</body>
</html>
)HTML";
}

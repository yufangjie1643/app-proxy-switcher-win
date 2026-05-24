#pragma once
#include <string>

inline std::wstring BuildWebViewHtml() {
    std::wstring html = LR"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Codex 代理启动器</title>
  <style>
    :root {
      color-scheme: light;
      --bg: #eef3f6;
      --paper: #fbf6ee;
      --paper-deep: #f2eadf;
      --surface: #fffdf8;
      --surface-soft: #f7f1e8;
      --surface-cool: #f2f7f8;
      --text: #171717;
      --muted: #6a6259;
      --faint: #8f867b;
      --line: rgba(39, 31, 22, 0.18);
      --line-strong: rgba(39, 31, 22, 0.32);
      --primary: #315f8d;
      --primary-strong: #24496f;
      --ok: #2e7d67;
      --warn: #d86135;
      --danger: #a33b34;
      --amber-bg: #fff0df;
      --shadow: 0 18px 42px rgba(22, 22, 22, 0.14);
      --shadow-soft: 0 8px 20px rgba(22, 22, 22, 0.08);
      font-family: "Segoe UI Variable", "Segoe UI", "Microsoft YaHei UI", sans-serif;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      color: var(--text);
      background:
        linear-gradient(180deg, #f9fbfc 0, var(--bg) 100%);
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
      min-width: 0;
      padding: 2px 4px 6px;
    }

    .title {
      display: flex;
      align-items: center;
      gap: 14px;
      min-width: 0;
    }

    .mark {
      width: 44px;
      height: 44px;
      border-radius: 0;
      display: grid;
      place-items: center;
      background: var(--text);
      color: #fff;
      font-weight: 800;
      letter-spacing: 0;
      box-shadow: 8px 8px 0 rgba(49, 95, 141, 0.18);
      flex: 0 0 auto;
      position: relative;
    }

    .mark:after {
      content: "";
      position: absolute;
      left: 8px;
      right: 8px;
      bottom: 8px;
      height: 2px;
      background: #d86135;
    }

    h1 {
      margin: 0;
      font-size: 25px;
      line-height: 1.16;
      font-weight: 760;
      letter-spacing: 0;
    }

    .subtitle {
      margin-top: 5px;
      color: var(--muted);
      font-size: 13px;
      line-height: 1.35;
    }

    .top-meta {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }

    .version-badge,
    .mode-pill {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      min-height: 34px;
      padding: 7px 12px;
      border: 1px solid var(--line);
      border-radius: 0;
      background: var(--surface);
      color: var(--muted);
      font-size: 13px;
      white-space: nowrap;
      box-shadow: 0 1px 0 rgba(255, 255, 255, 0.8) inset;
    }

    .version-badge {
      color: #2f5f88;
      background: #edf4f6;
      border-color: rgba(49, 95, 141, 0.28);
    }

    .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--ok);
      box-shadow: 0 0 0 4px rgba(46, 125, 103, 0.13);
    }

    body[data-found="false"] .dot {
      background: var(--warn);
      box-shadow: 0 0 0 4px rgba(183, 106, 0, 0.14);
    }

    .system-warning {
      display: none;
      padding: 12px 14px;
      border: 1px solid #ebc27c;
      border-radius: 0;
      background: var(--amber-bg);
      color: #744700;
      font-size: 13px;
      line-height: 1.45;
      box-shadow: var(--shadow-soft);
    }

    .system-warning.show {
      display: block;
    }

    main {
      display: grid;
      grid-template-columns: minmax(0, 1fr) 270px;
      gap: 16px;
      align-items: start;
      min-width: 0;
    }

    .panel {
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: 0;
      box-shadow: var(--shadow);
      overflow: hidden;
      min-width: 0;
      position: relative;
    }

    .panel:before {
      content: "";
      position: absolute;
      inset: 14px;
      border: 1px solid rgba(39, 31, 22, 0.10);
      pointer-events: none;
    }

    .status {
      padding: 18px;
      display: grid;
      gap: 14px;
      position: relative;
    }

    .guide {
      padding: 14px;
      border: 1px solid rgba(49, 95, 141, 0.25);
      border-left: 4px solid var(--primary);
      border-radius: 0;
      background: var(--surface-cool);
      display: grid;
      gap: 12px;
    }

    .guide-head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }

    .guide-title {
      font-weight: 720;
      font-size: 15px;
    }

    .guide-tag {
      color: #2f5f88;
      font-size: 12px;
      font-weight: 650;
      padding: 4px 8px;
      border-radius: 0;
      background: #ffffff;
      border: 1px solid rgba(49, 95, 141, 0.24);
      white-space: nowrap;
    }

    .steps {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 9px;
    }

    .step {
      padding: 11px;
      min-height: 72px;
      border-radius: 0;
      background: #fff;
      border: 1px solid rgba(49, 95, 141, 0.20);
      display: grid;
      grid-template-columns: 26px minmax(0, 1fr);
      gap: 9px;
      align-items: start;
    }

    .step-index {
      width: 26px;
      height: 26px;
      display: grid;
      place-items: center;
      border-radius: 0;
      background: var(--primary);
      color: #fff;
      font-size: 13px;
      font-weight: 760;
    }

    .step-copy {
      font-size: 13px;
      line-height: 1.42;
      color: #25324a;
    }

    .status-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 10px;
    }

    .row {
      display: grid;
      gap: 6px;
      padding: 12px;
      border: 1px solid var(--line);
      border-radius: 0;
      background: var(--surface-soft);
      min-width: 0;
    }

    .row.full {
      grid-column: 1 / -1;
    }

    .label {
      color: var(--muted);
      font-size: 12px;
      font-weight: 680;
      letter-spacing: 0;
      color: var(--primary);
    }

    .value {
      font-size: 15px;
      line-height: 1.45;
      overflow-wrap: anywhere;
      word-break: break-word;
    }

    .value.strong {
      font-size: 17px;
      font-weight: 660;
    }

    .path {
      padding: 13px 14px;
      border-radius: 0;
      background: #ffffff;
      border: 1px solid var(--line-strong);
      font-family: "Cascadia Mono", Consolas, monospace;
      font-size: 13px;
      line-height: 1.5;
      max-height: 116px;
      overflow: auto;
    }
)HTML";
    html += LR"HTML(

    .actions {
      padding: 14px;
      display: grid;
      gap: 10px;
      align-content: start;
      position: sticky;
      top: 24px;
      background:
        linear-gradient(180deg, #fffdf8 0, #f8f1e8 100%);
    }

    .actions-title {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
      color: var(--muted);
      font-size: 12px;
      font-weight: 720;
      letter-spacing: 0;
      padding: 1px 2px 2px;
    }

    .actions-title span:last-child {
      color: var(--faint);
      font-weight: 600;
    }

    button {
      width: 100%;
      border: 1px solid var(--line);
      border-radius: 0;
      padding: 10px 12px;
      min-height: 46px;
      background: #fff;
      color: var(--text);
      font: inherit;
      text-align: left;
      cursor: pointer;
      display: grid;
      grid-template-columns: 28px minmax(0, 1fr);
      gap: 10px;
      align-items: center;
      transition: transform 120ms ease, border-color 120ms ease, background 120ms ease, box-shadow 120ms ease;
    }

    button:hover {
      transform: translate(-1px, -1px);
      border-color: rgba(49, 95, 141, 0.45);
      box-shadow: 4px 4px 0 rgba(49, 95, 141, 0.13);
    }

    button:focus-visible {
      outline: 3px solid rgba(216, 97, 53, 0.20);
      outline-offset: 2px;
    }

    button:active {
      transform: translateY(0);
    }

    .btn-icon {
      width: 28px;
      height: 28px;
      border-radius: 0;
      display: grid;
      place-items: center;
      background: var(--paper-deep);
      color: var(--primary);
      font-size: 13px;
      font-weight: 780;
      border: 1px solid rgba(39, 31, 22, 0.08);
    }

    .btn-text {
      min-width: 0;
      display: grid;
      gap: 2px;
    }

    .btn-label {
      font-size: 14px;
      font-weight: 720;
      line-height: 1.24;
    }

    .btn-caption {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.25;
    }

    .primary {
      border-color: transparent;
      background: var(--text);
      color: #fff;
      box-shadow: 6px 6px 0 rgba(216, 97, 53, 0.24);
    }

    .primary .btn-icon {
      background: rgba(255, 255, 255, 0.18);
      color: #fff;
    }

    .primary .btn-caption {
      color: rgba(255, 255, 255, 0.78);
    }

    .secondary {
      background: #fbfcfc;
    }

    .quiet {
      min-height: 42px;
      background: var(--surface);
    }

    .danger {
      color: var(--danger);
      background: #fff3ef;
      border-color: #e9b9ad;
    }

    .danger .btn-icon {
      background: #ffebe9;
      color: var(--danger);
    }

    .toast {
      position: fixed;
      left: 28px;
      right: 28px;
      bottom: 22px;
      padding: 12px 14px;
      border-radius: 0;
      color: #fff;
      background: rgba(22, 21, 20, 0.95);
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
)HTML";
    html += LR"HTML(

    @media (max-width: 860px) {
      .app { padding: 16px; }
      header { align-items: flex-start; flex-direction: column; }
      .top-meta { justify-content: flex-start; }
      main { grid-template-columns: 1fr; }
      .actions {
        position: static;
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
      .actions-title,
      .actions .primary,
      .actions .danger {
        grid-column: 1 / -1;
      }
    }

    @media (max-width: 680px) {
      .steps,
      .status-grid {
        grid-template-columns: 1fr;
      }
      .guide-head {
        align-items: flex-start;
        flex-direction: column;
      }
    }

    @media (max-width: 480px) {
      .actions { grid-template-columns: 1fr; }
      h1 { font-size: 21px; }
      button { grid-template-columns: 26px minmax(0, 1fr); }
      .btn-icon {
        width: 26px;
        height: 26px;
      }
    }
  </style>
</head>
<body data-found="false">
  <div class="app">
    <header>
      <div class="title">
        <div class="mark">CP</div>
        <div>
          <h1>Codex 代理启动器</h1>
          <div class="subtitle">为 AI 编程工具选择直连或代理启动</div>
        </div>
      </div>
      <div class="top-meta">
        <div class="version-badge">WebView2 推荐版</div>
        <div class="mode-pill"><span class="dot" id="foundDot"></span><span id="foundText">检测中</span></div>
      </div>
    </header>

    <div class="system-warning" id="systemWarning">
      系统代理正在影响全局网络。用完请点击“关闭系统代理”，避免影响浏览器和其他程序。
    </div>

    <main>
      <section class="panel status">
        <div class="guide">
          <div class="guide-head">
            <div class="guide-title">第一次使用，按这 3 步走</div>
            <div class="guide-tag">推荐流程</div>
          </div>
          <div class="steps">
            <div class="step">
              <div class="step-index">1</div>
              <div class="step-copy">点击“检测代理”，确认本机代理端口可用。</div>
            </div>
            <div class="step">
              <div class="step-index">2</div>
              <div class="step-copy">查看“程序路径”，确认已经找到目标 Agent。</div>
            </div>
            <div class="step">
              <div class="step-index">3</div>
              <div class="step-copy">普通用户建议点击“代理启动”。</div>
            </div>
          </div>
        </div>

        <div class="status-grid">
          <div class="row">
            <div class="label">应用</div>
            <div class="value strong" id="appName">-</div>
          </div>
          <div class="row">
            <div class="label">模式</div>
            <div class="value strong" id="modeText">-</div>
          </div>
          <div class="row">
            <div class="label">代理</div>
            <div class="value" id="proxyUrl">-</div>
          </div>
          <div class="row">
            <div class="label">代理检测</div>
            <div class="value" id="proxyCheckText">-</div>
          </div>
          <div class="row full">
            <div class="label">最近操作</div>
            <div class="value" id="lastActionText">-</div>
          </div>
          <div class="row full">
            <div class="label">程序路径</div>
            <div class="value path" id="pathText">-</div>
          </div>
        </div>
      </section>

      <aside class="panel actions">
        <div class="actions-title"><span>操作</span><span>本机执行</span></div>
        <button class="primary" data-command="launchProxy">
          <span class="btn-icon">▶</span>
          <span class="btn-text"><span class="btn-label">代理启动</span><span class="btn-caption">推荐方式</span></span>
        </button>
        <button data-command="launchNative">
          <span class="btn-icon">↗</span>
          <span class="btn-text"><span class="btn-label">原生启动</span><span class="btn-caption">不注入代理</span></span>
        </button>
        <button class="secondary" data-command="checkProxy">
          <span class="btn-icon">✓</span>
          <span class="btn-text"><span class="btn-label">检测代理</span><span class="btn-caption">扫描本机常见端口</span></span>
        </button>
        <button class="secondary" data-command="rescan">
          <span class="btn-icon">↻</span>
          <span class="btn-text"><span class="btn-label">重新扫描</span><span class="btn-caption">刷新目标程序</span></span>
        </button>
        <button class="secondary" data-command="switchApp">
          <span class="btn-icon">□</span>
          <span class="btn-text"><span class="btn-label">切换应用</span><span class="btn-caption">选择或手动添加</span></span>
        </button>
        <button class="secondary" data-command="switchMode">
          <span class="btn-icon">⇄</span>
          <span class="btn-text"><span class="btn-label">切换模式</span><span class="btn-caption">环境变量 / 系统代理</span></span>
        </button>
        <button class="quiet" data-command="configureProxy">
          <span class="btn-icon">:</span>
          <span class="btn-text"><span class="btn-label">配置代理</span><span class="btn-caption">修改地址和端口</span></span>
        </button>
        <button class="quiet" data-command="openConfigDir">
          <span class="btn-icon">…</span>
          <span class="btn-text"><span class="btn-label">配置目录</span><span class="btn-caption">打开 settings.ini</span></span>
        </button>
        <button class="danger" id="disableProxy" data-command="disableSystemProxy">
          <span class="btn-icon">×</span>
          <span class="btn-text"><span class="btn-label">关闭系统代理</span><span class="btn-caption">恢复全局网络</span></span>
        </button>
      </aside>
    </main>
  </div>
  <div class="toast" id="toast"></div>
)HTML";
    html += LR"HTML(

  <script>
    const send = (command) => window.chrome.webview.postMessage({ command });
    const text = (id, value) => { document.getElementById(id).textContent = value || "-"; };
    let toastTimer = 0;

    function showToast(message) {
      const el = document.getElementById("toast");
      el.textContent = message;
      el.classList.add("show");
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => el.classList.remove("show"), 3000);
    }

    function applyStatus(data) {
      text("appName", `${data.appName} [${data.appType}]`);
      text("proxyUrl", data.proxyUrl);
      text("modeText", data.modeText);
      text("proxyCheckText", data.proxyCheckText);
      text("lastActionText", data.lastActionText);
      text("pathText", data.pathText);
      text("foundText", data.found ? "已找到目标程序" : "未找到目标程序");
      document.body.dataset.found = data.found ? "true" : "false";
      document.getElementById("disableProxy").style.display = data.canDisableSystemProxy ? "grid" : "none";
      document.getElementById("systemWarning").classList.toggle("show", !!data.systemProxyActive);
      const proxyBtn = document.querySelector('[data-command="launchProxy"] .btn-label');
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
    return html;
}

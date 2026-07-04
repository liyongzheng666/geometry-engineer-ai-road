// 周深拆阅读器：documents[week][kind] → fetch 仓库内 md → 渲染 + 目录
const REPO_BLOB = "https://github.com/liyongzheng666/geometry-engineer-ai-road/blob/main/";
const REPO_TREE = "https://github.com/liyongzheng666/geometry-engineer-ai-road/tree/main/";

const weeks = {
  w1: {
    label: "第 1 周 · 验证闭环",
    extraLabel: "补充：CMake 构建",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week01_实操_AI代码验证闭环.md" },
      code:  { label: "代码解读",   path: "site-docs/week01_代码解读.md" },
      extra: { label: "CMake 构建解读", path: "site-docs/week01_补充_CMake构建解读.md" }
    }
  },
  w2: {
    label: "第 2 周 · 退化防御",
    extraLabel: "补充：退化与容差",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week02_实操_退化输入防御.md" },
      code:  { label: "代码解读",   path: "site-docs/week02_代码解读.md" },
      extra: { label: "退化输入与容差", path: "site-docs/week02_补充_退化输入与容差.md" }
    }
  },
  w3: {
    label: "第 3 周 · 上下文注入",
    extraLabel: "补充：Eigen 集成",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week03_实操_上下文注入.md" },
      code:  { label: "代码解读",   path: "site-docs/week03_代码解读.md" },
      extra: { label: "Eigen 集成",  path: "site-docs/week03_补充_Eigen集成.md" }
    }
  },
  w4: {
    label: "第 4 周 · 网格平滑",
    extraLabel: "补充：网格与保特征",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week04_实操_保特征网格平滑.md" },
      code:  { label: "代码解读",   path: "site-docs/week04_代码解读.md" },
      extra: { label: "网格数据结构与保特征", path: "site-docs/week04_补充_网格数据结构与保特征.md" }
    }
  },
  w5: {
    label: "第 5 周 · SymPy 梯度",
    extraLabel: "补充：有限差分验证",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week05_实操_SymPy推导梯度.md" },
      code:  { label: "代码解读",   path: "site-docs/week05_代码解读.md" },
      extra: { label: "有限差分验证", path: "site-docs/week05_补充_有限差分验证.md" }
    }
  },
  w6: {
    label: "第 6 周 · 符号转 C++",
    extraLabel: "补充：CSE 与稀疏组装",
    docs: {
      plan:  { label: "计划拆解",   path: "docs/week06_实操_符号转CPP.md" },
      code:  { label: "代码解读",   path: "site-docs/week06_代码解读.md" },
      extra: { label: "CSE 与 Eigen 稀疏组装", path: "site-docs/week06_补充_CSE与Eigen稀疏组装.md" }
    }
  }
};

let currentWeek = "w1";
let currentKind = "plan";
let activeDocPath = "";

const content = document.querySelector("#doc-content");
const toc = document.querySelector("#doc-toc");
const weekTabs = [...document.querySelectorAll(".week-tab[data-week]")];
const kindTabs = [...document.querySelectorAll(".doc-tab[data-kind]")];
const extraTab = document.querySelector("#extra-tab");

function escapeHtml(value) {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

// 站内 md 链接 → 切换阅读器；其余相对链接 → 跳转 GitHub 对应文件
function docTargetForLink(href) {
  const decoded = decodeURIComponent(href);
  const filename = decoded.split("/").filter(Boolean).pop();
  for (const [wk, week] of Object.entries(weeks)) {
    for (const [kind, doc] of Object.entries(week.docs)) {
      if (doc.path.endsWith(filename)) return { week: wk, kind };
    }
  }
  return null;
}

function resolveRepoHref(href) {
  if (/^(https?:|mailto:|#)/.test(href)) return null;
  const baseDir = activeDocPath.split("/").slice(0, -1);
  const stack = [...baseDir];
  for (const part of decodeURIComponent(href).split("/")) {
    if (part === "..") stack.pop();
    else if (part !== "." && part !== "") stack.push(part);
  }
  const isDir = href.endsWith("/");
  return (isDir ? REPO_TREE : REPO_BLOB) + encodeURI(stack.join("/"));
}

function renderInline(source) {
  const codeTokens = [];
  let value = source.replace(/`([^`]+)`/g, (_, code) => {
    const token = `@@CODE${codeTokens.length}@@`;
    codeTokens.push(`<code>${escapeHtml(code)}</code>`);
    return token;
  });

  value = escapeHtml(value);
  value = value.replace(/\[([^\]]+)\]\(([^)]+)\)/g, (_, text, href) => {
    const target = docTargetForLink(href);
    if (target) {
      return `<a href="#reader" data-doc-week="${target.week}" data-doc-kind="${target.kind}">${text}</a>`;
    }
    const repoHref = resolveRepoHref(href);
    if (repoHref) {
      return `<a href="${repoHref}" target="_blank" rel="noreferrer">${text}</a>`;
    }
    const external = /^https?:\/\//.test(href);
    return `<a href="${href}"${external ? ' target="_blank" rel="noreferrer"' : ""}>${text}</a>`;
  });
  value = value.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
  value = value.replace(/__([^_]+)__/g, "<strong>$1</strong>");
  value = value.replace(/(?<!\*)\*([^*]+)\*(?!\*)/g, "<em>$1</em>");
  value = value.replace(/~~([^~]+)~~/g, "<del>$1</del>");
  codeTokens.forEach((code, index) => {
    value = value.replace(`@@CODE${index}@@`, () => code);
  });
  return value;
}

// 轻量代码高亮（输入已 escapeHtml）：C++ / CMake / shell 注释
function highlightCode(escaped, language) {
  const lang = (language || "").toLowerCase();
  const tokens = [];
  const stash = (cls) => (match) => {
    tokens.push(`<span class="${cls}">${match}</span>`);
    return `@@TOK${tokens.length - 1}@@`;
  };
  let v = escaped;

  if (lang === "cpp" || lang === "c++" || lang === "c") {
    v = v.replace(/\/\/[^\n]*/g, stash("tok-com"));
    v = v.replace(/\/\*[\s\S]*?\*\//g, stash("tok-com"));
    v = v.replace(/&quot;[\s\S]*?&quot;/g, stash("tok-str"));
    v = v.replace(/^[ \t]*#[ \t]*\w+[^\n]*/gm, stash("tok-pre"));
    v = v.replace(/\b(if|else|for|while|return|const|constexpr|static|inline|struct|class|enum|namespace|using|template|typename|public|private|protected|auto|void|bool|int|double|float|char|size_t|true|false|nullptr|switch|case|break|continue|new|delete|sizeof|this|operator|friend|virtual|override)\b/g,
      '<span class="tok-kw">$1</span>');
    v = v.replace(/\b(\d+(?:\.\d+)?(?:e[+-]?\d+)?[fLu]*)\b/gi, '<span class="tok-num">$1</span>');
  } else if (lang === "cmake") {
    v = v.replace(/#[^\n]*/g, stash("tok-com"));
    v = v.replace(/&quot;[\s\S]*?&quot;/g, stash("tok-str"));
    v = v.replace(/^([ \t]*)([A-Za-z_][A-Za-z0-9_]*)([ \t]*\()/gm, '$1<span class="tok-kw">$2</span>$3');
    v = v.replace(/\$\{[^}]+\}/g, (m) => `<span class="tok-num">${m}</span>`);
  } else if (lang === "bash" || lang === "sh" || lang === "fish" || lang === "cmd" || lang === "text") {
    v = v.replace(/(^|\s)#[^\n]*/g, stash("tok-com"));
  }

  tokens.forEach((token, index) => {
    v = v.replace(`@@TOK${index}@@`, () => token);
  });
  return v;
}

function slugify(text, index) {
  const clean = text
    .replace(/<[^>]+>/g, "")
    .replace(/[^\p{Letter}\p{Number}]+/gu, "-")
    .replace(/^-|-$/g, "")
    .toLowerCase();
  return `${clean || "section"}-${index}`;
}

function isTableDivider(line) {
  return /^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)+\|?\s*$/.test(line);
}

function tableCells(line) {
  return line.trim().replace(/^\||\|$/g, "").split("|").map(cell => cell.trim());
}

function isSpecial(line, nextLine = "") {
  return /^(#{1,6})\s+/.test(line)
    || /^\s*```/.test(line)
    || /^\s*>/.test(line)
    || /^\s*([-*+]\s+|\d+\.\s+)/.test(line)
    || /^\s*([-*_])(?:\s*\1){2,}\s*$/.test(line)
    || (line.includes("|") && isTableDivider(nextLine));
}

function renderMarkdown(markdown) {
  const lines = markdown.replace(/\r\n?/g, "\n").split("\n");
  const output = [];
  let headingIndex = 0;
  let index = 0;

  while (index < lines.length) {
    const line = lines[index];
    const nextLine = lines[index + 1] || "";

    if (!line.trim()) {
      index += 1;
      continue;
    }

    if (/^\s*```/.test(line)) {
      const language = line.trim().slice(3).trim();
      const code = [];
      index += 1;
      while (index < lines.length && !/^\s*```/.test(lines[index])) {
        code.push(lines[index]);
        index += 1;
      }
      index += 1;
      const highlighted = highlightCode(escapeHtml(code.join("\n")), language);
      output.push(`<pre><button class="copy-code" type="button">复制</button><code${language ? ` data-language="${escapeHtml(language)}"` : ""}>${highlighted}</code></pre>`);
      continue;
    }

    const heading = line.match(/^(#{1,6})\s+(.+)$/);
    if (heading) {
      const level = heading[1].length;
      const rendered = renderInline(heading[2]);
      const id = slugify(rendered, headingIndex++);
      output.push(`<h${level} id="${id}">${rendered}</h${level}>`);
      index += 1;
      continue;
    }

    if (/^\s*([-*_])(?:\s*\1){2,}\s*$/.test(line)) {
      output.push("<hr>");
      index += 1;
      continue;
    }

    if (/^\s*>/.test(line)) {
      const quote = [];
      while (index < lines.length && /^\s*>/.test(lines[index])) {
        quote.push(lines[index].replace(/^\s*>\s?/, ""));
        index += 1;
      }
      output.push(`<blockquote><p>${quote.map(line => renderInline(line)).join("<br>")}</p></blockquote>`);
      continue;
    }

    if (line.includes("|") && isTableDivider(nextLine)) {
      const headers = tableCells(line);
      index += 2;
      const rows = [];
      while (index < lines.length && lines[index].includes("|") && lines[index].trim()) {
        rows.push(tableCells(lines[index]));
        index += 1;
      }
      output.push(`<div class="table-wrap"><table><thead><tr>${headers.map(cell => `<th>${renderInline(cell)}</th>`).join("")}</tr></thead><tbody>${rows.map(row => `<tr>${row.map(cell => `<td>${renderInline(cell)}</td>`).join("")}</tr>`).join("")}</tbody></table></div>`);
      continue;
    }

    const listMatch = line.match(/^\s*([-*+]|\d+\.)\s+(.+)$/);
    if (listMatch) {
      const ordered = /\d+\./.test(listMatch[1]);
      const tag = ordered ? "ol" : "ul";
      const items = [];
      while (index < lines.length) {
        const match = lines[index].match(/^\s*([-*+]|\d+\.)\s+(.+)$/);
        if (!match || /\d+\./.test(match[1]) !== ordered) break;
        items.push(`<li>${renderInline(match[2])}</li>`);
        index += 1;
      }
      output.push(`<${tag}>${items.join("")}</${tag}>`);
      continue;
    }

    const paragraph = [line.trim()];
    index += 1;
    while (index < lines.length && lines[index].trim() && !isSpecial(lines[index], lines[index + 1] || "")) {
      paragraph.push(lines[index].trim());
      index += 1;
    }
    output.push(`<p>${renderInline(paragraph.join(" "))}</p>`);
  }

  return output.join("\n");
}

function buildToc() {
  const headings = [...content.querySelectorAll("h2, h3")];
  if (!headings.length) {
    toc.innerHTML = '<span class="toc-loading">此文档没有可用目录。</span>';
    return;
  }
  toc.innerHTML = headings.map(heading => (
    `<a class="toc-${heading.tagName.toLowerCase()}" href="#${heading.id}">${heading.textContent}</a>`
  )).join("");
}

function bindRenderedContent() {
  content.querySelectorAll("[data-doc-week]").forEach(link => {
    link.addEventListener("click", () => {
      loadDocument(link.dataset.docWeek, link.dataset.docKind || "plan");
    });
  });

  content.querySelectorAll(".copy-code").forEach(button => {
    button.addEventListener("click", async () => {
      const code = button.nextElementSibling.textContent;
      await navigator.clipboard.writeText(code);
      button.textContent = "已复制";
      window.setTimeout(() => { button.textContent = "复制"; }, 1200);
    });
  });
}

function syncTabs() {
  weekTabs.forEach(tab => {
    const active = tab.dataset.week === currentWeek;
    tab.classList.toggle("is-active", active);
    tab.setAttribute("aria-selected", String(active));
  });
  kindTabs.forEach(tab => {
    const active = tab.dataset.kind === currentKind;
    tab.classList.toggle("is-active", active);
    tab.setAttribute("aria-selected", String(active));
  });
  if (extraTab) extraTab.textContent = weeks[currentWeek].extraLabel;
}

async function loadDocument(weekKey, kindKey) {
  currentWeek = weeks[weekKey] ? weekKey : "w1";
  currentKind = weeks[currentWeek].docs[kindKey] ? kindKey : "plan";
  const week = weeks[currentWeek];
  const doc = week.docs[currentKind];
  activeDocPath = doc.path;
  syncTabs();

  content.innerHTML = '<div class="loading-card"><span></span><p>正在读取文档…</p></div>';
  toc.innerHTML = '<span class="toc-loading">正在整理目录…</span>';

  try {
    const response = await fetch(encodeURI(doc.path));
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const markdown = await response.text();
    content.innerHTML = renderMarkdown(markdown);
    buildToc();
    bindRenderedContent();
    document.title = `${week.label} ${doc.label} · 几何算法工程师的 AI 之路`;
  } catch (error) {
    content.innerHTML = `<div class="error-card"><strong>文档读取失败</strong><p>请刷新页面重试，或前往 <a href="https://github.com/liyongzheng666/geometry-engineer-ai-road" target="_blank" rel="noreferrer">GitHub 仓库</a>查看原文。</p><code>${escapeHtml(error.message)}</code></div>`;
    toc.innerHTML = '<span class="toc-loading">目录暂不可用。</span>';
  }
}

weekTabs.forEach(tab => tab.addEventListener("click", () => loadDocument(tab.dataset.week, currentKind)));
kindTabs.forEach(tab => tab.addEventListener("click", () => loadDocument(currentWeek, tab.dataset.kind)));

// 路线清单里的"阅读拆解" → 跳到阅读器并切到对应周
document.querySelectorAll("[data-open-week]").forEach(link => {
  link.addEventListener("click", () => loadDocument(link.dataset.openWeek, "plan"));
});

document.querySelector("#print-page").addEventListener("click", () => window.print());
loadDocument("w1", "plan");

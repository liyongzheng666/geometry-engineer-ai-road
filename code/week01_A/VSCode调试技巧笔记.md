# VSCode 调试技巧笔记（以 week01_A 为例）

这篇承接 [CMake构建与编译笔记（Mac·Clang篇）.md](CMake构建与编译笔记（Mac·Clang篇）.md)：代码能编译、能 F5 断下之后，
怎么把调试看得**更清楚**、抓 bug 抓得**更准**。三件事：
**变量显示增强 → 用图看曲线 → 值被改时自动中断**。

本工程用的调试器是 **CodeLLDB**（`vadimcn.vscode-lldb`），底层是 LLDB。
下面的配置已经写进 `.vscode/` 里，重开工作区 + F5 重启一次即可生效。

---

## 0. 一句话直觉

> 调试器看到的「变量」就是**一块内存 + 一个类型**。
> 默认它老老实实把每个成员铺开给你看；我们做的事是教它
> **① 用更顺眼的方式显示**、**② 用图画出来**、**③ 这块内存被写时喊停**。

---

## 1. 变量显示增强

### 1.1 让 Point3D 一行显示成 (x, y, z)

默认 `Point3D` 在变量面板里要展开才看到 x/y/z，`vector<Point3D>` 更是一坨。
在 `.vscode/launch.json` 里给这个配置加了 `initCommands`：

```jsonc
"initCommands": [
  "type summary add --summary-string \"(${var.x}, ${var.y}, ${var.z})\" Point3D"
]
```

`type summary` 是 LLDB 的「类型摘要」命令：告诉调试器**遇到 Point3D 类型，那一行就按这个模板显示**。
效果：`ctrl`、`curve` 这些 `vector<Point3D>` 的每个元素直接是 `(1, 0, 0)`，不用点开。

> 想临时改格式，直接在调试控制台敲（不用改文件）：
> `type summary add --summary-string "(${var.x}, ${var.y})" Point3D`  ← 只显示 x,y

### 1.2 行尾内联值 + 显示类型

`.vscode/settings.json` 里开了两项：

```jsonc
"debug.inlineValues": "on",       // 变量当前值直接贴在代码行尾，不用悬停
"debug.showVariableTypes": true   // 变量面板同时显示类型，区分 double / Point3D
```

---

## 2. 用 Debug Visualizer 把曲线画出来

几何代码最爽的一点：**形状对不对，眼睛一看就知道**，比盯一列数字快十倍。
扩展：**Debug Visualizer**（`hediet.debug-visualizer`，已加进推荐扩展）。

用法：
1. 断在算完 `curve` / `arc` 之后那一行。
2. `Cmd+Shift+P` → `Debug Visualizer: New View`。
3. 表达式框填变量名（如 `curve`），右上角 visualizer 下拉选 **Plotly** 或 **Scatter**。

它会读每个 `Point3D` 的 x/y 子成员画成 2D 点图 / 折线。

> **如果某个 vector 自动识别不出 x/y**（C++ 下偶尔会发生）：
> 这是因为 Debug Visualizer 需要一段「能解析的数据」。解决办法是在调试时
> 让程序输出一段 JSON 字符串再喂给它，或临时加个 `to_json()` 辅助函数。
> 真遇到了再处理，多数情况直接选 Plotly 就能画。

---

## 3. 值被修改时自动中断（watchpoint / 数据断点）

场景：**某个控制点的值莫名其妙变了，想抓「是哪行代码改的」**。
普通断点是「执行到某行停」，watchpoint 是「**某块内存被写就停**」，正好治这个。
LLDB 用的是 CPU 硬件 watchpoint，所以很快，但有数量限制（见下）。

### 3.1 图形方式（最简单）

调试断下后，在左侧 **变量面板** 右键某个变量 →
**「Break When Value Changes / 值更改时中断」**。
之后只要那块内存被写入，自动断在改它的那行。

### 3.2 控制台方式（更灵活）

`.vscode/settings.json` 里设了 `"lldb.consoleMode": "commands"`，
所以调试控制台可以**直接敲 LLDB 命令**：

```
watchpoint set variable myPoint.x        # 这个变量被写就断
watchpoint set expression -- &ctrl[2]    # 监视数组里某个元素
watchpoint modify -c '(myPoint.x > 10)'  # 加条件，满足才断
watchpoint list                          # 看当前有哪些
watchpoint delete 1                       # 删掉编号 1 的
```

> 命令模式下，要算 C++ 表达式得在前面加反引号：`` `ctrl.size() ``。

### 3.3 两个坑（Apple Silicon / ARM Mac 尤其注意）

1. **数量很少**：硬件 watchpoint 寄存器通常只有 ~4 个，不能无限设。
2. **盯的是固定地址**：
   - 局部变量一旦出作用域，地址失效，watchpoint 自动没了；
   - `std::vector` 扩容会**重新分配内存**，旧地址的 watchpoint 就盯不到新数据了 ——
     这种情况下要重新对新元素设一次。

---

## 4. 速查表

| 我想…… | 怎么做 |
|---|---|
| Point3D 不展开就看到坐标 | 已配（launch.json 的 `type summary`） |
| 变量值贴在代码行尾 | 已配（`debug.inlineValues`） |
| 把控制点 / 曲线画成图 | Debug Visualizer → 填变量名 → 选 Plotly |
| 抓某个值被谁改了 | 变量面板右键「Break When Value Changes」 |
| 控制台手动设监视 | `` watchpoint set variable 变量名 `` |
| 临时改 Point3D 显示格式 | 控制台敲 `type summary add ... Point3D` |

> 改了 `.vscode/` 下的配置后，记得**重开工作区**让推荐扩展提示弹出，
> 并 **F5 重启调试**让 `initCommands` 重新执行。

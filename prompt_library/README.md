# prompt_library — 个人 Prompt 模板库

这是**你自己逐周填充**的模板库（第 12 周的"产出物 1"）。课程只给目录约定和一个示例，
内容由你在每周实操中提炼——别人的模板抄来不顺手，自己踩坑攒出来的才会真用。

## 目录约定（来自总纲附录 A）

```text
prompt_library/
├── README.md                  # 本索引：模板编号、名字、一句话用途
├── 01_algorithm_design/       # 几何算法生成（第 1-3 周开始攒）
│   ├── geometric_algorithm.md
│   ├── degenerate_checklist.md
│   ├── context_injection.md
│   └── numerical_stability.md
├── 02_code_review/            # 第 3、9 周
├── 03_debug/                  # 第 7 周（CGAL 模板报错 / OCCT Handle）
├── 04_documentation/          # 第 11 周（Doxygen / 论文摘要）
├── 05_daily_dev/              # 第 11 周（commit / PR / 单测生成）
└── 06_math/                   # 第 5-6 周（SymPy 推导 / 符号→C++）
```

每个模板独立成文件，可以 `cat` 后通过管道喂给 Claude Code。

## 模板格式（每个文件照此写）

```markdown
## 用途：一句话说明什么时候用它

## 模板
（正文，{花括号} 标出填坑变量）

## 填坑变量
{变量1}, {变量2}, ...

## 使用示例
（一次成功的输入 + 输出摘要）

## 失败模式
（什么时候这个模板会不灵）
```

## 示例：01_algorithm_design/geometric_algorithm.md（第 1 周周四的产出）

```markdown
## 用途：让 AI 写工业级几何算法 C++ 代码

## 模板
你是工业级几何算法工程师。请按以下顺序回答：
1. 写出 {算法名} 的数学定义（用 LaTeX）
2. 列出所有可能的退化/奇异输入（不少于 {N} 种）
3. 对每个退化情况给出处理策略
4. 给出 C++ 实现，要求：
   - 使用 {算法名选项}（避免使用 {数值不稳定的替代}）
   - 函数签名：{签名}
   - 假设 {可用的项目类型，如 Point3D 重载} 已存在
5. 给出至少 3 个能触发退化分支的单元测试输入

## 填坑变量
{算法名}, {N}, {算法名选项}, {数值不稳定的替代}, {签名}
```

## 当前索引

| 编号 | 模板 | 用途 | 状态 |
|---|---|---|---|
| 01 | geometric_algorithm | 工业级几何算法生成 | 示例已给，第 1 周完善 |
| 02 | degenerate_checklist | 生成代码前穷举退化输入 | 第 2 周周末产出 |
| 03 | context_injection | 注入项目类型与风格禁令 | 第 3 周产出 |
| … | | 逐周追加 | |

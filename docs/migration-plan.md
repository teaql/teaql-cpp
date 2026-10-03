# TEAQL C++ 迁移计划

## 唯一基准

Rust `v5.0.5`，提交 `62940ae2b88e7acc32763853b8b4685a01c7e90a`。
机器可读基准见 `upstream.lock.json`。本机源仓库实际路径是
`~/githome/teaql-rs`；`.reference/teaql-rs` 是该提交的 detached worktree，
不参与 C++ 构建，不提交到版本库。不得使用源仓库当前分支替代锁定版本。

目标是参照 Rust 分层与核心行为，为边缘运行提供 SQLite 数据访问能力，
接入 C++ 系统编程与 GPU 生态。C++ 初始版本号为 `0.1.0`，不以 `5.0.5`
暗示已经完成全量兼容。内存与资源所有权采用 C++ 的全局 / UserContext
两级生命周期，不机械照搬 Rust 的 Arc 与异步执行方式。

以内核优先，保持精巧：C++20 标准库承担容器、内存和资源生命周期；数据库
仅引入 SQLite。暂不引入第三方容器、async、logging 框架或 GPU SDK。

明确排除：MySQL、PostgreSQL、Redis、Meilisearch、云服务发现/配置/actuator、
Web 框架和 TFP endpoint。Linux 等系统能力通过 C++ 集成边界按需接入，
不把 Rust 周边组件逐个迁移作为目标。

## 分阶段交付

1. **基准与工程**：锁文件、C++20/CMake、模块依赖边界、测试。
   优先确定两级内存生命周期、跨 Context 数据传递、任务结束与资源释放契约。
2. **Core**：Value/数值边界、元数据、Expr、SelectQuery、独立的
   MutationValues/EntitySnapshot/GeneratedValues、命令、TraceNode；随后补齐
   Decimal/JSON/时间、CompactRow/SmartList、实体状态和请求构建层。
3. **SQL 与 data-service**：SqlDialect、CompiledQuery、SqlBindings、
   SqlTransport、QueryExecutor/MutationExecutor、SQLite 首条执行链路；再补齐
   子查询、表达式投影、关系聚合、分区分页、批量操作、stream、schema SPI。
4. **Runtime**：UserContext、metadata registry、purpose/comment/audit trace、
   SQL logging；随后按 Rust 迁移事务、实体图身份映射、关系装配、mutation ledger、
   persisted readback、审计事件、mutation policy/approval、business ID、
   context-bound/round-trip reference、检查器和查询策略。
5. **边缘集成与验收**：SQLite 完整性、C++ 系统资源与 GPU
   适配接口、Context 生命周期压力测试、内存上界与运行开销测量。
   不内置特定 GPU 厂商 SDK；先明确任务完成与资源销毁边界。

当前推进方式：优先按语义迁移 Rust 的简单 SQLite examples，以可运行示例驱动
最小内核能力。模型和关系映射手写；暂不修改生成器。

每阶段以实际行为测试验收；不把类型占位或未实现接口记作已完成。

## Rust → C++ 对应关系

| Rust crate / 模块 | C++ 位置 / 目标 | 首批范围 |
| --- | --- | --- |
| teaql-core: value/meta/expr/query/mutation/trace | include/teaql/core/, teaql::core | 标量、元数据、基本 AST/查询、单实体命令、trace |
| teaql-data-service | 暂并入 runtime/sqlite.hpp 与 sql/sqlite_dialect.hpp | 结果和参数日志策略；未复制通用 executor 框架 |
| teaql-sql | include/teaql/sql/, src/sql/, teaql::sql | 参数化编译、字段映射、SQLite dialect、transport adapter |
| teaql-provider-sqlite | src/runtime/sqlite.cpp，teaql::runtime | 小型原生 SQLite 会话、prepared statement、事务与资源生命周期 |
| teaql-runtime | include/teaql/runtime/, src/runtime/, teaql::runtime | registry、UserContext、查询意图、写入原因、安全 SQL 日志 |
| teaql-macros / generated_support | 暂不修改 | examples 使用手写模型与映射 |
| 其他 provider、cache、web、cloud、tfp | 不迁移 | 不在边缘版范围内 |

## 两级生命周期（已确认）

- 全局：不可变模型元数据、服务工厂、长期存在的系统/GPU 服务；由 Runtime
  拥有，生命周期长于其创建的所有 UserContext。禁止长期缓存指向 Context
  arena 的指针、视图或 allocator。
- Context：请求、实体、查询结果、mutation 工作集、日志缓冲和临时数据。
  使用显式内存资源分配，销毁 Context 后无 Context 所有的存活分配。
- 当前已实现的 core 动态存储、SQL 参数、查询结果和日志使用显式 PMR resource。
  Value 与 Expr 的跨域复制必须显式提供目标 resource；普通 copy 被禁止。
- 已确认：可增长的多块 arena，Context 销毁时全部回收；结果归 Context，
  销毁后失效，必须显式复制才能跨 Context 保留。禁止共享引用延长 Context
  的隐式生命周期。底层实现和限制见 `context-lifetime.md`。
- 分配入口显式为 `global.make<T>()` / `user_context.make<T>()`，外部资源
  使用对应 scope 的 `make_resource<T>()`。UserContext 必须关联一个显式的
  GlobalContext，不允许默认构造时暗中选择全局 allocator。
- SQLite 连接/statement、文件描述符、GPU buffer/stream/event 不因 arena
  回收自动释放，必须先执行清理。SQLite/GPU 驱动内部内存单独核算，不能
  宣称这些第三方分配也来自 Context 的唯一内存块。
- 清理顺序：禁止新操作 → 等待/结束已提交任务 → rollback/finalize 与 GPU
  清理 → 析构 Context 对象 → 释放 arena。不能在 GPU 仍访问主机内存时释放。
- 回收保证指释放分配的所有权，不保证进程 RSS 立即降低；操作系统和分配器
  可能保留已释放的页。

## 5.0.5 专项要求

- MutationValues、EntitySnapshot、GeneratedValues 必须为不同类型。
- 参数的 Plain/Masked/Credential/Unknown 策略由编译器根据字段元数据生成，
  执行输入与日志投影分离；缺少字段声明默认 Unknown。SQL 失败也要记录结果。
- 完整日志兼容还包括 free-text/trace 脱敏、safe/debug 投影、observer、
  cancellation 和 formatter；首批没有完成的分支必须明确标记。
- mutation policy/approval、graph save、readback、审计与事务必须作为完整行为迁移；
  不把普通低层 command 执行等同于高层 audited graph save。
- 保持乐观锁版本递增、软删除负版本、恢复正版本的 Rust 语义。
- 标准库无法无损表达的 Decimal/JSON/date 等能力后续明确选择实现，
  不以 double 或无校验字符串替代并宣称兼容。

## 验证策略

- 从锁定 Rust 测试提取固定 SQL/参数期望，标注源文件及测试名。
- SQLite 内存数据库验证 query → mutation → query，冲突与失败日志。
- 测试字段映射、NULL、空 IN、分页上限、参数顺序、版本边界、日志策略。
- 编译测试明确 MutationValues 不可由 snapshot/generated 隐式转换。
- 后续以共享 fixtures 和 Rust/C++ 双运行器扩展为自动差分测试。

## 首批交付状态

已实现并建立测试：

- 版本锁定与校验、CMake 工程，依赖仅为标准库和 SQLite。
- GlobalContext / UserContext 显式分配、可增长 arena、allocator-aware 对象构造、
  逆序析构、全部块回收及跨域显式复制。
- PMR 标量 Value、元数据、基本 Expr/SelectQuery、单实体 mutation 命令和 trace。
- SQLite 建表、参数化执行、字段映射、分页上限、乐观锁、软/硬删除、恢复、事务。
- SQL 成功/失败日志，保留参数化 SQL；日志参数与执行输入分离，敏感意图文本脱敏。
- `sqlite_schema_crud` 和 `sqlite_relations_graph` 的手写语义迁移与运行断言。
  已实际运行锁定版本的两个 Rust 示例核对结果。

明确未实现：通用实体图与自动关系装配、mutation policy/approval、persisted
readback、完整审计事件、完整 Rust 日志格式/observer/明文调试、异步任务/GPU
适配器、stream、schema 变更迁移、Decimal/JSON/时间、完整聚合/子查询 AST。
关系示例显式执行小型事务并按外键查询装配，不等同于通用 graph save。

有意的首批边界：Masked 参数使用完整 `[REDACTED]`，尚未实现 Rust 的部分掩码；
未知写入字段报错，避免静默丢弃拼写错误；超出 SQLite 有符号 64 位整数范围报错。
Rust 返回实体需要的生成器与 derive 由示例手写元数据替代，不改生成器。

下一步继续以具体 example 和行为测试驱动内核扩展，不预先移植外围组件。

# TEAQL C++

面向边缘运行的 TEAQL，参照 Rust **v5.0.5** 的核心设计，数据库仅支持 SQLite。
接入 C++ 系统编程与 GPU 生态，内存按全局和 UserContext 两级生命周期管理。
以内核优先，使用 C++20 标准库，避免引入外围框架。

锁定提交：`62940ae2b88e7acc32763853b8b4685a01c7e90a`，详见
[基准锁文件](upstream.lock.json) 和 [迁移计划](docs/migration-plan.md)。

已迁移并运行两个 Rust 示例的业务语义：

- [sqlite_schema_crud](examples/sqlite_schema_crud.cpp)：建表、创建、更新、软删除、查询，版本 `1 → 2 → -3`。
- [sqlite_relations_graph](examples/sqlite_relations_graph.cpp)：订单及两条明细原子写入，查询并装配商品引用。

当前内核包含显式 PMR 分配、标量 Value、元数据、基本表达式/查询、mutation
命令、SQLite 参数化执行、乐观锁、恢复、事务及安全 SQL 日志。详见
[生命周期契约](docs/context-lifetime.md)。仅依赖 C++20 标准库和 SQLite。

模型与关系映射均为手写，未修改生成器。关系示例没有宣称实现 Rust 通用实体图
引擎；完整 graph save、mutation policy/approval、审计事件、stream 及完整日志
格式仍待迁移。日志目前对 Masked/Unknown/Credential 参数统一完整隐藏，较 Rust
的业务值部分掩码更保守，不提供明文调试模式。
分配显式选择 `global.make<T>()` 或 `user_context.make<T>()`。Context 结果
在 Context 销毁后失效，保留数据必须显式深拷贝到目标生命周期。

## 构建

要求 CMake 3.20+、支持 C++20 的编译器、SQLite 3.37+ 开发库（如 libsqlite3-dev）。
core 和 memory targets 本身仅依赖标准库。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/teaql_context_lifetime
./build/teaql_sqlite_schema_crud
./build/teaql_sqlite_relations_graph
```

## 获取固定参考

参考源只用于开发对照，不参与 C++ 构建。

```sh
git clone --branch v5.0.5 https://github.com/teaql/teaql-rs.git .reference/teaql-rs
git -C .reference/teaql-rs checkout --detach 62940ae2b88e7acc32763853b8b4685a01c7e90a
python3 scripts/verify-upstream.py
```

## 对照与测试

两个 Rust 5.0.5 原始示例已在固定参考工作区运行，确认版本变化、订单/明细与商品
对应关系一致。C++ 示例自带运行时断言，并加入 CTest。另有 core、arena 生命周期、
SQLite 回滚/冲突/脱敏/参数绑定测试；测试禁用默认 PMR resource 来检查隐式分配。

API 仍处于初始迁移阶段，未实现的能力不通过占位实现宣称兼容。

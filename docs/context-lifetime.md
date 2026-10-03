# UserContext 生命周期契约

已确认：可增长的多块 arena；所有结果归 Context 所有，销毁后失效。
跨生命周期保留必须显式复制。不采用 shared_ptr 延迟回收。

## 两层所有权

| 所有者 | 典型内容 | 销毁时机 |
| --- | --- | --- |
| 全局 Runtime / 宿主应用 | 不可变模型元数据、服务工厂、长期 GPU/系统服务 | 所有依赖它的 Context 结束之后 |
| UserContext | 查询工作集、实体与结果、mutation 工作集、日志缓冲、临时资源 | 显式 close() 或 Context 析构 |

全局对象不能保存 Context 分配的指针或 string_view。Context 可以借用比它活得
更久的全局对象。线程局部存储与静态缓存不能成为第三种隐含所有权。

## 已实现的内存底座

底层 `Arena` 使用 `std::pmr::monotonic_buffer_resource` 按需增长，默认
向 `new_delete_resource()` 申请块。每个 Context 独占自己的块；不会把块留在
一个长期存活的全局 pool 中。可注入其他 upstream resource，但其生命周期必须
更长；是否缓存已归还块由该 resource 决定。

所有业务分配必须从调用处看得出生命周期：

```cpp
teaql::GlobalContext global;
auto& metadata = global.make<std::pmr::string>("EdgeDevice");
auto user_context = global.make_user_context();
auto& rows = user_context->make<std::pmr::vector<std::pmr::string>>();
```

`GlobalContext` 是应用显式拥有的对象，不是 singleton。`UserContext` 必须显式
关联 GlobalContext，不提供默认构造。GlobalContext 必须活得更久；提前销毁时
会 fail-fast，避免悄悄留下悬空元数据引用。当前两个 scope 都按单线程使用。

`global.make<T>()` 与 `user_context.make<T>()` 返回借用引用，并对非平凡析构
类型登记析构动作。普通 std::string/vector 被 make 的编译期约束拒绝；
`make_resource<T>()` 是外部 RAII 包装器的显式入口，不承诺其内部使用 arena。
`std::pmr::vector`、`std::pmr::string` 等 allocator-aware 类型自动接收 Context
的 allocator，嵌套 PMR 容器同样传播 allocator。普通 `std::vector` / `std::string`
不会自动改成 arena 分配；它们只能通过已登记对象的析构释放自己的独立分配。
当前已实现的 core 动态存储、SQL 参数、结果和日志使用显式 PMR。测试在禁用
默认 PMR resource 后运行完整 SQLite 路径，检查是否存在隐藏的 allocator 回退。

关闭顺序：

1. 将 Context 标记为关闭，禁止新分配。
2. 按成功构造的逆序执行已登记对象的析构。
3. 归还所有 arena 块；live_blocks 和 live_bytes 归零。

Context 不可复制或移动。`close()` 幂等且不抛异常，析构自动调用相同逻辑。
对象构造失败时不登记该对象析构，已经分配的 arena 空间在 Context 关闭时回收。
不提供 reset/reuse，以免旧引用在新请求中混用。

## 使用方式

```cpp
teaql::GlobalContext global;
auto ctx = global.make_user_context();
auto& rows = ctx->make<std::pmr::vector<std::pmr::string>>();
rows.emplace_back("edge-device-001");
// 显式将值深拷贝到 global；嵌套字符串也使用 global allocator。
auto& retained = global.make<std::pmr::vector<std::pmr::string>>(rows);
ctx.reset(); // 析构 Context 对象并释放全部 Context 块
// rows 从此失效，retained 继续有效，直到 global 销毁。
```

`make` / `make_resource` 返回的对象不得自行 delete、手动析构，或交给拥有所有权的智能指针。
如果在栈上直接创建使用 `ctx.memory_resource()` 的 PMR 容器，必须让这个容器
先于 Context 关闭而析构；更推荐由 `ctx.make` 登记其生命周期。

## 非内存资源与 GPU

释放 arena 不会替代 SQLite finalize/rollback、文件 close 或 GPU buffer 释放。
外部资源通过 `global.make_resource<T>()` 或 `ctx.make_resource<T>()`
选择生命周期，再通过 noexcept RAII 析构接入清理序列，遵循逆构造顺序的依赖关系。
析构阶段不允许继续向 Context 分配。

当前实现仅支持单线程、同步使用，不包含任务调度或 GPU 同步实现。宿主必须先
结束所有使用此 Context 的任务，然后关闭它。后续 GPU 适配器必须管理 completion
事件/同步，保证设备不再访问 Context 的主机内存；发出 cancel 请求本身不够。
设备内存和 SQLite/驱动内部堆内存单独核算，通过资源析构释放，不声称来自 arena。

内存回收保证是分配所有权已释放，不是操作系统 RSS 立即下降。单个长寿命 Context
中的临时分配不会逐项返还给 upstream；边缘工作负载应使用短生命周期 Context，
后续可增加显式内存预算，而不是默默扩展为跨请求缓存。

## 测试验收

- 多块增长及每次 upstream 分配的大小/对齐对应释放。
- 256 字节对齐对象、嵌套 PMR allocator 传播。
- 资源逆序析构发生在释放块之前。
- 构造异常、分配失败、重复关闭、关闭后的分配拒绝。
- 多 Context 隔离及 1000 次创建/销毁后无遗留块。
- global/Context 分配分离、跨域显式深拷贝、Context 关闭不回收 global 数据。
- AddressSanitizer / UndefinedBehaviorSanitizer 验证。

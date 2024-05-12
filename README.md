## 1.Project 0: primer

### ------Task1----
Remove():
bug1: 每次遍历key的过程都会clone(),而每次对于值节点的clone()都会获得一个is_value_node_为true的值节点，但是这个字段未必为true，即使它是一个值节点。主要是因为clone()返回的都是基类的指针，我们并不知道实际的类型。clone()后要判定一下原来的is_value_node避免值节点恢复的情况。

### -----Task 2-----

并发键值存储..easy

允许多读者一写者,put()和delete()都是写.

### ----Task 3 -----

debug....lldb/gdb/print

### ----Task 4-----

lower / upper 函数:easy..

### -----other-----

#### 构建系统

debug模式:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j`nproc`
```

release模式:

```
mkdir build_rel && cd build_rel
cmake -DCMAKE_BUILD_TYPE=Release ..
```



#### 测试

````
$ cd build
$ make trie_test trie_store_test -j$(nproc)
$ make trie_noncopy_test trie_store_noncopy_test -j$(nproc)
$ ./test/trie_test
$ ./test/trie_noncopy_test
$ ./test/trie_store_test
$ ./test/trie_store_noncopy_test
````



#### 检测格式

```
make format && make check-lint && make check-clang-tidy-p2
```



#### 提交

```
make submit-p2
```



#### 内存泄漏

[ASAN and LSAN](https://clang.llvm.org/docs/AddressSanitizer.html)

禁用内存泄漏检测：

```
$ cmake -DCMAKE_BUILD_TYPE=Debug -DBUSTUB_SANITIZER= ..
```



#### 断言

```
BUSTUB_ASSERT: debug模式
BUSTUB_ENSURE: debug && release模式
```



#### 日志

```
包含 src/include/common/logger. h
LOG_INFO("# Pages: %d", num_pages);
LOG_DEBUG("Fetching page %d", page_id);
```



## 0. Project 0

2024/4/5

cow字典树

## 1. Project 1 buffer pool

2024/4/9

#### project1排行榜任务(可选)

```
mkdir cmake-build-relwithdebinfo
cd cmake-build-relwithdebinfo
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j`nproc` bpm-bench
./bin/bustub-bpm-bench --duration 5000 --latency 1
```



#### **建议的优化**(wait)

1. Better replacer algorithm. Given that get workload is skewed (i.e., some pages are more frequently accessed than others), you can design your LRU-k replacer to take page access type into consideration, so as to reduce page miss.
   更好的算法。假设get工作负载是偏斜的（即，某些页面比其它页面更频繁地被访问）， 您可以在设计LRU-k缓存器时考虑页面访问类型，以减少页面丢失。
2. Parallel I/O operations. Instead of holding a global lock when accessing the disk manager, you can issue multiple requests to the disk manager at the same time. This optimization will be very useful in modern storage devices, where concurrent access to the disk can make better use of the disk bandwidth.
   并行I/O操作。在访问磁盘管理器时，您可以发出多个请求，而不是持有全局锁。磁盘管理器同时这种优化在现代存储设备中非常有用， 访问磁盘可以更好地利用磁盘带宽。



## 2. Project2 

2024/4/20

### 1. B+树和哈希表！！！

> 哈希表

+ 内部元数据
+ 核心数据存储
+ 临时的数据结构



> b+树

+ 表索引: 存储开销、维护开销



### 2. B-Tree Family

+ B-Tree
+ B+Tree
+ B*Tree
+ B^link-Tree

### 3. B+Tree

+ 允许搜索、插入、删除。o(log n)
+ 插入要确保叶子节点有足够空间，注意拆分节点
+ 删除要确保叶子节点半满，注意合并节点
+ 允许**顺序访问！！！**

1. **所有值在叶节点**：在B+树中，所有的数据值和数据的指针都只存在于叶子节点中。这与B-树不同，在B-树中，数据和指针分布在所有节点中。
2. **叶节点连接**：B+树的所有叶子节点都通过指针连接成一个链表，这为全范围的**顺序扫描**提供便利。这种结构特别适合执行**范围查询**，因为可以通过遍历链表来访问所有的元素，而不需要回溯到树的上层。
3. **非叶节点作为索引**：非叶节点（内部节点）仅存储它们子节点中元素的最大（或最小）键值以及指向子节点的指针。这使得内部节点可以**存储更多的键**，从而减少了树的高度。
4. **统一的叶节点深度**：B+树保持所有叶节点在同一深度，这意味着每个叶节点从根节点到自己的距离相同，保证了每次搜索的性能均衡。
5. **优化磁盘I/O**：B+树的设计充分考虑了磁盘存储的效率。由于数据仅存储在叶节点，并且叶节点通过指针连接，因此在对数据库进行顺序读取时可以减少磁盘I/O操作。



> key重复的问题:

方法1: 扩充key为<key, (page,slot)>

方法2: 额外的叶子节点



> 设计选择

+ node size：磁盘越慢，节点越大。(能够减少磁盘i/o但会有更多冗余数据)；和文件页大小一致是一种选择。

+ 节点合并的阈值：延迟合并.../考虑合适的大小...
+ 如何处理可变长度的key:  

1. 存储指向元组属性的指针(寻址浪费时间)
2. 节点本身可变长(需要小心地管理内存)
3. 填充key到最大长度(浪费空间)

+ 节点内部搜索...

1. 线性查找(可以接受，瓶颈在于磁盘i/o，不在搜索算法)
2. 二分查找
3. 推断。。。



> 优化方案

+ 前缀压缩: 同一个叶子节点的key很可能有相同的前缀，所以可以把相同前缀单独存储
+ Deduplication: 对于叶子节点里相同的key，只保留一个，把他们的元组一起保存
+ 批量插入: 从一个现存表建立B+树最快的方式就是把key先排序，然后从底部进行构建，而不是一个个插入。



### 4. 聚簇索引

> 聚簇索引

+ **它会将数据行存储在索引结构中，找到了索引也就找到了数据，而无需进行进一步的物理读取。**
+ 一般情况下主键会默认创建聚簇索引，且一张表只允许存在一个聚簇索引。

> 非聚簇索引：

- **将索引结构和数据行分开存储的索引类型**。
- 在非聚簇索引中，索引树的叶子节点只包含索引键值和指向对应数据行的指针，而数据行存储在表的数据页中。当使用非聚簇索引进行查询时，MySQL需要先通过索引树获取到对应数据行的指针，再通过指针访问实际的数据行。因此，非聚簇索引在查找数据时需要进行额外的物理读取，相对聚簇索引来说更慢一些。



### 5. 索引并发控制

#### 1. latches

+ Read/Write modes
+ 

#### 2. 哈希表加锁



#### 3. B+Tree 加锁



#### 4. 叶子节点扫描



### 6.  实验感触...

1. 对于内部节点，第一个kv是无效的，在insert拆分时需要注意，如果新的节点只有一个kv该怎么办，只有这个kv的节点没有意义。其实这种情况只在internal_max_size很小时会发生，比如当其为3时，内部节点拆分为作业两个节点，比如左节点包含0、1，右节点包含2，此时这个右节点就没有意义，不能让上层继续处理。

   我们需要避免这种情况。我的做法是**在逻辑上**不让第一个kv占据size的大小。这样internal_max_size=3时，在物理存储上最多包含四个kv，这时分裂右节点至少有两个kv，是有效的。

   具体的做法可以是：

   1. 从内部节点的Init函数入手，让其max_size等于参数里的size+1(最大时不变)，此时计算size时，第一个kv计算在内。(这样的话，通过绘制相关的函数绘制B+树时，显示出来的key的数目是符合逻辑的)。
   2. 直接在计算size时忽视第一个kv。个人觉得这样处理可能也会影响其他函数。所以用的第一种。





### 7. Debug Tips

- TAs已经为我们准备好了b_plus_tree_printer工具，并且已经准备的Draw/ToString方法，善用它们将B+树可视化，更好的观察插入、删除行为是否正确。 示例：

```cpp
int step = 0;     
for (auto key : keys) {       
  int64_t value = key & 0xFFFFFFFF;       
  rid.Set(static_cast<int32_t>(key >> 32), value);       
  index_key.SetFromInteger(key);       
  tree.Insert(index_key, rid, transaction);       
  tree.Draw(bpm, "SplitTest_step" + std::to_string(step++) + "_insert" + std::to_string(key) + ".dot");     
}     
tree.Draw(bpm, "SplitTest_step.dot");
```

使用 dot 工具在相应文件夹下 `dot -Tpng -O *.dot` ，然后愉快的debug，查看千奇百怪的B+树形状吧

以下命令都可以尝试:)

```
dot -Tpng -O *.dot
```



- debug模式下开启了 sanitizer 是不会产生coredump文件的

如果要查看coredump，请在顶层cmake CXX_FLAGS_DEBUG中把 fsanitize 注释掉

```text
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=${BUSTUB_SANITIZER} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
```

替换为

```
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb)
```

接着 `ulimit -c unlimited`,程序崩溃时就会产生coredump文件。

然后 `gdb -c [exefilename] [corename]`，知道下面几个命令就差不多够了，多线程很有用。

```text
l 查看代码
info threads 查看线程信息
thread [num] 切换线程
bt 查看栈帧
frame [num] 切换栈帧
info locals 查看变量信息   
p xx 打印变量
layout split 同时显示源代码和汇编代码窗口。
```



---

## 3. Project3: 查询执行

### 1. Bustub Shell

```
cd build 
make -j8  shell
./bin/bustub-shell

 \dt: show all tables                                                             
 \di: show all indices                                                            
 \help: show this message again    
```

### 2. Test

```
make -j8 sqllogictest

# Task 1
./bin/bustub-sqllogictest ../test/sql/p3.01-seqscan.slt
./bin/bustub-sqllogictest ../test/sql/p3.02-insert.slt
./bin/bustub-sqllogictest ../test/sql/p3.03-update.slt
./bin/bustub-sqllogictest ../test/sql/p3.04-delete.slt
./bin/bustub-sqllogictest ../test/sql/p3.05-index-scan.slt
./bin/bustub-sqllogictest ../test/sql/p3.06-empty-table.slt

# NestedLoopJoin
./bin/bustub-sqllogictest ../test/sql/p3.07-simple-agg.slt
./bin/bustub-sqllogictest ../test/sql/p3.08-group-agg-1.slt
./bin/bustub-sqllogictest ../test/sql/p3.09-group-agg-2.slt
./bin/bustub-sqllogictest ../test/sql/p3.10-simple-join.slt
./bin/bustub-sqllogictest ../test/sql/p3.11-multi-way-join.slt
./bin/bustub-sqllogictest ../test/sql/p3.12-repeat-execute.slt

# Task 2
./bin/bustub-sqllogictest ../test/sql/p3.14-hash-join.slt
./bin/bustub-sqllogictest ../test/sql/p3.15-multi-way-hash-join.slt

# Task 3
./bin/bustub-sqllogictest ../test/sql/p3.16-sort-limit.slt
./bin/bustub-sqllogictest ../test/sql/p3.17-topn.slt
./bin/bustub-sqllogictest ../test/sql/p3.18-integration-1.slt
./bin/bustub-sqllogictest ../test/sql/p3.19-integration-2.slt
```

### 3. task 1.

#### 1. SeqScan

```
bustub> CREATE TABLE t1(v1 INT, v2 VARCHAR(100));
Table created with id = 15

bustub> EXPLAIN (o,s) SELECT * FROM t1;
=== OPTIMIZER ===
SeqScan { table=t1 } | (t1.v1:INTEGER, t1.v2:VARCHAR)
```

#### 2. Insert

```
bustub> CREATE TABLE t1(v1 INT, v2 VARCHAR(100));

bustub> EXPLAIN (o,s) INSERT INTO t1 VALUES (1, 'a'), (2, 'b');
=== OPTIMIZER ===
Insert { table_oid=15 } | (__bustub_internal.insert_rows:INTEGER)
  Values { rows=2 } | (__values#0.0:INTEGER, __values#0.1:VARCHAR)
```

`InsertExecutor`将元组插入到表中并更新任何受影响的索引。它只有一个子节点生成要插入到表中的值。规划器将确保值与表具有相同的模式。执行器将生成一个整数类型的元组作为输出，指示有多少行已插入到表中。如果有索引与之关联，请记住在插入到表中时更新索引。

提示：您只需在创建或修改`is_delete_`时更改`TupleMeta`字段。对于`insertion_txn_`和`deletion_txn_`字段，只需将其设置为`INVALID_TXN_ID`。这些字段旨在用于未来的学期，我们可能会切换到MVCC存储。

#### 3. Update

```
bustub> explain (o,s) update test_1 set colB = 15445;
=== OPTIMIZER ===
Update { table_oid=20, target_exprs=[#0.0, 15445, #0.2, #0.3] } | (__bustub_internal.update_rows:INTEGER)
  SeqScan { table=test_1 } | (test_1.colA:INTEGER, test_1.colB:INTEGER, test_1.colC:INTEGER, test_1.colD:INTEGER)
```

提示：要实现更新，首先删除受影响的元组，然后插入一个新的元组。不要使用`TableHeap``UpdateTupleInplaceUnsafe`函数，除非你正在为项目4实现排行榜优化。



#### 4. Delete

```
bustub> CREATE TABLE t1(v1 INT, v2 VARCHAR(100));
bustub> EXPLAIN (o,s) DELETE FROM t1;
=== OPTIMIZER ===
Delete { table_oid=15 } | (__bustub_internal.delete_rows:INTEGER)
  Filter { predicate=true } | (t1.v1:INTEGER, t1.v2:VARCHAR)
    SeqScan { table=t1 } | (t1.v1:INTEGER, t1.v2:VARCHAR)

bustub> EXPLAIN (o,s) DELETE FROM t1 where v1 = 1;
=== OPTIMIZER ===
Delete { table_oid=15 } | (__bustub_internal.delete_rows:INTEGER)
  Filter { predicate=#0.0=1 } | (t1.v1:INTEGER, t1.v2:VARCHAR)
    SeqScan { table=t1 } | (t1.v1:INTEGER, t1.v2:VARCHAR)
```

提示：要删除一个元组，你需要从子执行器获取一个`RID`，并更新该元组对应[的`is_deleted_`](https://github.com/cmu-db/bustub/blob/master/src/include/storage/table/tuple.h)的`TupleMeta`字段。

所有删除操作都将在事务提交时应用。



#### 5.IndexScan

你可以通过`SELECT FROM <table> ORDER BY <index column>`测试你的索引扫描执行器。我们将在[任务3](https://15445.courses.cs.cmu.edu/spring2023/project3/#task3)中解释为什么`ORDER BY`可以转换为`IndexScan`。

```
bustub> CREATE TABLE t2(v3 int, v4 int);
Table created with id = 16

bustub> CREATE INDEX t2v3 ON t2(v3);
Index created with id = 0

bustub> EXPLAIN (o,s) SELECT * FROM t2 ORDER BY v3;
=== OPTIMIZER ===
IndexScan { index_oid=0 } | (t2.v3:INTEGER, t2.v4:INTEGER)
```

plan中索引对象的类型在此项目中将始终为`BPlusTreeIndexForTwoIntegerColumn`。您可以安全地将其转换并存储在executor对象中：

```
tree_ = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index_info_->index_.get())
```

然后可以从索引对象构造索引迭代器，扫描所有键和元组ID，从table_heap中查找元组，并按顺序发出所有元组。

BusTub仅支持具有单个唯一整数列的索引。我们的测试用例不会包含重复的键。

注意：我们永远不会在有索引的表中插入重复的行。



### Task 2. 

Aggregation & Join Executors 

聚合连接执行器

在此任务中，您将添加**一个聚合执行器、几个连接执行器**，并允许优化器在规划一个查询时可以在嵌套循环连接(**nested loop join**)和散列连接(**hash join**)之间进行选择。 您将在以下文件中完成实现：

- `src/include/execution/aggregation_executor.h`
- `src/execution/aggregation_executor.cpp`
- `src/include/execution/nested_loop_join_executor.h`
- `src/execution/nested_loop_join_executor.cpp`
- `src/include/execution/hash_join_executor.h`
- `src/execution/hash_join_executor.cpp`
- `src/optimizer/nlj_as_hash_join.cpp`

每个子任务如下所述。

#### 1. Aggregation 聚合

[`AggregationPlanNode`](https://github.com/cmu-db/bustub/blob/master/src/include/execution/plans/aggregation_plan.h)用于支持以下查询：

```
EXPLAIN SELECT colA, MIN(colB) FROM __mock_table_1 GROUP BY colA;
EXPLAIN SELECT COUNT(colA), min(colB) FROM __mock_table_1;
EXPLAIN SELECT colA, MIN(colB) FROM __mock_table_1 GROUP BY colA HAVING MAX(colB) > 10;
EXPLAIN SELECT DISTINCT colA, colB FROM __mock_table_1;
```














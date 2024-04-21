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



#### 检测格式:

```
make format && make check-lint && make check-clang-tidy-p0
```



#### 提交

```
make submit-p0
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

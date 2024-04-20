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



#### **建议的优化**

1. Better replacer algorithm. Given that get workload is skewed (i.e., some pages are more frequently accessed than others), you can design your LRU-k replacer to take page access type into consideration, so as to reduce page miss.
   更好的算法。假设get工作负载是偏斜的（即，某些页面比其它页面更频繁地被访问）， 您可以在设计LRU-k缓存器时考虑页面访问类型，以减少页面丢失。
2. Parallel I/O operations. Instead of holding a global lock when accessing the disk manager, you can issue multiple requests to the disk manager at the same time. This optimization will be very useful in modern storage devices, where concurrent access to the disk can make better use of the disk bandwidth.
   并行I/O操作。在访问磁盘管理器时，您可以发出多个请求，而不是持有全局锁。磁盘管理器同时这种优化在现代存储设备中非常有用， 访问磁盘可以更好地利用磁盘带宽。



## 2. Project2 

2024/4/20




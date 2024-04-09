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
make format
make check-lint
make check-clang-tidy-p0
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



## homework1:

placeholder directory





## 2. Project 2 buffer pool

2024/4/9








1.Project 0 primer
Remove():
bug1: 每次遍历key的过程都会clone(),而每次对于值节点的clone()都会获得一个is_value_node_为true的值节点，但是这个字段未必为true，即使它是一个值节点。主要是因为clone()返回的都是基类的指针，我们并不知道实际的类型。clone()后要判定一下原来的is_value_node避免值节点恢复的情况。

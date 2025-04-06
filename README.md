# Deque

SJTU JOHN class's project deque
# Deque README


## 项目概述

在小作业中，大家已经使用过STL的 `std::deque` 的头尾插入删除功能，实际上，`std::deque` 也支持类似普通数组那样的随机下标访问。


<img src="https://www.oreilly.com/api/v2/epubs/9781787120952/files/assets/fd7f0c6e-e5cb-400d-ad2f-c38e91772682.png" width="500">

在这次大作业中，你需要使用分块链表（Unrolled Linked List）数据结构实现高效的双端队列（Deque），支持快速随机访问与动态容量调整。所需要实现的接口在给定的文件中。

## 核心要求

分块链表就是对存储元素进行分块，以加速随机访问等操作。理想情况下，每一块的大小在 $O(\sqrt n)$ 量级。一种保证方法是在块过大时分成两块，相邻两块均很小时合成一块。在具体实现中，每一
块内部的储存方式和所有块的储存方式均可使用链表或数组。 你需要保证头尾插入和删除的均摊
复杂度是 $O(1)$，随机插入、删除和查询的复杂度是最坏 $O(\sqrt n)$。

这次的大作业不允许使用STL容器，但你可以使用你上次写的双向链表，直接粘贴进 `src.hpp` 即可。这次任务的几乎所有测试都在下发的文件中，其中带有 memcheck 的测试仅是其原始测试数据更少
的版本，可用作检查内存泄漏和内存错误。

我们在此提供两种可以考虑的实现方式：一种是链表套链表，另一种的链表套循环数组，一般来说后一种实现的效率会更高一些。**注：你的实现必须包括动态调整块长的策略，不允许固定块长**

## 项目结构

```
.
├── tests/
│   ├── one/             # 功能测试用例
│   ├── two/      
│   ├── three/      
│   ├── four/      
│   ├── two.memcheck/    # 内存检查专用测试
│   └── four.memcheck/    
├── (various utility hpp files...)
├── README.md            # 你给deque写的文档
└── deque.hpp            # 你的deque实现
```

## 对于 `README.md` 的要求

你需要在 `README.md` 中给出自己分裂和合并的策略，并说明为什么时间复杂度符合要求。

**Note: 如果说明非常清晰，会有额外1-5分加分**

另附：最好认真写，因为我们 CR 的时候一定会问

## 时间安排

这次大作业的发布时间为2025年3月10日，截止时间是2025年4月7日

## 策略说明

我采用分块链表的数据结构实现双端队列(deque)，具体的实现方式是链表套循环数组。
代码中，Block是块，它存储了上一个块和下一个块的地址，并且内含一个循环数组来存储信息。同时，我采取了动态调整块长的策略，在满足一定条件的情况下会合并或者分裂块。

首先分析头尾插入和删除。

在deque中，存储了头块和尾块的地址。

头插入时，访问头块，并且在循环数组未满时，直接将循环数组的head减1，并将插入的数据存储在新的head的位置。如果头块已满，则若头块的大小偏小，就将头块的大小翻倍，否则就将头块分裂成两个块并分配一半的元素到新增的块中。上面两种情况下的时间复杂度都是O($\sqrt{n}$),但是由于每进行O($\sqrt{n}$)次插入才需要分裂或者倍增，均摊时间复杂度是O(1)。

删除同理。将head减1就行，时间复杂度O(1)。

尾插入与头插入相似，访问尾块并且将信息存储在循环数组tail的位置处然后将tail++。如果尾块已满，采用与头块相同的策略，因此均摊时间复杂度也是O(1)。

尾删与头删完全一致，不再赘述。


再来分析随机插入、删除和查询。

随机下标访问时，已知pos去查找该元素。首先从头块开始，根据块内的元素数量先找到pos所处的块，然后下标访问该块的循环数组即可得到该元素。由于块长和块的数量都在O($\sqrt{n}$)量级，随机下标访问的复杂度是最坏O($\sqrt{n}$)。

随机插入时，已知插入处的迭代器，可以访问迭代器所在的块。为了插入元素，将循环数组中在插入位置及以后的元素全部向后移动一格，然后将新的元素插入到插入位置即可。由于块长是O($\sqrt{n}$)量级，最坏情况下移动最多的元素时间复杂度仍然有O($\sqrt{n}$)。

随机删除时，将删除位置后面的元素全部向前移动一格就可以了，时间复杂度同样是O($\sqrt{n}$)。


但是上面的时间复杂度分析都有一个前提，即块长是O($\sqrt{n}$)数量级。所以，我们需要设计分裂和合并的策略。

我采取的策略是：

（1）如果某个块的大小大于8 $\sqrt{n}$,就将其分裂为两个新块，每个新块储存一半的元素

（2）如果某个块与前面一个块的大小之和小于 $\sqrt{n}$,就将两个块合并为一个新块。

其中的分裂和合并操作比较耗时。由于块长保持在O($\sqrt{n}$)量级，这两个操作的时间也是 $O(\sqrt{n})$。

但是如果一个块分裂成两个 $4 \sqrt{n}$大小的新块后，显然以他们的大小，不容易和别的块合并，而n减小来造成这两个新块也需要分裂时，元素总量已经变为原来的 $\frac{1}{4}$ (这是在这两个新块的大小都不减小的情况下)，当n很大时，分裂的均摊时间复杂度是 $O(1)$的。

而两个块合并之后的块大小$\sqrt{n}$，以他的大小不容易产生分裂。如果n增大来造成这个块又需要和别的块合并时，元素总量已经变为原来的4倍(这是在新块的大小不增大的情况下)，当n很大时，合并的均摊复杂度是 $O(1)$的。

# Deque

SJTU JOHN class's project

我采用分块链表的数据结构实现双端队列(deque)，具体的实现方式是链表套循环数组。
代码中，Block是块，它存储了上一个块和下一个块的地址，并且内含一个循环数组来存储信息。同时，我采取了动态调整块长的策略，在满足一定条件的情况下会合并或者分裂块。

首先分析头尾插入和删除。

在deque中，存储了头块和尾块的地址。

头插入时，访问头块，并且在循环数组未满时，直接将循环数组的head减1，并将插入的数据存储在新的head的位置。如果头块已满，则若头块的大小偏小，就将头块的大小翻倍，否则就将头块分裂成两个块并分配一半的元素到新增的块中。上面两种情况下的时间复杂度都是O(√n),但是由于每进行O(√n)次插入才需要分裂或者倍增，均摊时间复杂度是O(1)。

删除同理。将head减1就行，时间复杂度O(1)。

尾插入与头插入相似，访问尾块并且将信息存储在循环数组tail的位置处然后将tail++。如果尾块已满，采用与头块相同的策略，因此均摊时间复杂度也是O(1)。

尾删与头删完全一致，不再赘述。

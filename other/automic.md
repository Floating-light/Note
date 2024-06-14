std::memory_order_release
* 保证当前原子操作之前的所有写入操作都在本次原子操作之前完成。
  * 即之前的写入操作不会被重排到后面 

std::memory_order_acquire
* 保证当前原子操作之后的读取操作不会被重排到本次操作之前
* 保证在获取锁（原子操作）之后，可以看到(std::memory_order_release)发布方所作的所有内存写入结果。

std::memory_order_relaxed - EMemoryOrder::Relaxed
* 仅保证单个原子操作的原子性，不保证前后操作的执行顺序
* 没有顺序约束，更高效

std::memory_order_seq_cst - EMemoryOrder::SequentiallyConsistent
* 不仅保证原子操作的原子性，还保证所有操作按照程序代码的顺序执行，无论哪个线程。
* 确保不同线程上的SC操作能观察到一致的执行顺序，使程序的行为更可预测。
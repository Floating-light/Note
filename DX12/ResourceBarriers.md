---
id: 6mz2sb5hgt9si424e7hnmhf
title: ResourceBarriers
desc: ''
updated: 1671763060642
created: 1671759902831
---

# 目的

* 减少CPU的总体利用率
* enable driver multi-threading
* enable driver pre-processing

example： 
texture的状态，是作为SRV访问还是RTV?

D3D11中由Drivers在背后管理资源的状态。从CPU的角度来看，这是很expensive，在多线程的设计情况下会非常复杂。Direct3D12中由App用`ID3D12GraphicsCommandList::ResourceBarrier`管理资源的状态转换。

# Using the ResourceBarrier API to manage per-resource state
ResourceBarrier通知Driver，同步对一个资源内存的访问。用ResourceBarrierDescription描述一个资源的的状态要怎么转变。

有三种ResourceBarrier，对应`D3D12_RESOURCE_BARRIER`中的union。

* TransitionBarrier Resource的usage状态的转变。用D3D12_RESOURCE_TRANSITION_BARRIER表明资源转换前后的状态。系统会验证同一个commandList中的转换是连续的。使用flag `D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES`表明这个Resource中所有子资源都需要转换。

* Aliasing barrier 两个资源映射到同一Heap，而且还有重叠部分，AliasingBarrier可以过渡当前哪一个Resource处于使用状态。`TiledReousrce`, `VolumeTiledResource`。

* Unordered access view(UAV)barrier 表示所有对这个Resource的读写访问在下次读写之前都必须完成。相同的操作之间不用插入UAVBarrier, 两个Draw都只会读这个Resource，或者只会写Resource。

* https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12

# 细节
假设一个D3D12_RESOURCE_BARRIER_TYPE_TRANSITION，从D3D12_RESOURCE_STATE_COPY_DEST转换到D3D12_RESOURCE_STATE_GENERIC_READ：

1. 完成所有待处理的写入操作（D3D12_RESOURCE_STATE_COPY_DEST）：
   * GPU 确保所有当前排队的对资源的写入操作已完成。这包括所有将要写入 COPY_DEST 状态资源的数据，都必须完成。这一步保证了在进入下一个状态之前，数据的一致性和完整性。
2. 保证数据可见性：
   * 对于 COPY_DEST 状态的资源，确保数据写入完成后可供接下来的读取操作使用。这可能涉及对缓存的冲刷或同步，以确保数据对于后续操作完全可见。
3. 状态标记更新：
   * 将资源的内部状态标记从 D3D12_RESOURCE_STATE_COPY_DEST 更新为 D3D12_RESOURCE_STATE_GENERIC_READ。这在内部数据结构中进行更新，以确保对资源后续访问的正确性。
4. 确保读取操作(StateAfter)的同步：
   * 确保在 D3D12_RESOURCE_STATE_GENERIC_READ 状态下，可以安全地读取数据。这个状态下可能包含多个部分类型，比如着色器资源视图（SRV）、常量缓冲视图（CBV）等。
5. 继续执行后续命令
   * 在资源屏障完成后，GPU 将继续处理命令列表中的后续命令。在这一点上，资源已经处于 D3D12_RESOURCE_STATE_GENERIC_READ 状态，并且可以安全地进行读取操作。

# BEGIN_ONLY 和 END_ONLY
还有两种特殊的转换方式`D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY`和 `D3D12_RESOURCE_BARRIER_FLAG_END_ONLY`。分别只关系资源的StateBefore和StateAfter相关的操作。

* D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
  * 保证StateBefore状态下的操作全部完成，且这些操作造成的影响对后续的操作都是可见的。
  * 不保证资源一定处于StateAfter
  * 此后对资源进行操作可能存在不可预见的行为
  * 需要逻辑上保证后续操作只有假定资源状态是StateAfter的操作。
* D3D12_RESOURCE_BARRIER_FLAG_END_ONLY
  * 会在假设资源的StateBefore相关的操作都已经转换完成。
  * 确保资源最后处于StateAfter的状态

这两个标记将一个完整的资源转换过程分成两部分，在进行完`BEGIN_ONLY`后，就可以进行一些复杂的计算任务，这些任务必须确保只对资源进行StateAfter状态下的操作，因为状态转换并没有完全完成。最后再进行`END_ONLY`的转换，把资源转换成一个稳定的目标状态（StateAfter），这次转换的StateBefore是上次`BEGIN_ONLY`的StateAfter，StateAfter可以是你期望的任何状态。
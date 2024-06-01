---
id: 6mz2sb5hgt9si424e7hnmhf
title: ResourceBarriers
desc: ''
updated: 1671763060642
created: 1671759902831
---

## 目的

* 减少CPU的总体利用率
* enable driver multi-threading
* enable driver pre-processing

example： 
texture的状态，是作为SRV访问还是RTV?

D3D11中由Drivers在背后管理资源的状态。从CPU的角度来看，这是很expensive，在多线程的设计情况下会非常复杂。Direct3D12中由App用`ID3D12GraphicsCommandList::ResourceBarrier`管理资源的状态转换。

## Using the ResourceBarrier API to manage per-resource state
ResourceBarrier通知Driver，同步对一个资源内存的访问。用ResourceBarrierDescription描述一个资源的的状态要怎么转变。

有三种ResourceBarrier，对应`D3D12_RESOURCE_BARRIER`中的union。

* TransitionBarrier Resource的usage状态的转变。用D3D12_RESOURCE_TRANSITION_BARRIER表明资源转换前后的状态。系统会验证同一个commandList中的转换是连续的。使用flag `D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES`表明这个Resource中所有子资源都需要转换。

* Aliasing barrier 两个资源映射到同一Heap，而且还有重叠部分，AliasingBarrier可以过渡当前哪一个Resource处于使用状态。`TiledReousrce`, `VolumeTiledResource`。

* Unordered access view(UAV)barrier 表示所有对这个Resource的读写访问在下次读写之前都必须完成。相同的操作之间不用插入UAVBarrier, 两个Draw都只会读这个Resource，或者只会写Resource。

* https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
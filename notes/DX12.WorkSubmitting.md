---
id: y227cvhqe3mkhe69n4j8wy6
title: WorkSubmitting
desc: ''
updated: 1668171641477
created: 1668169030699
---

# Work Submission in Direct3D12

为了更好地支持RenderingWork地复用和多线程的灵活性，需要D3D App提交渲染任务的方式有根本性地改变。与之前的方式有三方面不同：

1. 淘汰*ImmediateContext*，支持了多线程。
> 立即上下文是应用程序主线程向 GPU 发送命令的上下文，立即上下文提交的命令直接提交 GPU 的命令队列中，在创建设备的同时创建它。

每个CommandList的API看起来像Direct3D11d的渲染方法，也和immediate contexts一样，不是线程安全的。但是可以创建多个Commandlist，同时记录命令。

2. App决定如何把渲染调用组织成GPU的工作项。这使得支持重用渲染命令。

利用如今的硬件特性，增加了Bundles:

* First level commandlist -- *direct command lists*
* Second level commandlist -- *bundles*

*Bundles*的目的是为了把少量的API命令Goup起来，可以在*DirectCommandList*中重复执行。创建Bundle时，Driver会执行尽可能多的预处理，使后面执行时更高效。可以在多个Commandlist中执行，也可以在同一个Commandlist中执行多次。

[[Bundle也有许多限制|DX12.D3D12#bundle-restrictions]]。

3. App显式控制什么时候把Work提交给GPU。



### Reference 

[图形程序接口简介](https://zhuanlan.zhihu.com/p/452013032)
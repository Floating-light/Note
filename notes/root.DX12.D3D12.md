---
id: otmzefuwcx3rt3twfca5n0f
title: D3D12
desc: ''
updated: 1667997319956
created: 1667868747835
---

> https://learn.microsoft.com/en-us/windows/win32/direct3d12/interface-hierarchy

![](/assets/images/Direct3D12.png)

# ID3D12Device

代表虚拟的Adapter，用于创建各种D3D对象：

* Command allocators
* Command lists
* command queues
* Fences
* Resources
* Pipeline state objects
* Heaps
* Root signatures
* Samplers
* Many resource views

创建ID3D12Device:
```c++
HRESULT WINAPI D3D12CreateDevice(
    _In_opt_ IUnknown* pAdapter,
    D3D_FEATURE_LEVEL MinimumFeatureLevel,
    _In_ REFIID riid, // Expected: ID3D12Device
    _COM_Outptr_opt_ void** ppDevice );
```
* pAdapter是DXGI的对象，从DXGIFactory创建。
* MinimumFeatureLevel 要求Device满足的最小[FeatureLevel](https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_feature_level)，这将决定支持不同的ShaderModel。
* riid和ppDevice是COM接口的常规操作。ppDevice如果为空，返回结果将表明这个Adapter是否支持这个FeatureLevel。
```c++
// Check to see whether the adapter supports Direct3D 12, but don't create the
// actual device yet.
if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
{
    break;
}
```

# ID3D12CommandQueue

* 提交CommandLists
* 同步CommandList的执行
* instrumenting the command queue
* 更新Resource tile mappings

创建：
```c++
// Describe and create the command queue.
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // 可以Disable GPU timeout
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;// 可执行的CommandList类型

ComPtr<ID3D12CommandQueue> m_commandQueue;
m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
```
D3D12中还可用于创建DXGI的SwapChain。

提交CommandLists:
```c++
// Execute the command list.
ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
```
提交后并不一定会立即执行，稍后才会把命令给GPU执行。而且这一过程较慢，应该尽量记录更多的Command的，减少提交次数。

还可以通过Fence同步GPU的执行：
```c++
void D3D12HelloTriangle::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
```
这里m_fenceValue总是保存的下一个可用的信号值。`ID3D12CommandQueue::Signal`会保证前面提交的Commandlist都执行完之后才会发出这个信号。随后绑定一个事件到这个Fence，并等待它。

 # ID3D12CommandAllocator

* 创建Allocator的LIST_TYPE必须和CommandList的类型相同
* 一个Allocator最多和一个正在记录Command的List关联
* 但是一个Allocator可以用于创建任意数量的GraphicsCommandList对象
* ID3D12CommandAllocator::Reset回收内存，可用于新的CommandList，但底层Size不变
* 执行Reset之前App必须保证GPU不在执行和这个Allocator关联的任何CommandList。
* 非线程安全

 # Bundles

CommandList和Bundles都可以记录Drawing和State-Changing calls,稍后给GPU执行。

[Bundle Feature](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)：
* 通过添加二级CommandList利用GPU硬件中存在的功能
* Bundle的目的是使App可以把少量的Commands组合在一起执行
* Bundle在创建时，Driver会执行尽可能多的预处理使得稍后的执行性能消耗更少。
* Bundle被设计成可以被多次复用（通常CommandList只执行一次，但只要App保证前一次执行已经完成了，也可以再次执行）。

通常，一些D3D的API调用组成Bundles，API calls和Bundles组成CommandLists，CommandLists组成一帧。下图中Commandlist1和Commandlist2复用了Bundle1。

![](/assets/images/BundleToAFrame.png)

## Creating command list

有两个方法可以创建CommandList：

* ID3D12Device::CreateCommandList
* ID3D12Device4::CreateCommandList1
`ID3D12Device4::CreateCommandList1`可以创建一个已经Close的CommandList，而且也避免了传入Allocator和PSO。

用`ID3D12Device::CreateCommandList`创建时可以传入PSO，但这不是必须的。通常一个App会在初始化时创建大量PSO，然后用`ID3D12GraphicsCommandList::SetPipelineState`改变当前绑定的PSO。See:[ManagingGraphicsPipelineState](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12).

## Recording command lists
`CreateCommandList`创建的Commandlist立即处于recording状态。

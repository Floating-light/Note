Descriptor，也称作View，用于描述ID3D12Resource。

# Descriptor

是一块相对比较小的数据，用于充分描述一个GPU对象。

Descriptor的Size和硬件有关，可以调用`ID3D12Device::GetDescriptorHandleIncrementSize.`获取对应Descriptor的size。

Driver不会持有Descriptor的引用，由App保证Descriptor的类型和信息是正确的。

Descriptor[不需要被释放](https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors-overview#descriptor-data)。

Descriptor，也称作View，各种Buffer的View，Texture的View，Sampler的View等，都被保存在DescritptorHeap中。

为了引用DescriptorHeap中的Descriptor，可以从Heap中创建DescriptorHandle。每个DescritporHeap中的元素都有两种Handle，CPUDescriptorHandle和GPUDescriptorHandle。它们都引用对应的Descriptor，是同一个Descriptor的两种引用方式。

CPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE)：
* 用于在CPU端对Descriptor进行管理和操作，更新，Copy。
* 在CPU端通过这个Handle设置DescriptorHeadp中的View。
```c++
CreateShaderResourceView()
CreateUnorderedAccessView()
CreateRenderTargetView()
CreateDepthStencilView()
CreateConstantBufferView()
CopyDescriptorsSimple()

OMSetRenderTargets()
ClearDepthStencilView()
ClearRenderTargetView()
```
GPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE):
* 用于在GPU端对Descriptor进行访问和使用。
* 用于GPU端绑定DescriptorTable以进行实际的渲染操作。
* GPU通过它访问Descriptor引用的底层资源
* 通常在执行渲染阶段使用。

```c++
SetGraphicsRootDescriptorTable()
SetComputeRootDescriptorTable()
```

```c++
CPU和GPU Handle都需要：
CopyDescriptors()
ClearUnorderedAccessViewUint()
ClearUnorderedAccessViewFloat()
```

为什么有两种Handle，不统一使用一种？

> 效率和吞吐量: GPU 的指令吞吐量和并行处理能力非常强，但它的上下文切换以及与 CPU 直接交互的开销非常大。这意味着频繁在 GPU 上执行资源管理操作会极大地降低系统性能。相反，CPU 端做这些操作会更有效率。

> 异步执行: GPU 和 CPU 可以并行工作，但它们之间的通信需要通过命令队列进行，这是一个异步过程。如果让 GPU 也负责资源管理，会导致频繁的同步操作，从而影响异步执行的效率。

> 统一使用一个 Handle 意味着描述符的创建和绑定都依赖一个共享的句柄空间。这会导致频繁的同步问题，影响指令流的高效性。

CPUDescriptorHandle 和 GPUDescriptorHandle 可以看作是同一描述符堆在不同执行端（CPU 和 GPU）上的引用。这意味着它们可能指向的实际上是存储在同一描述符堆中的相同描述符，但是根据使用上下文不同，引用方式有所区分。为了让 GPU 以高效的方式去处理和访问资源，这些描述符堆通常驻留在 GPU 内存中或是在 GPU 可访问的区域内。

在CPU端初始化Descriptor，更新，复制等操作时，其实是在CPU端高效完成的。然后，需要有某种机制（如命令队列和堆栈）将这些更新"提交"给 GPU。这实际上是通过指令流和同步机制来实现的。在 CPU 创建好描述符之后，通常要通过命令队列将这些描述符提交给 GPU。GPU 在执行这些命令队列时，将会通过 GPUDescriptorHandle 访问这些描述符。这个过程保证了在 GPU 使用资源之前，这些资源已经在 CPU 端正确地初始化和更新。

在GPU使用DescriptorHeap中的Descriptor期间，必须确保该DescriptorHeap不会被CPU修改。通常通过CommandList的同步机制保证。

# D3D12_GPU_VIRTUAL_ADDRESS
ID3D12Resource有个方法`GetGPUVirtualAddress`可以直接得到一个资源的GPU地址。这个地址可以用于直接绑定到CommandList：
```c++
virtual void  SetGraphicsRootShaderResourceView( 
    _In_  UINT RootParameterIndex,
    _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
virtual void  SetGraphicsRootUnorderedAccessView( 
    _In_  UINT RootParameterIndex,
    _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) = 0;
```
直接用GPU地址绑定，提供了最低的使用开销。适合频繁更新的小资源，如`ConstantBuffer`，`StructuredBuffer`。

用DescriptorTable，尽管引入了一些额外的间接层，但其优势在于高度的灵活性和通用性。使得复杂的着色器资源管理变得更加容易，View的管理更加规范化。
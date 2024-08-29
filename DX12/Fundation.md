## Createing a basic Direct3D 12 component

1. Vertex Data and Input layout description
在CPU端将单个顶点的数据存储在一块连续内存空间(通常是一个结构体)，然后需要向GPU描述其内容组成(InputLayoutDescrition).
```c++
typedef struct D3D12_INPUT_LAYOUT_DESC
{
    const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
    UINT NumElements;
}D3D12_INPUT_LAYOUT_DESC;
```
其中, 结构体中有多少个数据成员，D3D12_INPUT_LAYOUT_DESC中就有多少个`D3D12_INPUT_ELEMENT_DESC`，即结构体中每一个成员都对应一个`D3D12_INPUT_ELEMENT_DESC`。
```c++
typedef struct D3D12_INPUT_ELEMENT_DESC
    {
    LPCSTR SemanticName; // VertexShader中输入参数的名字
    UINT SemanticIndex; // 每一个名字默认为数组，这即为它的索引，
    DXGI_FORMAT Format; // GPU解释这个元素的格式，DXGI_FORMAT_R32G32B32_FLOAT
    UINT InputSlot; // https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage-getting-started
    UINT AlignedByteOffset; // 在绑定到特定Inputslot的顶点数据c++Struct中，当前表示的元素的首地址偏移量
    D3D12_INPUT_CLASSIFICATION InputSlotClass;// D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA（另一个用于实现Instanceing）
    UINT InstanceDataStepRate; // 0， 用于实现instancing
    } 	D3D12_INPUT_ELEMENT_DESC;
```
![InputLayoutDesc](./InputLayoutDesc.png)
`D3D12_INPUT_ELEMENT_DESC`将顶点数据结构体中的一个元素映射到顶点着色器中的一个输入参数。SemanticName和SemanticIndex描述了一个VertexShader中的参数，SemanticName是这个参数名，SemanticIndex是索引，即认为有多个同样SemanticName的参数，在VertexShader输入参数中直接在相应SemanticName后加上索引即可，为0可以不加。

这通常和PS，VS shader 一起设置在`D3D12_GRAPHICS_PIPELINE_STATE_DESC`中。

2. ID3D12Resource
D2D12中，所有的资源均用ID3D12Resource对象表示。包括VertexBuffer和纹理。

创建时构建一个D3DX12_RESOURCE_DESC描述要创建的资源，其中D3DX12_RESOURCE_DESC::D3D12_RESOURCE_DIMENSION用于区分Buffer和Texture，调用`ID3D12Device`的方法（ID3D12Device::CreateCommittedResource）创建ID2D12Resource。

CD3DX12_RESOURCE_DESC有一些方法只需要简单的几个必要的参数即可创建D3DX12_RESOURCE_DESC，可以简化Desc的创建过程。

这些资源通常位于不同类型的堆中（物理内存池）。
https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
```c++
D3D12_HEAP_TYPE_DEFAULT : GPU访问快，读写, 但是CPU无法访问。大多数堆和资源都在这儿，通常通过Upload堆中的资源初始化
D3D12_HEAP_TYPE_UPLOAD ：优化的CPU访问，但GPU访问效率不高。通常用于 CPU-write-once, GPU-read-once data。
D3D12_HEAP_TYPE_READBACK ： 用于CPU从GPU读回数据。GPU-write-once, CPU-readable data.
D3D12_HEAP_TYPE_CUSTOM
```
通常，一些静态的物体每一帧都不会发生改变，应该被保存在DefaultHeap中，但DefaultHeap又不可被GPU读取，所以还要先把数据传到UploadHeap，由它把资源上传至GPU，再复制到DeafultHeap中。

这一过程需要调用一系列D3D的函数：

1. 创建这两种Buffer(UploadHeap,DefaultHeap),其中这两种Buffer的状态：
    * DefaultBuffer D3D12_RESOURCE_STATE_COPY_DEST，因为稍后要把buffer Copy到这儿来
    * UploadBuffer D3D12_RESOURCE_STATE_GENERIC_READ， 稍后从中读取Buffer copy到DefaultBuffer中
2. 将真正的Buffer数据用D3D12_SUBRESOURCE_DATA表示
3. 借助d3dx12.h中的UpdateSubresources<1>方法, 先调用ID3D12Device::GetCopyableFootprints获取Dest的Footprints
    * 调用UpdateSubresources()
    * 将Buffer数据读到中间UploadHeap的Resource中(ID3D12Resource::Map())
    * 然后调用ID3D12GraphicsCommandList::CopyBufferRegion或ID3D12GraphicsCommandList::CopyTextureRegion将UploadHeap中的Resource复制到DefaultHeap中的Resource
4. 把DefaultBuffer中的Resource的状态转换为我们期望的用途(D3D12_RESOURCE_STATE_INDEX_BUFFER)
    * 创建Barrier, CD3DX12_RESOURCE_BARRIER::Transition()
    * ID3D12GraphicsCommandList::ResourceBarrier()
5. 完成上述操作后，必须在得知复制命令已经完成之后(Fence)才能释放uploadBuffer. 
* Descriptor heap 
An array of descriptors. Where each descriptor fully describes an object to the GPU.

* Command allocator 
A command allocator manages the underlying storage for command lists and bundles.

* Root signature 
Defines what resources are bound to the graphics pipeline .

* Pipeline state object 
A pipeline state object maintains the state of all currently set shaders as well as certain fixed function state objects(such as the input assembler, tesselator, rasterizer and output merger)

* Fence 
A fence is used to synchronize the CPU (see Multi-engine synchronization)


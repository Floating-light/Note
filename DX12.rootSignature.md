---
id: i9pbjvbh3hxrmnmfshnc1ab
title: rootSignature
desc: ''
updated: 1669339498998
created: 1668429269162
---

*RootSignature*可以绑定一些每一帧都会发生变化的少量数据，并针对这一点进行了优化。

*RootSignature*是一种由App定义的资源绑定转换，Shader需要用它定位要访问的资源，RootSignature可以存储：

[GraphicsPipeline](https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control#resources-and-the-graphics-pipeline)

* 指向`descriptor heap`的`descriptor tables`的索引,在那里定义了这个`descriptor table`的`layout`。Descriptor。
* 直接绑定一些用户定义的常量，而不用通过*descriptors*和*descriptor tables*。
* 根签名中直接包含非常少量的*descriptor*，例如每次绘制都会更改的常量缓冲区视图 (CBV)，从而使应用程序无需将这些描述符放入描述符堆中。

*Root signature*需要用一组*ROOT_PARAMETER*，描述了Shader的要绑定的全局参数:

![](/assets/images/RootSignatureDesc.png)

Parameter可以指定三种类型的参数：

![](/assets/images/RootParameter.png)

## D3D12_ROOT_DESCRIPTOR_TABLE 

表示了在同一个DescriptorHeap上的一组连续范围内的Descriptor。因为CBV，SRV，UAV可以放到同一个descritpor heap上，所以一个descriptor table可以同时容纳这三种Range。而SAMPLER只能在SAMPLER的heap上，所以只能单独用一个Descriptor table表示。

![](/assets/images/DescriptorTable.png)

向图形管线设置DescriptorTable，必须先绑定Heaps，然后再设置这些Heap所对应的Ddescriptor table的索引，即它所在ROOT_PARAMETER的索引。

这里每种类型的Heap最多只能同时存在一个，即同时最多绑定两个Heap，一个CBV_SRV_UAV,一个SAMPLER。

![](/assets/images/BindDescriptorHeap.png)

调用*SetGraphicsRootDescriptorTable*后，从BaseDescriptor开始的，由RootParameterIndex指向的DescriptorTable描述的这么多Descriptor将会被用于这个RootParameter。

改变descriptor heap在一些硬件上可能导致pipeline flush， 所以建议每种类型的heap只存在一个，这样一帧就只用设置一次， 而不是频繁修改它。可以先将descriptor存在一些和shader无关的地方，然后再拷贝到这两个Heap中。

## D3D12_ROOT_CONSTANTS

它只指定了一些常量：

![](/assets/images/CONSTANTS.png)

直接用Commandlist的API`SetGraphicsRoot32BitConstant()`就可以绑定。

## D3D12_ROOT_DESCRIPTOR

这可以直接绑定一个Descriptor，而无需通过DescriptorTable：

![](/assets/images/Descriptor.png)

用Commandlist的方法直接绑定：

```c++
SetGraphicsRootConstantBufferView()
SetGraphicsRootShaderResourceView()
SetGraphicsRootUnorderedAccessView()
```

![](/assets/images/Family.png)

# Root Signatures Overview

* 可以在HLSL代码中定义RootSignaturesDesc，然后通过*D3D12CreateRootSignatureDeserializer*创建一个反序列化接口，最终可以得到RootRignaturesDesc。

* RS中每个参数类型都消耗固定空间：
  * Descriptor table: 1 DWORD
  * Root Constant: 1 DWORD
  * Root descriptor: 2 DWORDs
总共不能超过64DWORD。

## Root Constants

Root Constants是一堆32-bit的数据的合集。在HLSL中以constant buffer的形式出现。真正的ConstantBuffer是以4×32bit为基本单位的数据集合。

每个RootConstant Set当作一个32-bit数组，shader对这种数据是只读的，可以动态索引，越界访问会产生未定义行为。在HLSL中，可以为Constants提供数据结构定义以赋予它类型。例如，如果RootSignature定义了一组4个RootConstant，HLSL可以这样解释这四个数据：
```c++
struct DrawConstants
{
    uint foo;
    float2 bar;
    int moo;
};
ConstantBuffer<DrawConstants> myDrawConstants : register(b1, space0);
```
因为RootSignature空间不支持动态索引，所以在HLSL中，不能把映射到RootConstant的数据定义为数组，即定义一个*float myArray[2];*是非法的，即映射到RootConstants的ConstantBuffer它自己不能是一个数组。

Constants可以部分设置。

当设置Constants时需要注意shader所期望的ConstantBuffer layout，constants可能会被padded到vec4的边界。可以在HLSL中通过检查反射信息验证layout。

* SetGraphicsRoot32BitConstant
* SetGraphicsRoot32BitConstants
* SetComputeRoot32BitConstant
* SetComputeRoot32BitConstants

## Root descriptor
Pros:
* 方便，直接绑定Descriptor，不用遍历Heap
Convs:
* 占用RootSignature空间，2DWORD.

通常可以把每次绘制都变化的ConstantBufferView放这儿，这样不用每次都Allocate descriptor heap space，也不用把Descriptor table指向新的位置。直接把descriptor保存在RootSignature中，App就只需要关心把什么View直接绑定到Pipeline即可，而不用维护一个CPU的descriptor heap。

可以直接放到RootSignature的Descritpor:
* Constant buffer view.
* Shader resource views,unordered access views,不能有格式转换。
  * 可以和RootSingnature绑定的：
    * StructuredBuffer\<type>
    * RWStructuredBuffer\<type>
    * ByteAddressBuffer
    * RWByteAddreddBuffer
  * 不能绑定的：
    * Buffer\<uint>
    * Buffer\<float2>
* Raytracing acceleration structures的SRVs。


## Example 
一个RootConstant参数，消耗1个DWORD：

![](/assets/images/OnConstantBind.png)

API slot 是由root signature中参数的顺序决定的。

增加一个Root constant buffer view（2 DWORD）：

![](/assets/images/AddCBV.png)

绑定Descritpor table：

![](/assets/images/BindingDescriptorTable.png)

一个float4 Root constant占用4个DWORDS大小，而且可以仅设置其中几个DWORDS：
```c++
pCmdList->SetComputeRoot32BitConstants(0,2,myFloat2Array,1);  // 2 constants starting at offset 1 (middle 2 values in float4)
```
这里仅设置了中间两个数据。

更复杂的RootSignature：

![](/assets/images/RootSignature_MoreComplex.png)

* Slot3,slot6的descriptor table包含了unbounded size 数组。
  * DirectX-Graphics-Samples\Samples\Desktop\D3D12DynamicIndexing\src
  * hardware tier 2+

* Slot9中的UAV u4和UAV u5以同样的offset声明，这表明u4和u5都绑定到同一个descriptor。

### Streaming Shader Resource Views

![](/assets/images/StreamingShaderResourceViews.png)

这张图说明了一种应用场景，所有SRV都放在一个大数组中，在Shader中，通过传入的CBV等常量读取需要Texture。
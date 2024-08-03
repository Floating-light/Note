* [Creating a Custom Mesh Component in UE4 | Part 1: An In-depth Explanation of Vertex Factories](https://medium.com/realities-io/creating-a-custom-mesh-component-in-ue4-part-1-an-in-depth-explanation-of-vertex-factories-4a6fd9fd58f2)

* [Creating a Custom Mesh Component in UE4 | Part 0: Intro](https://medium.com/realities-io/creating-a-custom-mesh-component-in-ue4-part-0-intro-2c762c5f0cd6)
* [Global Uniform Shader Parameter(1)](https://medium.com/@solaslin/learning-unreal-engine-4-adding-a-global-shader-uniform-1-b6d5500a5161)
* https://outerra.blogspot.com/2012/05/procedural-grass-rendering.html
* https://gdcvault.com/play/1026177/Instancing-and-Order-Independent-Transparency

* [Shader参数绑定](https://zhuanlan.zhihu.com/p/485239547)
  * 在HLSL代码中可以不显示指定资源的绑定位置register(t0)
  * 编译时会自动生成
  * 然后用ID3D12ShaderReflection获取编译好的Shader代码的参数绑定信息
    * 可以拿到参数名字，绑定Space，BindPoint等所有信息。
  * 在C++端就可以动态决定资源的绑定位置
* [graphics-programming-overview-for-unreal-engine](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/graphics-programming-overview-for-unreal-engine)


最近发现一个插件，实现了DrawInstanced SkeletalMesh。这里需要实现自定义的`UPrimitiveComponent`和对应的`FPrimitiveSceneProxy`、`FVertexFactory`等。本文首先简要介绍实现自定义`UPrimitiveComponent`需要的基本操作，然后分析`Skelot`渲染方面的实现细节。

# FLocalVertexFactory
不同的VertexFactory实现了不同的顶点处理方式，即如何从VertexShader中传进来的Vertex数据得到VS输出给PS的顶点数据。通常，对于StaticMesh，直接把模型的VertexBuffer传给GPU，直接读顶点数据中的Position进行MVP变换即可，复杂一点，如果有顶点偏移，再加上顶点偏移，这就是`UStaticMeshComponent`实现的Mesh渲染方式。此外，如果是`USkeletalMeshComponent`这种骨骼动画的渲染，除了普通Mesh的顶点数据和处理逻辑，在顶点中还需要传入影响当前顶点的骨骼Index和对应的蒙皮权重，然后还需要一个额外的Buffer，传入当前帧的Pose，即每个骨骼对应的位置信息。渲染时，在VertexShader中根据这些数据计算出顶点的最终位置。这种顶点数据和处理逻辑的差异就是由`FLocalVertexFactory`处理的。而`FLocalVertexFactory`就是`UStaticMeshComponent`使用的顶点工厂，后面以它为例分析VertexFactory的工作机制。

VertexFactory
* 创建并绑定VertexBuffer
* 创建并绑定InputLayout
* 创建并绑定VertexShader
* 
[顶点工厂的类图]粗略的继承关系

VertexFactory首先要负责从模型资源获取渲染需要的数据，把它们上传到GPU，即各种GPUBuffer，这些在UE中被抽象成以`FRHI`开头的各种资源，这些资源才可以直接被绑定到渲染管线中。在CPU端这些数据存储在：

这一切自然要从`UStaticMesh`说起，毕竟一个模型导入到UE后，就成为了一个`UStaticMesh`。模型的原始数据保存在`UStaticMesh`的`FStaticMeshSourceModel`：
```c++
TArray<FStaticMeshSourceModel> SourceModels;
```
在`UStaticMesh::Serialize()`中序列化。里面的模型数据仅编辑器下才会存在：
![VF_SerializeSourceModel](../assets/UE/VF_SerializeSourceModel.png)

而真正的渲染数据是`UStaticMesh`中的`FStaticMeshRenderData`：
```c++
TUniquePtr<class FStaticMeshRenderData> RenderData;
```
[补一个FStaticMeshRenderData类图，到VF]

`FStaticMeshRenderData`中，以每个LOD的Mesh数据单独保存为一个`FStaticMeshLODResources`，其中包含了一个LODMesh的VertexBuffer和IndexBuffer等数据，以及对应的MeshSection，不同的Section对应不同的MaterialIndex，引用了不同VertexBuffer和IndexBuffer范围。`FStaticMeshLODResources`相当于是从原始的模型数据`SourceModels`中，提取并重新组织了自己用得到的顶点数据，这些数据仍然是CPU端的。

每个`FStaticMeshLODResources`都对应了一个`FStaticMeshVertexFactories`，里面就是`FLocalVertexFactory`，这里面就有对应的GPUBuffer。

`UObject::PostLoad()`会在对象加载出来之后调用一次，`UStaticMesh::PostLoad()`中触发了`RenderData`的初始化，在编辑器下：
```c++
UStaticMesh::PostLoad()
* UStaticMesh::ExecutePostLoadInternal()
    * UStaticMesh::CacheDerivedData()   
    * 在这里实现了生成`RenderData`。
```
这里会处理从`SourceModels`生成各级`LODResources`中的模型数据，也有可能直接从DDC加载。在打包好的流程中，直接就在`UStaticMesh::Serialize()`反序列化出`RenderData`：

![VF_SerializeRenderData](../assets/UE/VF_SerializeRenderData.png)

此时的`RenderData`只有CPU端的模型数据，`LODResources`和`LODVertexFactories`还没有初始化。随后对`RenderData`进行初始化：

UStaticMesh::PostLoad()
  * UStaticMesh::FinishPostLoadInternal()
    * UStaticMesh::InitResources()
      * FStaticMeshRenderData::InitResources()
      * 初始化所有LODResource和VertexFactory
        * FStaticMeshLODResources::InitResources()
        * FStaticMeshVertexFactories::InitResources()

在`FStaticMeshLODResources::InitResources()`中，对每一种Buffer都调用`BeginInitResource(Resource)`，里面`ENQUEUE_RENDER_COMMAND`创建渲染线程执行的命令，即简单地调用`Resource->InitResource()`：

![VF_BeginInitResource](../assets/UE/VF_BeginInitResource.png)

而`InitResource()`中主要功能就是调用`InitRHI()`方法，从`FRenderResource`派生的Buffer需要实现这个方法，创建当前Resource对应的GPU资源。例如，在`FRawStaticIndexBuffer IndexBuffer`中，

![VF_BeginInitResource](../assets/UE/VF_IndexBuffer.jpg)

当`FStaticMeshLODResources`加载好之后，其中的IndexStorage就是完整的索引数据，但IndexBufferRHI还没有创建，而且RHIBuffer的创建必须在RenderThread中执行。在`FRawStaticIndexBuffer::InitRHI()`中调用`FRawStaticIndexBuffer::CreateRHIBuffer()`创建`IndexBufferRHI`，并将`IndexStorage`上传到GPU。如果是DX12，具体的创建在`FD3D12DynamicRHI::CreateD3D12Buffer()`。

`VertexBuffers`中分了三种Buffer：

![VF_StaticMeshVertexBuffers](../assets/UE/VF_StaticMeshVertexBuffers.png)

这三者都是`FRenderResource`，都会执行一样的初始化流程。
`StaticMeshVertexBuffer`里面有两种顶点数据：

![VF_MeshvertexBufferG](../assets/UE/VF_MeshvertexBuffer.jpg)

其中TangentsData和TexcoorData是原始数据，然后是它们对应的RHIBuffer和SRV。在`FStaticMeshVertexBuffer::InitRHI()`中分别创建这两种Buffer和对应的SRV：

![VF_MeshVertexBufferCode](../assets/UE/VF_MeshVertexBuffer.png)

`PositionVertexBuffer`和`ColorVertexBuffer`也是同样的模式，这里不在赘述。

初始化好`LODResources`后，就可以用它去初始化`LODVertexFactories`，在`FStaticMeshVertexFactories::InitResources()`中：

![VF_VFInitResource](../assets/UE/VF_VFInitResource.png)

而这里的`InitVertexFactory()`也仅仅是将输入的`LodResource`和`VertexFactory`打包成参数，发到RenderThread上进行初始化：

![VF_VFInitRenderThread](../assets/UE/VF_VFInitRenderThread.png)

在`FLocalVertexFactory`中，有一个成员变量`FDataType Data;`，这里主要就是初始化它。`FDataType`继承自`FStaticMeshDataType`：

![VF_VFMeshData](../assets/UE/VF_VFMeshData.jpg)

可以看到`FStaticMeshDataType`中保存的就是对`LODResources`中各种顶点数据RHIBuffer的引用，包括RHIBuffer指针的直接引用和对应的SRV。在渲染命令`ENQUEUE_RENDER_COMMAND(InitStaticMeshVertexFactory)`中，直接创建一个`FDataType`，用`LODResource`中各个Buffer的`BindXXX`方法构建`FDataType`中的内容，Position的初始化：

![VF_RefenecePositionData](../assets/UE/VF_RefenecePositionData.png)

都是直接引用`LODResource`中这些资源创建的RHIBuffer和SRV。其它的也是一样。

然后调用`FLocalVertexFactory::SetData()`设置上数据，紧接着就调用`FLocalVertexFactory::InitResource()`初始化`FLocalVertexFactory`，别忘了它也是一个`FRenderResource`。有了所有的顶点数据后`(Data)`，就可以在`FLocalVertexFactory::InitRHI()`中声明对应的InputLayout了。InputLayout相关的数据都定义在父类`FVertexFactory`中：

![VF_VFMemberProperty](../assets/UE/VF_VFMemberProperty.png)

通常顶点数据中都包含了Position、Normal、TexCoord等数据，在正常渲染中使用，但也有一些Pass不需要这么多数据，例如DepthOnlyPass。所以这里需要声明三种顶点数据的布局，对应到枚举`EVertexInputStreamType`的三个枚举项，以正常渲染使用的顶点数据为例，即`Streams`和`Declaration`，`FVertexStream`的结构很简单：

![VF_InputElement](../assets/UE/VF_InputElement.png)

就是一种顶点元素对应的VertexBuffer和Offset，顶点数据中的元素按顺排在`Streams`中，而`Declaration`就是对应的图形API对顶点数据布局的描述信息，在DX12中即是`FD3D12VertexDeclaration`，里面就是存了一组`D3D12_INPUT_ELEMENT_DESC`。`InitRHI()`首先就完成了从`FDataType Data`到对应顶点输入布局的转换。

然后就是创建ConstantBuffer：

![VF_CreateConstantbuffer](../assets/UE/VF_CreateConstantbuffer.png)

这里代码的命名是UniformBuffer，似乎是OpenGL里面的叫法，和DX12中的Constantbuffer应该是一个意思。这里判断要不要创建`Uniformbuffer`在Windows上多半是能通过的。这个Buffer主要是把前面说的那些顶点数据，以Buffer的形式传给Shader，然后再HLSL中我们手动读取。里面创建时，会把`Data`中的那些Buffer初始化给它。而另外一个`LooseParametersUniformBuffer`似乎是和GPUSkin相关的？

至此，一个`UStaticMesh`的`FLocalVertexFactory`就初始化好啦。从`FStaticMeshRenderData`这里开始，把每一级LODResource的Mesh数据分开存，且一个LODResorce都对应一个VertexFactory，在GameThread，先加载好FStaticMeshRenderData，然后调用它的初始化，对LODResource中的每一个Buffer都调用它的初始化方法，创建渲染线程的命令，从对应的原始数据创建RHIBuffer，并上传数据，然后用LODResource初始化对应的VertexFactory，根据有的VertexBuffer和里面的格式，引用这些Buffer和对应的SRV，创建对应的`VertexDeclaration`。然后创建VertexFactory需要的UniformBuffer。












Vertex Factory 如何控制到Common base pass vertex shader的输入 ?

Tessellations 是如何处理的(Hull, Domain stages) ? 

Material Graph 最终是如何到HLSL Code中的?

Deferred pass 是如何Work的?

### A Second Look at Vertex Factories
以LocalVertexFactory.ush和 BasePassVertexCommon.ush为例. 对比GpuSkinVertexFactory.ush.

### Changing Input Data
CPU端, 用FVertexFactory处理不同类型的Mesh有着不同的顶点数据传递给GPU.

GPU端, 由于所有VertexFactories都用同样的VertexShader(至少BasePass是), 所以用一个通用命名的结构体FVertexFactoryInput来将这些数据传输到GPU. 每一种VertexFactory的shader factory中都定义自己的FVertexFactoryInput的具体实现, BasePassVertexCommon.ush中Include /Engine/Generated/VertexFactory.ush.这个文件在shader编译的时候会include到对应的*VertexFactory.ush, 其中就定义了对应的FVertexFactoryInput结构体.

此前提到, 在实现一种Mesh的VertexFactory时, 还需要用一个宏将这个VertexFactory和一个Shader代码的文件关联起来, 这个文件
```c++
IMPLEMENT_VERTEX_FACTORY_TYPE_EX(FLocalVertexFactory,"/Engine/Private/LocalVertexFactory.ush",true,true,true,true,true,true,true);
```
就是上面提到的VertexFactory的HLSL版本, 其中定义了GPU端的顶点数据表示FVertexFactoryInput.

这样, BasePassVertexShader就匹配上了顶点数据.

不同的VertexFactories在VS和PS之间需要不同的数据插值 ?

和FVertesFactoryInput的思路一样, BasePassVertexShader.usf中也会调用一些Generic function--GetVertexFactoryIntermediates, VertexFactoryGetWorldPostion, GetMaterialVertexParameters, 这些和FVertexFactoryInput一样都实现在对应的*VertexFactory.ush中.

### Changing Output Data
从VertexShader到PixelShader的输出数据, 同样的套路, 在BasePassVertexShader.usf, 用另一个Generically named struct FBasePassVSOutput, 它的实现同样也是看VertexFactory. 这里还有另一个障碍, 如果开启了Tessellation, 在Vertex Shader和Pixel shader之间还有两个阶段(Hull and Domain Stages), 这两个阶段需要不同数据(和仅仅是VS 到PS相比).

在BasePassVertexCommon.ush中, 用宏定义改变FBasePassVSOutput的定义, 根据是否开启了Tessellation, 选择FBasePassVSToDS或FBasePassVSToPS.

在最终的FBasePassVSOutput中, 有两个结构体成员FVertexFactoryInterpolantsVSToPS 和 FBasePassInterpolantsVSToPS(或DS版本), 其中, FVertexFactoryInterpolantsVSToPS是在具体的*VertexFactory.ush中定义的, 另外一个就是BasePassVertexShader通用的输出.

在BasePassVertexShader中, 用不同的inlcude, 重定义结构体和一些函数抽象出通用代码, 而不依赖于具体的VertexFactory和Tessellation的开启.

### BasePassVertexShader
在BasePassVertexShader.usf中, 所做的就是计算BasePassInterpolants和VertexFactoryInterpolants的值. 而这些计算过程就有点复杂了. 有许多的特殊情况, 由preprocessor定义 选择声明不同的interpolators, 决定给哪些属性赋值.

例如, 在BasePassVertexShader.usf中, 利用#if WRITES_VELOCITY_TO_GBUFFER , 根据这一帧和上一帧当前顶点世界坐标的差值计算出这个顶点的速度, 并存储在BasePassInterpolants变量中. 这意味着仅仅需要把Velocity写到GBuffer的Shader变体才会执行这个计算, 这减少了Shader stage之间的数据传输, 和计算量.


# Reference：

* [skelot instanced skeletal mesh rendering](https://www.unrealengine.com/marketplace/en-US/product/skelot-instanced-skeletal-mesh-rendering)
* [基于UE4的 Mobile Skeletal Instance - VS Shader](https://zhuanlan.zhihu.com/p/339031851)
* https://www.zhihu.com/question/377037950/answer/1067763870
* 
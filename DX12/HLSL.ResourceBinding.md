---
id: 7pe1f9prec6xe55krkd107r
title: ResourceBinding
desc: ''
updated: 1669986055386
created: 1669771627659
---

# Resource type and arrays

Shader Model 5.1资源绑定语法中使用`register`关键字传达资源绑定信息给HLSL编译器。例如：

```H
Texture2D<float4> tex1[4] : register(t3)
```

这个语句声明了一个4个Texture的Array，分别被绑定到slots t3，t4，t5和t6。即从绑定的第一个t3往后顺延。

HLSL中的Direct3D12资源绑定到逻辑寄存器空间(rigster sapce)内的虚拟寄存器：

* t —— Shader resource view(SRV)
* s —— Samplers
* u —— unordered access views(UAV)
* b —— Constant buffer views(CBV)

引用这个Shader的RootSignature必须与声明的Register slots兼容。例如：

```H
DescriptorTable( CBV(b1), SRV(t0,numDescriptors=99), CBV(b2) )
```

这个DescriptorTable所在的RootSignature就和Texture slot3 ~ 6的使用兼容，这个DescriptorTable用到了t0~t99，包含了t3~t6。

Resource 声明可以是Scalar，1D array，或者multidimendional array：

```c++
Texture2D<float4> tex1 : register(t3,  space0)
Texture2D<float4> tex2[4] : register(t10)
Texture2D<float4> tex3[7][5][3] : register(t20, space1)
```

SM5.1有着与SM5.0一样的资源类型和元素类型。SM5.1的资源声明更灵活，仅会被Runtime下的硬件所限制。关键字space指定了被声明的变量绑定到哪一个寄存器空间。如果缺失space关键字，将会使用默认寄存器空间索引0，上面tex2声明的Resource就绑定在space0中。register(t3,0)永远不会和register(t3, sapce1)冲突，也不会与任何其它寄存器空间内的t3冲突。

Array resource可以没有size，声明时指定第一个维度为空或者0：

```H
Texture2D<float4> tex1[] : register(t0)
```

匹配的descriptor table可以是：

```H
DescriptorTable( CBV(b1), UAV(u0, numDescriptors = 4), SRV(t0, numDescriptors=unbounded) )
```

HLSL中的固定size数组与decriptor table中的numDescriptor设置的固定数字相匹配，HLSL中的无界数组与desscriptor中unbounded声明匹配。

多维数组也可以是无界的，在虚拟寄存器空间，多维数组会被展平：

```c++
Texture2D<float4> tex2[3000][10] : register(t0, space0); // t0-t29999 in space0
Texture2D<float4> tex3[0][5][3] : register(t5, space1)
```

同一逻辑寄存器空间下的资源范围不可以重叠，声明的不同资源类型(t，s，u，b)的寄存器范围不会重叠，这些都包括unbounded range。上面的tex2和tex3声明在不同的寄存器空间，所以不会重叠。

访问数组资源直接用索引即可：

```H
Texture2D<float4> tex1[400] : register(t3);
sampler samp[7] : register(s0);
tex1[myMaterialID].Sample(samp[samplerID], texCoords);
```
对索引（myMaterialID，samplerID）有一个重要的默认限制，在一个[wave](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/hlsl-shader-model-6-0-features-for-direct3d-12#terminology)中索引不能改变。即使根据实例更改索引也算作变化。

如果需要改变索引，需要在index上指明*NonUniformResourceIndex*限定符：
```c++
tex1[NonUniformResourceIndex(myMaterialID)].Sample(samp[NonUniformResourceIndex(samplerID)], texCoords);
```
某些硬件上，使用此限定符生成额外的代码强制跨线程执行的正确性，但性能成本很小。如果没有指明这个限定符却更改了索引，执行结果将是undefined。

# Descriptor arrays and texture arrays

从DirectX 10开始支持`Texture arrays`。Texture arrays只要一个descriptor，所有元素必须有一样的format，width，height和mip count。数组必须在一个连续的虚拟地址空间。下面的代码展示了如何从shader中访问texture array：
```c++
Texture2DArray<float4> myTex2DArray : register(t0); // t0
float3 myCoord(1.0f,1.4f,2.2f); // 2.2f is array index (rounded to int)
color = myTex2DArray.Sample(mySampler, myCoord);
```
在texture array中，索引可以自由改变，不需要任何限定符。

等价的descriptor array：
```c++
Texture2D<float4> myArrayOfTex2D[] : register(t0); // t0+
float2 myCoord(1.0f, 1.4f);
color = myArrayOfTex2D[2].Sample(mySampler,myCoord); // 2 is index
```
这里的索引不再是浮点数。Descriptor array在维度上提供了更多的灵活性。类型（Texture2D）不能改变，但是format，width，height和mip数都可以改变。

texture array 的 descriptor array也是合法的：

```c++
Texture2DArray<float4> myArrayOfTex2DArrays[2] : register(t0);
```

如果一个结构体中包含descriptor，则不可以声明这个结构体的数组，下面的代码是不被支持的：

```c++
struct myStruct {
    Texture2D                    a; 
    Texture2D                    b;
    ConstantBuffer<myConstants>  c;
};
myStruct foo[10000] : register(....);
```
这样定义的内存布局是abcabcabc..., 由于HLSL的限制这是不被支持的。

# Resource Aliasing

在HLSL shader中指定的reource range是逻辑range。通过RootSignature机制可以把它们绑定到heap range。通常一个logical range对应的heap range不应该与其它heap range重叠。但RootSignature的机制可以支持alias，heap range可以重叠， 一个descriptor可以映射到多个logical range。例如，上面的tex2和tex3可以映射到同一个heap range，最终的效果就是在HLSL程序中支持texture别名。如果需要这种别名，要使用D3D10_SHADER_RESOURCES_MAY_ALIAS选项编译shader，在Effect-Compiler Tool(FXC)工具设置/res_may_alias选项。在假定资源可能是alias时，这个选项会生成正确的代码阻止一些load/store优化。

# Divergence and derivatives

SM5.1对资源索引没有限制，`tex2[idx].Sample(...)`中，索引idx可以是字面常量，cbuffer constant，或者插值的结果。虽然编程模型提供了如此大的灵活性，还是有一些问题需要注意：

* 如果索引跨四边形发生变化，则硬件计算的衍生数据（LOD）可能是不确定的。这种情况下，HLSL编译器会尽可能地发出警告，但不会停止编译。

* 如果资源索引有差异，相较于索引不变的情况下，性能会下降，因为硬件需要针对多个资源执行操作。必须在HLSL代码中用*NonUniformResourceIndex*标记可能出现差异的资源索引。否则结果是不确定的。

# UAVs in pixel shaders

SM5.1 对pixel shader中的UAV range没有施加限制，与SM5.0一样。

# Constant Buffers

SM5.1中constant buffers的语法有了改变，开发者现在可以索引constant buffers，SM5.1引入了ConstantBuffer"template"构造：
```c++
struct Foo
{
    float4 a;
    int2 b;
};
ConstantBuffer<Foo> myCB1[2][3] : register(b2, space1);
ConstantBuffer<Foo> myCB2 : register(b0, space1);
```
上面的代码声明了类型为*Foo*的constant buffer变量*myCB1*，size为6，还有一个标量constant buffer变量myCB2。然后可以直接索引这些变量：
```c++
myCB1[i][j].a.xyzw
myCB2.b.yy
```
字段a和b不会成为全局变量，而是必须被看作字段。为了向后兼容，SM5.1支持老的cbuffer的标量形式。下面的语句会使'a'和'b'称为全局的，只读变量，但是不可以索引：
```c++
cbuffer : register(b1)
{
    float4 a;
    int2 b;
};
```
当前，shader编译器仅对user-defined结构体支持ConstantBuffer template。

为了兼容，HLSL编译器会为声明在space0的range自动分配resource register。如果声明资源时没有指明space，默认使用space0。编译器会启发式地找到第一个合适的register。这种分配可以通过反射API获取，它经过扩展添加了space字段，BindPoint字段表明资源寄存器范围的下限。

# Bytecode changes in SM5.1

SM5.1更改了资源寄存器声明和引用的指令。声明一个寄存器变量的语法，和组织共享内存寄存器一样：

```c++
Texture2D<float4> tex0          : register(t5,  space0);
Texture2D<float4> tex1[][5][3]  : register(t10, space0);
Texture2D<float4> tex2[8]       : register(t0,  space1);
SamplerState samp0              : register(s5, space0);

float4 main(float4 coord : COORD) : SV_TARGET
{
    float4 r = coord;
    r += tex0.Sample(samp0, r.xy);
    r += tex2[r.x].Sample(samp0, r.xy);
    r += tex1[r.x][r.y][r.z].Sample(samp0, r.xy);
    return r;
}
```
这会被展开成：
```c++
// Resource Bindings:
//
// Name                                 Type  Format         Dim    ID   HLSL Bind     Count
// ------------------------------ ---------- ------- ----------- -----   --------- ---------
// samp0                             sampler      NA          NA     S0    a5            1
// tex0                              texture  float4          2d     T0    t5            1
// tex1[0][5][3]                     texture  float4          2d     T1   t10        unbounded
// tex2[8]                           texture  float4          2d     T2    t0.space1     8
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// COORD                    0   xyzw        0     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET   float   xyzw
//
ps_5_1
dcl_globalFlags refactoringAllowed
dcl_sampler s0[5:5], mode_default, space=0
dcl_resource_texture2d (float,float,float,float) t0[5:5], space=0
dcl_resource_texture2d (float,float,float,float) t1[10:*], space=0
dcl_resource_texture2d (float,float,float,float) t2[0:7], space=1
dcl_input_ps linear v0.xyzw
dcl_output o0.xyzw
dcl_temps 2
sample r0.xyzw, v0.xyxx, t0[0].xyzw, s0[5]
add r0.xyzw, r0.xyzw, v0.xyzw
ftou r1.x, r0.x
sample r1.xyzw, r0.xyxx, t2[r1.x + 0].xyzw, s0[5]
add r0.xyzw, r0.xyzw, r1.xyzw
ftou r1.xyz, r0.zyxz
imul null, r1.yz, r1.zzyz, l(0, 15, 3, 0)
iadd r1.y, r1.z, r1.y
iadd r1.x, r1.x, r1.y
sample r1.xyzw, r0.xyxx, t1[r1.x + 10].xyzw, s0[5]
add o0.xyzw, r0.xyzw, r1.xyzw
ret
// Approximately 12 instruction slots are used.
```
在Shader bytecode中，每一个shader resource range都有一个ID。例如，在shader bytecode中，tex1（t10）texture array称为了'T1'。给每个resource range一个唯一ID可以实现：
* 在一个指令中明确标识一个被索引的resource range（）。
* 在声明中附加一些属性，元素类型，stride size，raster operation mode。

注意，Range的ID与HLSL的下界声明无关。

反射资源绑定的顺序和shader声明指令的顺序是一致的，这样可以帮助识别对应的HLSL变量和bytecode ID。

SM5.1中的每一个声明指令都使用三个操作数：Range ID， lower和upper bounds。还需要一个额外的token指定register space。一些其它的token也会用于描述额外的range属性，cbuffer或结构体buffer声明指令会有cbuffer或者structure的size。详细的编码细节可以在d3d12TokenizedProgramFormat.h and D3D10ShaderBinary::CShaderCodeParser中找到。

Example：
```c++
Texture2D foo[5] : register(t2);
Buffer bar : register(t7);
RWBuffer dataLog : register(u1);

Sampler samp : register(s0);

struct Data
{
    UINT index;
    float4 color;
};
ConstantBuffer<Data> myData : register(b0);

Texture2D terrain[] : register(t8); // Unbounded array
Texture2D misc[] : register(t0,space1); // Another unbounded array 
                                        // space1 avoids overlap with above t#

struct MoreData
{
    float4x4 xform;
};
ConstantBuffer<MoreData> myMoreData : register(b1);

struct Stuff
{
    float2 factor;
    UINT drawID;
};
ConstantBuffer<Stuff> myStuff[][3][8]  : register(b2, space3)
```

# Input element layout
D3D12_INPUT_ELEMENT_DESC 是 Direct3D 12 中描述输入布局元素的结构，它用于定义顶点输入的格式和方式。每个 D3D12_INPUT_ELEMENT_DESC 结构体描述一个输入元素（如顶点的位置、法线、纹理坐标等）。它的定义如下：
```c++
typedef struct D3D12_INPUT_ELEMENT_DESC {
    LPCSTR                     SemanticName;
    UINT                       SemanticIndex;
    DXGI_FORMAT                Format;
    UINT                       InputSlot;
    UINT                       AlignedByteOffset;
    D3D12_INPUT_CLASSIFICATION InputSlotClass;
    UINT                       InstanceDataStepRate;
} D3D12_INPUT_ELEMENT_DESC;
```
下面是各个属性的解释：

1. SemanticName:
类型: LPCSTR
说明: 指定输入元素的语义名称（如 "POSITION", "NORMAL", "TEXCOORD" 等）。语义名称用于将顶点着色器的输入与输入布局中的元素匹配。

2. SemanticIndex:
类型: UINT
说明: 语义的索引，允许区分具有相同语义名称的多个元素。例如，可以有多个纹理坐标（"TEXCOORD0", "TEXCOORD1" 等）。

3. Format:
类型: DXGI_FORMAT
说明: 指定输入元素的数据格式。常见的格式包括 DXGI_FORMAT_R32G32_FLOAT（2D 浮点向量），DXGI_FORMAT_R32G32B32A32_FLOAT（4D 浮点向量）等。

4. InputSlot:
类型: UINT
说明: 指定输入槽的索引，该索引是绑定顶点缓冲区的标识符。例如，具有多个顶点缓冲区时可以通过不同的槽号来区分。

5. AlignedByteOffset:
类型: UINT
说明: 输入元素在每个顶点的内存布局中的偏移量，以字节为单位。
D3D12_APPEND_ALIGNED_ELEMENT 的特殊值可以用于自动计算偏移量。

6. InputSlotClass:
类型: D3D12_INPUT_CLASSIFICATION
说明: 指定输入元素的分类类型，取值为：
D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA: 每个顶点都有一个对应的数据（常用于顶点属性如位置、法线等）。
D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA: 每个实例都有一个对应的数据（常用于实例化渲染）。

7. InstanceDataStepRate:
类型: UINT
说明: 当 InputSlotClass 为 D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA 时，指定实例数据的步长率。即每几个实例使用相同的数据。例如，当 InstanceDataStepRate 为 1 时，每个实例的数据都是不一样的；如果为 2，则每两个实例使用相同的数据。

## 示例
假设有一个简单的顶点格式，包含位置和纹理坐标，输入布局的描述可能如下：
```c++
D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
```
这里的 POSITION 在第 0 个输入槽（vertex buffer slot 0）中的偏移量为 0，格式为 3D 浮点向量；TEXCOORD 在同一槽中，其偏移量是自动计算的（紧跟在 POSITION 属性之后），格式为 2D 浮点向量。

通过正确配置 D3D12_INPUT_ELEMENT_DESC 结构体，您能够告知 D3D12 如何从顶点缓冲区读取数据并传递给顶点着色器。

# Blend desc
在 Direct3D 12（D3D12）中，D3D12_BLEND_DESC 结构体用于描述混合状态（Blend State）。混合状态控制渲染时如何将新绘制的像素颜色与帧缓冲区中已有像素的颜色进行组合。这对于实现透明效果和其它复杂的渲染效果非常重要。

D3D12_BLEND_DESC 结构体定义如下：
```c++
typedef struct D3D12_BLEND_DESC {
    BOOL AlphaToCoverageEnable;
    BOOL IndependentBlendEnable;
    D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[8];
} D3D12_BLEND_DESC;
```
1. BOOL AlphaToCoverageEnable:

说明: 指定是否启用 Alpha-to-Coverage。Alpha-to-Coverage 是一种多重采样抗锯齿（MSAA）技术，用于在不透明表面和半透明表面之间进行更细粒度的反锯齿效果。当 TRUE 时启用 Alpha-to-Coverage；当 FALSE 时禁用。

2. BOOL IndependentBlendEnable:
   
说明: 指定是否启用独立混合。独立混合允许对每个渲染目标分别设置混合选项。当 TRUE 时，可以为每个渲染目标设置不同的混合配置；当 FALSE 时，所有渲染目标使用 RenderTarget[0] 的混合配置。

3. D3D12_RENDER_TARGET_BLEND_DESC[8] RenderTarget:

说明: 包含最多 8 个 D3D12_RENDER_TARGET_BLEND_DESC 结构体，每个结构体描述一个渲染目标的混合状态。
其定义如下：
```c++
typedef struct D3D12_RENDER_TARGET_BLEND_DESC {
    BOOL                     BlendEnable;
    BOOL                     LogicOpEnable;
    D3D12_BLEND              SrcBlend;
    D3D12_BLEND              DestBlend;
    D3D12_BLEND_OP           BlendOp;
    D3D12_BLEND              SrcBlendAlpha;
    D3D12_BLEND              DestBlendAlpha;
    D3D12_BLEND_OP           BlendOpAlpha;
    D3D12_LOGIC_OP           LogicOp;
    UINT8                    RenderTargetWriteMask;
} D3D12_RENDER_TARGET_BLEND_DESC;
```
以下是各个属性的详细解释：

1. BOOL BlendEnable:
说明: 指定是否启用混合。当 TRUE 时启用混合；当 FALSE 时禁用。

2. BOOL LogicOpEnable:
说明: 指定是否启用逻辑操作。当 TRUE 时启用逻辑操作，使用 LogicOp 属性；当 FALSE 时禁用。使用`LogicOp`定义的操作，直接对两个RGBA的各个分量进行操作。不再使用后面定义的复杂D3D12_BLEND。

3. D3D12_LOGIC_OP LogicOp:
说明: 指定逻辑操作，用于在逻辑操作模式下组合源和目标颜色。例如 D3D12_LOGIC_OP_CLEAR、D3D12_LOGIC_OP_SET 等（仅在 LogicOpEnable 为 TRUE 时适用）。

4. UINT8 RenderTargetWriteMask:
说明: 指定渲染目标的写入掩码，确定哪些颜色通道（Red、Green、Blue、Alpha）能够被写入。可以是 D3D12_COLOR_WRITE_ENABLE_RED、D3D12_COLOR_WRITE_ENABLE_GREEN、D3D12_COLOR_WRITE_ENABLE_BLUE、D3D12_COLOR_WRITE_ENABLE_ALPHA 的组合。

`SrcBlend`,`DestBlend`,`BlendOp`定义了新绘制的颜色如何与RenderTarget中已有的颜色混合。`SrcBlend`定义新绘制的颜色的系数计算方式，`DestBlend`定义的是RenderTarget中已有的颜色的系数计算方式，`BlendOp`定义了把这两个颜色分别乘以各自的系数后，如何组合起来，计算公式如下：
```c++
SourceColor=(Rs,Gs,Bs)
DestColor=(Rd,Gd,Bd)
BlendOp = +
ResultColor = SrcBlendFactor * SourceColor + DestBlendFactor * DestColor

如果
SrcBlend=D3D12_BLEND_SRC_ALPHA
DestBlend=D3D12_BLEND_INV_SRC_ALPHA
ResultColor = (As) * SourceColor + (1-As) * DestColor

BlendOp还有以下选择：
D3D12_BLEND_OP_SUBTRACT 从Src的值减去Dest的值
D3D12_BLEND_OP_REV_SUBTRACT 从Dest的值减去Src的值
```
`SrcBlendAlpha`,`DestBlendAlpha`,`BlendOpAlpha`也是一样的，不过仅针对如何混合最终的Alpha值。

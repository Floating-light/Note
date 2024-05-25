---
id: 0mgnzscvpzf7jjo1ta03whf
title: Model
desc: ''
updated: 1716596663492
created: 1670806808115
---
`Model`工程实现了加载gltf文件，并转成自己的mini格式的Model数据文件。还提供了Mesh渲染的基本渲染管线配置。

# 模型加载与转换
对于一个加载进来的gltf文件，需要首先转换成自己定义的，方便渲染的格式，并直接以这个格式序列化到磁盘上，作为自定义的模型文件格式。这样可以做到加载自己定义的模型文件就直接可以开始渲染，不用再对模型数据做额外的处理。

首先定义文件头结构`FileHeader`，包括格式，版本等基本文件信息，以及基本的模型信息：node数量，Mesh数量，材质数量，各个Buffer的数据大小，方便读取后续的数据。
```c++
struct FileHeader
{
    char     id[4];   // "MINI"
    uint32_t version; // CURRENT_MINI_FILE_VERSION
    uint32_t numNodes;
    uint32_t numMeshes;
    uint32_t numMaterials;
    uint32_t meshDataSize;
    uint32_t numTextures;
    uint32_t stringTableSize;
    uint32_t geometrySize;
    uint32_t keyFrameDataSize;      // Animation data
    uint32_t numAnimationCurves;
    uint32_t numAnimations;
    uint32_t numJoints;     // All joints for all skins
    float    boundingSphere[4];
    float    minPos[3];
    float    maxPos[3];
};
```
实际的`ModelData`结构体：
```c++
struct ModelData
{
    BoundingSphere m_BoundingSphere;
    AxisAlignedBox m_BoundingBox;
    std::vector<byte> m_GeometryData;
    std::vector<byte> m_AnimationKeyFrameData;
    std::vector<AnimationCurve> m_AnimationCurves;
    std::vector<AnimationSet> m_Animations;
    std::vector<uint16_t> m_JointIndices;
    std::vector<Matrix4> m_JointIBMs;
    std::vector<MaterialTextureData> m_MaterialTextures; // 对应材质的textures , 一个TextureData里面多个texture, 对应texture的值是下面"m_TextureNames"的index，没有就是 0xFFFF
                                                            // addressModes 是采样器的flag， 用一个uint32记下了所有对应texture的采样参数
    std::vector<MaterialConstantData> m_MaterialConstants; // 材质constants参数, baseColorFactor ...
    std::vector<Mesh*> m_Meshes;
    std::vector<GraphNode> m_SceneGraph;
    std::vector<std::string> m_TextureNames; // texture 的相对path(文件名)
    std::vector<uint8_t> m_TextureOptions; // 每个texture对应的纹理选项:TexConversionFlags， 没有就是0xFF
};
```
在加载原始`gltf`文件后，从`gltf`文件构建`ModelData`。
## 构建材质
提取Texture，在Model文件中，纹理通常由相对于模型文件的路径表示，通常直接就是文件名，所以直接把`glTF::Asset`中的images路径按顺序复制给`m_TextureNames`。

一个Model通常有多个材质。`m_MaterialConstants`与`m_MaterialTextures`一一对应，`m_MaterialConstants`中存储的是每个材质的`Constants`参数：
```c++
struct MaterialConstantData
{
    float baseColorFactor[4]; // default=[1,1,1,1]
    float emissiveFactor[3]; // default=[0,0,0]
    float normalTextureScale; // default=1
    float metallicFactor; // default=1
    float roughnessFactor; // default=1
    uint32_t flags;
};
```
表示这个材质的baseColor系数，emissive系数，metallic，roughness等参数。`m_MaterialTextures`中的元素表示的是一个材质所包含的纹理：
```c++
enum { kBaseColor, kMetallicRoughness, kOcclusion, kEmissive, kNormal, kNumTextures };
// Used at load time to construct descriptor tables
struct MaterialTextureData
{
    uint16_t stringIdx[kNumTextures];
    uint32_t addressModes;
};
```
渲染管线预先定义好了支持的纹理枚举，`stringIdx`中，以纹理枚举为Index，表示对应纹理在`m_TextureNames`中的索引位置，没有就是0xFFFF，即uint16_t的最大值。`addressModes`是采样器的flag， 用一个uint32记下了所有对应texture的采样参数，按这里原来的写法，每个纹理用了四位记下两个参数，一共五张，32位绰绰有余。

`m_TextureOptions`保存每个Texture的属性，与`m_TextureNames`一一对应，这是每个纹理独立于材质的属性信息:
```c++
enum TexConversionFlags
{
    kSRGB = 1,          // Texture contains sRGB colors
    kPreserveAlpha = 2, // Keep four channels
    kNormalMap = 4,     // Texture contains normals
    kBumpToNormal = 8,  // Generate a normal map from a bump map
    kDefaultBC = 16,    // Apply standard block compression (BC1-5)
    kQualityBC = 32,    // Apply quality block compression (BC6H/7)
    kFlipVertical = 64,
};
```
最后，根据`m_TextureOptions`对所有纹理进行格式转换，压缩等操作，

## Mesh
与Mesh相关的数据结构有两个，Mesh:
```c++
struct Mesh
{
    float    bounds[4];     // A bounding sphere
    uint32_t vbOffset;      // BufferLocation - Buffer.GpuVirtualAddress
    uint32_t vbSize;        // SizeInBytes
    uint32_t vbDepthOffset; // BufferLocation - Buffer.GpuVirtualAddress
    uint32_t vbDepthSize;   // SizeInBytes
    uint32_t ibOffset;      // BufferLocation - Buffer.GpuVirtualAddress
    uint32_t ibSize;        // SizeInBytes
    uint8_t  vbStride;      // StrideInBytes
    uint8_t  ibFormat;      // DXGI_FORMAT
    uint16_t meshCBV;       // Index of mesh constant buffer， of GraphNode m_SceneGraph
    uint16_t materialCBV;   // Index of material constant buffer
    uint16_t srvTable;      // Offset into SRV descriptor heap for textures
    uint16_t samplerTable;  // Offset into sampler descriptor heap for samplers
    uint16_t psoFlags;      // Flags needed to request a PSO
    uint16_t pso;           // Index of pipeline state object
    uint16_t numJoints;     // Number of skeleton joints when skinning
    uint16_t startJoint;    // Flat offset to first joint index
    uint16_t numDraws;      // Number of draw groups

    struct Draw
    {
        uint32_t primCount;   // Number of indices = 3 * number of triangles
        uint32_t startIndex;  // Offset to first index in index buffer 
        uint32_t baseVertex;  // Offset to first vertex in vertex buffer
    };
    Draw draw[1];           // Actually 1 or more draws
};
```
有一个数组`m_Meshes`，保存所有Mesh。每个Mesh主要保存vertex，index buffer在`m_GeometryData`的起始位置和大小，还有材质信息，以及Runtime下渲染的一些Buffer信息。

一个Model的所有Vertex，index buffer都放在一个大的byte数组`m_GeometryData`中。

`GraphNode`数组`m_SceneGraph`代表场景中所有可渲染的实体，包含LocalTransform，Mesh只有几何数据。

Mesh对所有它用到的数据都是通过Index间接引用。meshCBV就是表示Transform等基本CBV信息的Node的引用。
```c++
struct GraphNode // 96 bytes
{
    Math::Matrix4 xform;// all Local space
    Math::Quaternion rotation;
    Math::XMFLOAT3 scale;

    uint32_t matrixIdx : 28; // 当前Node在m_SceneGraph的Index
    uint32_t hasSibling : 1; // 是否有右兄弟节点
    uint32_t hasChildren : 1;
    uint32_t staleMatrix : 1;
    uint32_t skeletonRoot : 1;
};
```
在构建时，遍历源模型文件的`Scene`，构建`m_SceneGraph`，同时遇到Mesh就构建Mesh。

从源模型文件构建Mesh的indices时需要注意：
* Primitive可能不是三角面，此时可能无法处理
* 源Primitive可能没有indices，Primitive中的顶点按顺序作为三角面绘制，此时要手动生成indices
* 源indices的类型不一定是int32，我们可以根据源indices的最大值，确定我们indices是int32的，还是int16的，可以节省内存（是否有必要）。
* 可以先对顶点数据进行优化，对indices进行重排，vertex cache optimization. 最大化"post-transform vertex cache reuse"。这种优化通常与硬件的CacheSize相关，CacheSize不匹配会导致负优化。
```
OptimizeFaces()
https://github.com/microsoft/DirectXMesh/wiki/OptimizeFaces
```

读取Mesh顶点数据时，通过原始文件的数据信息，有什么顶点数据就读什么顶点数据，并且用psoFlags表明有什么顶点数据，是否开启AlphaBlend等所以与Pso相关的设置，后面就用这个确定使用或创建什么样的PSO。`OptimizeMesh()`




## Header m_Header
保存这个Model的一些基本信息：
* mesh数量
* material数量
* 顶点数据Size
* 索引数据Size
* BoundingBox
且这个结构体与数据Header的布局一致，可以直接读到这个结构的内存块中。

然后按照这些Header的数据读取后续的大块数据，通常顶点，索引数据都是放在一起的，算出它们的SIze后可以可以直接读到一个大的Buffer中（UploadBuffer），然后通过地址偏移引用它们。

然后加载所有纹理，根据材质的设置，这里所有材质都是6张纹理，所有这些纹理SRV保存在一块连续heap中，没有的用默认texture代替。这里表明，所有材质都是预先设置好的。

## indices buffer optimize
对indices进行重排，vertex cache optimization. 最大化"post-transform vertex cache reuse"。这种优化通常与硬件的CacheSize相关，CacheSize不匹配会导致负优化。

`OptimizeFaces()`

https://github.com/microsoft/DirectXMesh/wiki/OptimizeFaces

## mesh data 
用PSOFlags表明这个Mesh有什么顶点数据。vertex buffer和indeces buffer等按实际情况组织。`OptimizeMesh()`
所有Mesh的数据都存在一个Buffer里面`Renderer::CompileMesh()`，组成一个Model。
```c++
    struct FileHeader
    {
        char     id[4];   // "MINI"
        uint32_t version; // CURRENT_MINI_FILE_VERSION
        uint32_t numNodes;
        uint32_t numMeshes;
        uint32_t numMaterials;
        uint32_t meshDataSize;
        uint32_t numTextures;
        uint32_t stringTableSize;
        uint32_t geometrySize;
        uint32_t keyFrameDataSize;      // Animation data
        uint32_t numAnimationCurves;
        uint32_t numAnimations;
        uint32_t numJoints;     // All joints for all skins
        float    boundingSphere[4];
        float    minPos[3];
        float    maxPos[3];
    };
struct Mesh
{
    Math::AxisAlignedBox boundingBox;

    uint32_t materialIndex;

    uint32_t attribsEnabled;
    uint32_t attribsEnabledDepth;
    uint32_t vertexStride;
    uint32_t vertexStrideDepth;
    Attrib attrib[maxAttribs];
    Attrib attribDepth[maxAttribs];

    uint32_t vertexDataByteOffset;
    uint32_t vertexCount;
    uint32_t indexDataByteOffset;
    uint32_t indexCount;

    uint32_t vertexDataByteOffsetDepth;
    uint32_t vertexCountDepth;
};
// All of the information that needs to be written to a .mini data file
struct ModelData
{
    BoundingSphere m_BoundingSphere; 
    AxisAlignedBox m_BoundingBox;
    std::vector<byte> m_GeometryData;
    std::vector<byte> m_AnimationKeyFrameData;
    std::vector<AnimationCurve> m_AnimationCurves;
    std::vector<AnimationSet> m_Animations;
    std::vector<uint16_t> m_JointIndices;
    std::vector<Matrix4> m_JointIBMs;
    std::vector<MaterialTextureData> m_MaterialTextures;
    std::vector<MaterialConstantData> m_MaterialConstants;
    std::vector<Mesh*> m_Meshes;
    std::vector<GraphNode> m_SceneGraph;
    std::vector<std::string> m_TextureNames;
    std::vector<uint8_t> m_TextureOptions;
};

### primitive 
```c++
struct Primitive
{
    BoundingSphere m_BoundsLS;  // local space bounds
    BoundingSphere m_BoundsOS;  // object space bounds
    AxisAlignedBox m_BBoxLS;       // local space AABB
    AxisAlignedBox m_BBoxOS;       // object space AABB
    Utility::ByteArray VB;
    Utility::ByteArray IB;
    Utility::ByteArray DepthVB;
    uint32_t primCount;
    union
    {
        uint32_t hash; // 与下面结构体共享一块内存
        struct {
            uint32_t psoFlags : 16; // 前16个bit表示pso相关配置的枚举项(输入layout，alphatest等配置)
            uint32_t index32 : 1; // indices是不是32bit的(or 16bit)
            uint32_t materialIdx : 15; // 材质索引
        };
        // Primitive.psoFlags, Primitive.index32, Primitive.materialidx可以直接访问或设置这三个部分
        // 也可以用Primitive.hash，用这三种配置组成唯一hash值,以判断两个PSO配置是否一样.
    };
    uint16_t vertexStride;
};
```

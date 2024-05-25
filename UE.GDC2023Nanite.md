---
id: bnq07qttl824zwts7ghsk2f
title: GDC2023Nanite
desc: ''
updated: 1683461403688
created: 1683421979323
---
TODO: Displacement with Nanite

# High-detail geometry with Nanite
通常的工作流程：
* 构建high-detail的模型
* 导入引擎，并用自动生成的LODs作为Fallback

在Fortnite中，
* 需要频繁地发布大量内容
* 高度可自定义的建筑系统
* 因此，手动给每一种high-detail models的变体建模是不现实的
  
  ![ModelVariations](/assets/images/ModelVariations.png)
  
所以只能用Displacement。
## Displacement with Nanite
在Fortnite中，要求：
* Crack-free meshes
* Seamless modular tiling meshes
* 与Fortnite中的建筑系统一起使用（这应该是一个特殊需求）
* Scalability 
* 容易更新旧资产
* 低内存占用率
* 对美术的工作流的影响也要尽可能地低

1. World position offset shader
  * Pros:
    * 完全动态
    * 即时更新
  * Cons:
    * 在NaniteMesh上用WPO，性能消耗非常大
    * 所有Mesh默认都需要很高地曲面细分

2.在DCC中使用Displacement直接建模
  * Pros:
    * Artist对最终效果可以完全掌控
  * Cons:
    * Artists将会有巨大工作量
    * 一旦导入Engine，将难以修改，不灵活
    * 迭代慢
所以这两种方式都不合适。

**Solution：让Artist仅创建Low-poly Mesh和Displacement map，在UE中程序化生成对应的Nanite Mesh！**

## 解决常见的Displacement问题

* Split vertices displacement

会在Mesh上造成Crack：

![Crack](/assets/images/Crask.png)

* Average normal displacement
  * Mesh会出现畸变
  * UV展开困难
  * 破坏modular seamless tiling meshes

使用Direct and indirect displacement解决这些问题。
* Displacement prototype in Houdini
  * 使用Mask确定顶点是Direct还是indirect displacement
  * Indirect displaced vertices将查找它到其它Direct vert的距离，并由此确定使用对应Direct displaced顶点的Displacement vector.
  * 证明了这一方法的正确性，但工作流不友好。

Unreal Engine geometry script prototype

* 也可以达到相同的效果，但工作流上仍然不是很理想。

## Move all to c++ code 

* Fortnite在可破坏建筑块上使用了Texture data-driven system
* This blends up to four material layers in a single material element 
* Displacement texture 和相关设置就添加到这个TextureData中
* Artist仅需要制作Displacement maps和简单的low-poly meshes
* 然后在编辑器内根据这些Displacement maps程序化生成所有high detail meshes。

### Optimizations

* 仅生成最少的需要的Unique的Meshes
* Adaptive tesselation 
* 代码处理high detail nanite mesh的创建
* Nanite mesh存储在DDC中

# Trees and foliage
Video ： https://www.youtube.com/watch?v=05FCjQR--Sc&list=PLZlv_N0_O1gYUMkyGerbUGedE7y-XQSJR&index=2
## 测试结果
* OpaqueMaterial比masked alpha test materials快得多
* WPO能用，但开销非常很大
* 每棵树的顶点数量 < 36W
* 虽然树干和树叶都是opaque，如果进一步将它拆分成两个材质，比使用更高开销的单个材质整体渲染更快。
* Nanite本身的渲染有overdraw问题
* Shadow proxies可以帮助节省性能，但在非常远的距离上，没有太好的效果，因为此时ProxyMesh和真实渲染的Mesh有着差不多的三角形数量。
* 所有这些细节的处理需要专门设计的程序化建模工具
## Tree
### Each vertex count
* 单个树叶有大概8 - 10 个Verts
* 在树冠内部被外面单个树叶遮挡的区域，使用有30 - 38 个顶点的树叶簇，以更少的顶点，使看起来更密。
* 大概3W片树叶， 整体 ~ 36W面

### WPO animation shader需要更高效：
* PivotPainter消耗过高。
* 要保持Shader中的计算量最小，不能有复杂的计算
* 将Animation bake到lookup texture（4张）中。
  *  U - Animation timeline, V - mapping of each branch
  *  Texture1 - Neutral pose of the tree 
  *  Texture2 - Wind orientation-> 基于风向修改Animation
  *  Texture3 - Position offset 
  *  Texture4 - Rotation offset
* 用Houdini script完成
![TreeAnimLookup](/assets/images/TreeAnimLookup.png)

### Result 
* High-detailed animated trees
* 由于那四个Texture要使用非流送的HDR纹理，所以内存上的开销比较大

Future improvements

* 单个树叶的移动还需要做得更好一点
* Physicals field reactions
* 动画过于刚性(rigid)， 还需要做得更柔和一点

Fallback mesh需要单独制作，用单独的材质，引擎提供方法可以把一个自定义的fallback meshes导入，并组成单个资产，并添加Nanite Override Material。

> 在平台不支持Nanite或高级LOD时需要使用fallback mesh

### Single Material 

如果，所有NaniteTree都用一个材质渲染（No Material Instance）（用UDIMs[^1]和array textures），理论上讲会更快。

但事实并非如此，因为Shader更复杂，导致开销剧增。

### Pipeline Blueprint

* 自动合并Nanite 和 它的fallback mesh
* 自动把来自另一资产的Collision合并到Nanite资产
* 自动bake impostor(fallback mesh ?)

## Grass and small pants 
 
 * Vertices < 1200（直径约150cm的一簇草） per cluster
 * 需要Distance culling
 * 因为VertexShader开销很大，WPO需要按距离Disabled
    * 这意味着，不能用Vert position offset shader在远处淡化草地。可以移到Pixel shader做
 * blade modeled 比 cards with masked material好
 * 降低Nanite overdraw 甚至比树还重要 
  
WPO animation仍然需要优化：

使用一张lookup texture用作offset，看起来不及真正的Rotaion（PivotPainter），但开销非常低。

![OffsetLookup](/assets/images/OffsetLookup.png)

Wind speed noise texture

* RG channel：风向 noise
* B：intensity noise
* Wind speed 存在mipmap1

TODO ：
# Converting old assets to nanite meshes

[^1]: [UDIMs](https://garagefarm.net/blog/setting-up-udims-in-blender-step-by-step#:~:text=What%20are%20UDIMS,grid%20we're%20used%20to) 
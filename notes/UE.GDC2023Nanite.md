---
id: bnq07qttl824zwts7ghsk2f
title: GDC2023Nanite
desc: ''
updated: 1683430266905
created: 1683421979323
---
TODO: Displacement with Nanite
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
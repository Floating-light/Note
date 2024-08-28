Optimizing UE5
# Nanite
https://zhuanlan.zhihu.com/p/687141143

https://www.youtube.com/watch?v=xSUV8BahmI0&list=WL&index=3&t=272s

Rethinking Performance Paradigms for High-Quality Visuals - Part 1

https://www.youtube.com/watch?v=Cb63bHkWkwk&t=11963s

Nanite: 

- Not just for hi-res meshes 
  - 采用Nanite的低分辨率Mesh还是比普通StaticMesh渲染得快
  - Smaller data footprint
- if VSMs, then Nanite
  - Other systems run faster
- Nanite everything，eventually

Considerations
* NaniteVisBuffer
  * Occlusion
    * HZB
    * Primitives
    * Instances
    * Clusters
  * Rasterization
    * What materials are covered by what pixel
    * nanitestats
    * HW
      * 大三角形，覆盖大于一个像素 
    * SW
      * 小三角形，覆盖小于一个像素
    * r.nanite.visualize.advanced 1 可以看到更多可视化选项
    * Fixed Function 
      * Opaque
      * Non-deforming
      * 就是顶点的位置不会变的，顶点数据里面说在哪儿就在哪儿。非常快，整个场景的可以一次就光栅化完
    * Programmable Rasterization
      * 顶点位置，会发生变化，WPO，MaskedMaterials，PDO(PixelDepthOffset)，Displacement
    * Raster Bin可视化中，彩色的为一种可编程光栅化，黑色为固定函数光栅化
    * r.Nanite.ShowMeshDrawEvents 1，然后profileGPU，可以看到HWRasterize下，可以看到所有要求可编程光栅化的材质名字。
    * WPO
      * static mesh component上有bool值可以关EvaluateWorldPositionOffset
        * 或者设置个关闭的距离
      * 材质里可以设置MaxWOPDistance
      * Nanite Visualization 可以看什么地方在计算WPO
    * Masked Material
      * Always programmable
      * 增加Overdraw
* Overdraw
  * Masked材质尤为严重
* "Draw Calls"
  * CPU pre-allocate bins for material
  * 同样材质的物体，在resterize后，标记了屏幕像素对应的材质id，即使不同模型，后续也只需要一次draw。
  * 想办法减少材质种类
    * custom primitive data
    * per instance custom data
    * material parameter collection
  * Empty draws
    * 对场景里所有Unique的材质pre-allcoate material bins，但对应的geo可能在光栅化之前就被剔除了
      * 这就是empty draws
      * nanitestats
  * 减少Bins，空的Bins也对性能有影响。
  * Instancing
  * Scalability


* Mesh construction
  * Poly Budgets
  * Cluster Size
  * Long， Thin Tris
  * Disk Size vs. Cooked Size

Lumen:

Ray Tracing - 软光追，MeshSDF,GlobalSDF。硬件光追。

Radiance injection, Caching: 
* Cards，Lumen捕获光线的位置
* Mesh Card,Rutnime捕获，Mesh遮挡，Mesh的材质信息：Albedo,Normal,Depth,Emissive
  * Atlas ，Surface Cache，4096x4096
  * 显存压力大
![](https://pic2.zhimg.com/80/v2-27ceeb2c02e7ca70807ebb0c6453a985_1440w.webp)
* Indirect Light，Radiance cache
  * VoxelLighting 给下一帧
  * 使用上一帧的VoxelLighting得到Indirect Lighting。每帧逐渐积累。
  * 加上Surface cache的DirectLighting 得到这一帧当前lighting。

1. Lumen scene update 
   * Mesh card capture
   * CopyCardsToSurfaceCache(Alas)
   * BuildCardUpdateContext
2. Lumen scene direct lighting 
   * CullDirectLightingTiles
   * CombineLighting 
3. VoxelLigthing3D
   * UpdateVoxelVisBufferN with clipmap
   * TranslucencyVolumenLighting
4. LumenScreenProbeGather
  * UniformPlacement+AdaptivePlacement
  * RenderLumenScreenProbeGather
  * TraceScreen
  * TraceMeshSDF
  * TraceVoxels
  * CompositeTraces
  * TemporalReprojection
5. Lumen Reflection


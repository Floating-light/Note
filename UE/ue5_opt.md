Optimizing UE5
https://linktr.ee/epicmattoztalay?utm_source=qr_code

# Nanite
https://zhuanlan.zhihu.com/p/687141143

https://www.youtube.com/watch?v=xSUV8BahmI0&list=WL&index=3&t=272s

Rethinking Performance Paradigms for High-Quality Visuals - Part 1

https://www.youtube.com/watch?v=Cb63bHkWkwk&t=11963s
1:50:26 Lumen 

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

# Lumen

https://www.youtube.com/watch?v=Cb63bHkWkwk&t=11963s
1:50:26 Lumen 

Considerations
- SWRT vs HWRT
  - SWRT fast,good when you've got a lot of Kit bash stuff that's overlapping instances, 
  - HWRT high quality, a lot of shinned meshes that might be significantly affecting GI and mirror-like Reflections
  - 性能目标：
  - 30FPS ~8ms HW, for lighting and refections, Console
  - 60FPS ~4ms SW,
- Lumen Scene Lighting 
- Lumen Screen Probe Gather
- Lumen Reflections
- Async Compute
- Reponsiveness
- Mesh and Scene Construction

HWRT 
- BVH Traces
- Rebuild TLAS every frame
- - Instance Counts, Overlaps
- Trace Performance
- Features:
- - Hit Lighting
- - Far Field 

SWRT
- DF Traces
- GDF(Global distance field) Build 
- Independent of instance count 

Action Items
- HWRT
- - Instance Counts
- - Instanc Overlap
- SWRT
- - Affects DF 
- Roughness to Trace 


Matt Oztalay
## Real Action
* 超高（Epic） 级别对应的帧率为30FPS。 - 主机
* 高（High） 级别对应的目标帧率为60FPS。

试图让间接光照的品质在不同质量级别下保持相似，不启用Lumen：

中
* 对于大规模的环境光遮蔽， 距离场环境光遮蔽（Distance Field Ambient Occlusion） 会取代 Lumen全局光照（Lumen Global Illumination） 。
* 对于小规模的环境光遮蔽，会启用 屏幕空间环境光遮蔽（Screen Space Ambient Occlusion） 。

低
* 仅使用无阴影的天空光照。
* 降低天空光照强度（r.SkylightIntensityMultiplier=0.7），以近似模拟 中（Medium） 质量级别中的效果，因为此时没有天空光照阴影。

软光追
* 超高（Epic）会启用 细节追踪（Detail Traces），追踪单个Mesh的DSF，性能受实例数量和重叠实例数量影响。
  * 考虑禁用部分StaticMeshComponent的Affects Distance Field Lighting。
* 高（High）禁用DetailTraces，追踪合并后的全局距离场，不受实例数量及与其它实例重叠的影响。
* r.Lumen.TraceMeshSDFs.Allow

硬件光追

对实例数量敏感，对场景进行细致优化。
* 加速结构需要更复杂且更深的层级来代表这些重叠区域
* 当实例高度重叠时，为更好地判定哪个实例与光线相交，会带来更多的条件分支和复杂计算，这会影响执行效率。

主机，剔除后，光追场景实例数量少于10w。stat SceneRendering

Actor上禁用在光线追踪中可见（Visible In Ray Tracing）。

远场（Far Field） 激进剔除。

### Hint
Lumen反射开销会因为屏幕中`低粗造度材质`数量不同而变化。默认情况下，
* Roughness < 0.4 都会追踪反射光线，
* Roughness > 0.4 会根据Lumen全局光照，获得免费的反射近似效果。（无粗糙镜面反射近似值）

项目设置和后处理中可调整，
r.Lumen.Reflections.MaxRoughnessToTraceClamp 可在Scalability中调整。

双面植被（Two Sided Foliage） 或 次表面（Subsurface） 着色模型 都归为植被。
> r.Lumen.Reflections.MaxRoughnessToTraceForFoliage=0.4
植被的反射通常都不明显，可以关掉。

![LU_PerformanceOverview](../assets/UE/LU_PerformanceOverview.png)

### 用屏幕空间反射取代Lumen反射
r.Lumen.Reflections.Allow=0  屏幕空间反射 （SSR）取代Lumen反射

Xbox Series S 节约1ms。

![禁用了Lumen反射的Lumen GI镜面反射示例。](https://d1iv7db44yhgxn.cloudfront.net/documentation/images/7b589b85-88c6-40ef-833d-3bc331d6f008/lumen-gi-lumen-reflections-disabled.png)

### 表面缓存更新速度
直接光更新速度： r.LumenScene.DirectLighting.MaxLightsPerTile
间接光更新速度： r.LumenScene.Radiosity.UpdateFactor

"Lumen场景光照（Lumen Scene Lighting）"在每个表面缓存图块上只选择一小部分最重要的光源，这使其性能不太容易受到场景中光源总数的影响。每个图块的光源数量可由 r.LumenScene.DirectLighting.MaxLightsPerTile 控制。

三个

Lumen场景光照的性能取决于每帧更新的表面缓存的比例，此外还取决于半透明全局光照体积的分辨率。
- Lumen Scene Lighting 0.8 对表面缓存光照求值
  - r.LumenScene.DirectLighting.UpdateFactor
  - r.LumenScene.Radiosity.UpdateFactor

全局光照性能取决于内部渲染分辨率和屏幕探头追踪分辨率。
- Lumen Screen Probe Gather 2.88 用于对漫反射全局光照和粗糙反射以及半透明全局光照求值。
  - r.Lumen.ScreenProbeGather.RadianceCache.ProbeResolution 16 32 32 
  - r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget 300 300 1000
  - r.Lumen.ScreenProbeGather.DownsampleFactor 32 16 8 

Lumen反射（Lumen Reflections） 性能取决于专用反射光线的数量。只有粗糙度值低于阈值的像素才会被追踪。影响性能的另一个重要因素是内部渲染分辨率和反射分辨率。
- Lumen Reflections 1.77 用于对光滑表面上的专用反射光线求值。
  - r.Lumen.Reflections.MaxRoughnessToTrace
  - r.Lumen.Reflections.MaxRoughnessToTraceClamp
  - r.Lumen.Reflections.MaxRoughnessToTraceForFoliage
# Virtual Shadow Map
 Elevator Pitch 

 - High res geo? High res shadows!
 - Don't render all the shadows 
 - Virtualize shadow pages
 - Cache results
 - At ideal resolution

Considerations
- Shadow depths
- - Cache Invalidations
- - Geometry
- Shadow Projection 
- - Ray Count
- - Penumbra Size

Cache Invalidations
- Moving Camera 
- Moving Lights
- Moving Objects

StaticMeshComponent -> Shadow Cache invalidation Behavior
* 控制要不要在发生顶点偏移时InvalidShadow
* 可以在Runtime下更新

NaniteMesh被分为Cluster，可以以Cluster为单位绘制需要的区域。但是，非Nanite就只能整个对象都要绘制一遍。


r.Shadow.Virtual.OnePassProjection 1 多个光源也只有一次ShadowProjection
r.Shadow.Virutal.Visualize.LightName 

r.Shadow.Virutal.Visualize.Layout 2 
r.Shadow.Virutal.Visualize.Advanced 1


Action Items
- Shadow Depths 
- - Cache Invalidations
- Shadow Project
- - Soft Shadows, Overlaps

stat scenerendering
# World Partition

Other:

* Listenserver，Steam P2P，连续SeamlessTravel后，Client断开连接。在连续SeamlessTravel后，因为Client释放了一些GUID，但Server没有释放它，导致随后Server认为这个GUID仍然是有效的，但在Client上已经找不到，导致CLient因为解析不了这个GUID而强制断开连接。

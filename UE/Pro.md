
这几年一直从事UE引擎开发。我熟悉UE5引擎的各个模块，地形，渲染、Animation(渲染相关)、WorldParitition等。
熟悉各种调试工具，UnrealInsights、PIX、NSight graphics、RenderDoc等，用于排查项目中遇到的各种问题。过去3年，我主要深度参与了《重装前哨》的开发，一款RTS/FPS于一体的缝合怪游戏，我在其中主要负责引擎相关工作，包括大规模SkeltonMesh绘制、地形、CPU/GPU性能、风场动画、引擎一些功能的扩展，运行时随机生成植被，疑难BUG排查，扩展引擎支持Gameplay功能，部分gameplay系统的开发，等。

我热爱引擎技术，对游戏引擎尤其UE5有深刻的理解以及快速学习的能力，并善于挖掘(分析)引擎的底层实现原理，帮助项目更恰当地使用引擎功能，还能够在充分理解引擎实现原理的基础上恰当地扩展引擎功能。

---

深度参与《重装前哨》预研、开发、上线全过程，这是一款集RTS/FPS、生存建造等特性于一体的3D游戏，负责UE4/UE5引擎相关工作，根据项目需求制定技术方案，从技术预研，到方案制定、落地全流程。

* 制定并实现大规模敌人渲染方案，实现GPUIsntancedSkeletonMesh，在同时渲染5000个SkelotonMesh情况下，GPU耗时2ms，不同的Instance都有自己的动画，切动画时也有混合效果。GPU ComputeShader 剔除，FSceneViewExtensionBase。
  * Animation budget allocator
  * Vertex Animation，Notify，灵活，
  * GPU剔除
* 制定使用UE5WorldPartition制作大世界的规范，包括GridLayer划分策略、HLOD、DataLayer。
* 修改引擎源码，实现Runtime下修改地形，实现动态生成建筑时平滑地形、添加凹陷，弹坑等功能，包括地形材质、地形碰撞、Grass、导航网格在内都能正常发生改变。
* 扩展、修改引擎编辑器功能，加速美术、策划的工作。
  * 自动合批Instanced，runtime 随机生成植被，Blender风场动画Texture，
* 负责持续监控并优化游戏性能，为性能热点问题提供解决方案。
  * UnrealInsights，PIX，NSight Graphics，RenderDoc
  * 复杂碰撞，TriMesh，物理内存2G
  * 分帧操作，Parallel For，
  * 每次打包材质没有变化，ushaderbytecode，几百MB，都会变，Cook时资源加载顺序不一样，导致里面内容顺序不一样，修改源码，给他们按Hash排序。
  * 图形渲染设置，Nanite，Lumen，VSM，Grass
* 制定UE5 Nanite模型以及植被的使用规范，以达到最佳性能。
  * Mask，WPO，Rasterize BINS，Empty Draw，DrawCall。Blender，风场动画，
* 负责排查并修复任何疑难致命BUG，包括引擎或我们自己写的。
  * 连续SeamlessTravel 导致掉线
  * 各种迭代器失效导致的随机崩溃
  * TransientAllocator 导致的GPU崩溃，debuggpu，d3ddebug，
  * AMD显卡，编译PSO卡死
  * HISM 导航网格错误，
  * PVS 静态检查，UPROPERTY，内存泄漏
  * ISM负缩放时，导航网格生成在反面，生成导航网格是上下面的判断错误，检查到负缩放时，把Indices也反过来。
  * 当BodyInstance作为DataTable的RowStruct的Member时，在DataTableEditor中点击空的区域，会使用None RowName刷新DetailPanel，此时没有数据，会导致BodyInstnace的自定义PropertyPanel中的代码Check失败。
* 负责实现计算任意技能树UI布局算法(因为我们的技能树是完全随机的)，并申请专利。
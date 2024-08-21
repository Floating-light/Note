---
id: axegl1r91y6ibyrrbjq4xpy
title: Pro
desc: ''
updated: 1669540897556
created: 1669536613484
---

深度参与《重装前哨》预研、开发、上线全过程，这是一款集RTS/FPS、生存建造等特性于一体的3D游戏，负责UE4/UE5引擎相关工作，根据项目需求制定技术方案，从技术预研，到方案制定、落地全流程。

* 制定并实现大规模敌人渲染方案，实现GPUIsntancedSkeletonMesh，在同时渲染5000个SkelotonMesh情况下，GPU耗时2ms，不同的Instance都有自己的动画，切动画时也有混合效果。
* 制定使用UE5WorldPartition制作大世界的规范，包括GridLayer划分策略、HLOD、DataLayer。
修改引擎源码，实现Runtime下修改地形，实现动态生成建筑时平滑地形、添加凹陷，弹坑等功能，包括地形材质、地形碰撞、Grass、导航网格在内都能正常发生改变。
* 扩展、修改引擎编辑器功能，加速美术、策划的工作。
  * 自动合批，
* 负责持续监控并优化游戏性能，为性能热点问题提供解决方案。
  * UnrealInsights，PIX，NSight Graphics，RenderDoc
  * 复杂碰撞，TriMesh，物理内存2G
  * 分帧操作，Parallel For，
  * 每次打包材质没有变化，ushaderbytecode，几百MB，都会变，Cook时资源加载顺序不一样，导致里面内容顺序不一样，修改源码，给他们按Hash排序。
* 制定UE5 Nanite模型以及植被的使用规范，以达到最佳性能。
  * Mask，WPO，Rasterize BINS，DrawCall。Blender，风场动画，
* 负责排查并修复任何疑难致命BUG，包括引擎或我们自己写的。
  * 连续SeamlessTravel 导致掉线
  * 各种迭代器失效导致的随机崩溃
  * TransientAllocator 导致的GPU崩溃，debuggpu，d3ddebug，
  * AMD显卡，编译PSO卡死
  * HISM 导航网格错误，
  * PVS 静态检查，UPROPERTY，内存泄漏
* 负责实现计算任意技能树UI布局算法(因为我们的技能树是完全随机的)，并申请专利。
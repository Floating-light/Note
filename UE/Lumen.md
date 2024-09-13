Renference:
* [最强分析 ｜一文理解Lumen及全局光照的实现机制](https://zhuanlan.zhihu.com/p/643337359)
* [UE5 Lumen 源码解析（一）原理篇](https://zhuanlan.zhihu.com/p/499713106)

SDF:

* https://iquilezles.org/articles/distfunctions/
* [Raytracing and Raycasting with Signed Distance Functions](https://ben.land/post/2022/08/15/raycasting-raytracing-sdf/)
* [Signed Distance Fields Using Single-Pass GPU Scan Conversion of Tetrahedra](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-34-signed-distance-fields-using-single-pass-gpu)
* [UE5 Mesh Distance Field](https://dev.epicgames.com/documentation/en-us/unreal-engine/mesh-distance-fields-in-unreal-engine?application_version=5.0)

# BRDF
* [基于物理渲染的基础知识(辐射度量学，BRDF和渲染方程)](https://zhuanlan.zhihu.com/p/145410416)
* [PBRT4_Radiometry](https://pbr-book.org/4ed/Radiometry,_Spectra,_and_Color/Surface_Reflection)
* [BRDF PataBlog](https://www.patapom.com/blog/BRDF/BRDF%20Definition/)
* [Importance sampling - what](https://patapom.com/blog/Math/ImportanceSampling/)
* [Importance sampling - PDF to Actual Sampling](https://www.tobias-franke.eu/log/2014/03/30/notes_on_importance_sampling.html)
* [DDGI Probe - 预计算](https://zhuanlan.zhihu.com/p/404520592)

# UE5 Mesh SDF
把空间体素化(Volume texture)，每个点存的是它到最近的表面的距离，点在Mesh外则为正值，在Mesh内则为负值。
* Sphere tracing，从一个点出发，知道了离自己最近的Surface的距离（以当前点为中心，距离为半径的球范围内都没有别的Surface），可以直接向前进这么多距离，快速求交。
* Distance Field Ambient Occlusion
* [Using distance field for lighting](https://iquilezles.org/articles/raymarchingdf/)
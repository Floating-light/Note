---
id: 5v6dxkiak6x6ntf0iaemlri
title: ShaderDev
desc: ''
updated: 1699773374282
created: 1699772443313
---

# shader相关的Debug方式
* https://docs.unrealengine.com/5.3/zh-CN/graphics-programming-overview-for-unreal-engine/

Ctrl+Shift+. 或 recompileshaders changed   -- 重新编译上次保存 .usf 文件后发生变化的着色器。这将在加载后自动进行。

# 调试着色器编译过程
* https://docs.unrealengine.com/5.3/zh-CN/debugging-the-shader-compile-process-in-unreal-engine/

Engine/Config/ConsoleVariables.ini中：
r.ShaderDevelopmentMode=1


# 着色器调试工作流程
* https://docs.unrealengine.com/5.3/zh-CN/shader-debugging-workflows-unreal-engine/


## dump command 

dumpmaterialstats  // dump材质统计信息到MaterialStats-2020.12.15-13.12.23.csv

dumpparticlecounts  // 输出ParticleSystemComponent信息

dumpparticlemem  // 输出Particel的内存占用信息

particlemeshusage  // 打印particle中staticmesh使用情况

DumpShaderPipelineStats  // 输出Shader Pipeline统计信息到ShaderPipelineStats-2020.12.15-13.17.00.csv

dumpshaderstats  //  输出Shader统计信息到ShaderStats-2020.12.15-13.18.10.csv

rhi.DumpMemory   // 打印RenderTarget的内存信息

DumpAvailableResolutions  // 打印当前可使用的屏幕分辨率和刷新频率
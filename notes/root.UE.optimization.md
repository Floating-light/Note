---
id: jmbldi5n1nb3fr87vicrwsg
title: optimization
desc: ''
updated: 1667042620061
created: 1667027481667
---
UE 相关的优化操作。

# Asset reduction

优化Asset，以获取更好的渲染性能。

## LODs & Polygon Reduction

找到顶点计算消耗最大的对象

- r.screenpacentage 10 降低渲染的分辨率，排除PixelShader的影响

- r.RHISetGPUGaptureOptions 1, 在`ProgileGPU`中会显示每一个DrawCall的消耗

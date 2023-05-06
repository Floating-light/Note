---
id: qboqar9fpgks44ajllzaarl
title: Blender
desc: ''
updated: 1683338972281
created: 1683333234256
---

> Base Bledner version: 3.5

# Code Development

* https://github.com/blender/blender.git

* 构建步骤：https://wiki.blender.org/wiki/Building_Blender/Windows

* Debug python: https://wiki.blender.org/wiki/Tools/Debugging/Python_Visual_Studio

> make 2022 full nobuild pydebug

* FQA: https://wiki.blender.org/wiki/Reference/FAQ#Source_Code_FAQ

* Python API Doc : https://docs.blender.org/api/current/bpy.context.html

* Book: Core Blender Development - Understanding the Essential 
Source Code

* Code and Design Document: https://wiki.blender.org/wiki/Source

* 源码结构：https://wiki.blender.org/wiki/Source/File_Structure

## Addon

源码中没有插件，要自己下载。
Blender中重新加载插件： Command -> ReloadScript

* https://wiki.blender.org/wiki/Process/Addons
* https://docs.blender.org/manual/en/latest/advanced/scripting/addon_tutorial.html

### Development
导出UE中PivotPainter2使用的纹理资源工具：

```https://github.com/Floating-light/blender_addon.git```

可当作Blender插件开发的简单参考。源码中有许多Python脚本的示例`scripts\templates_py`,还有参考许多源码实现的功能和插件。

可以用VSCode开发，装插件Blender Development, 补全提示由https://github.com/nutti/fake-bpy-module


> TODO: 也许可以直接用Blender源码,加上编译选项pydebug，可以同时调试C 和 Python，Python的补全提示也可以用上面的fake-bpy-module，可以直接用pip装在对应版本的Python即可。

# Blender

* 对于UE, 要设置UnitScale = 0.01, 在ScenePropertyies -> Units -> UnitScale，然后在File-> Defaults -> SaveStartupFIle，可以让这个设置永久生效

* Edit -> Preferences -> Interface -> Display ->(勾上 Developer Extras和PythonTooltips)

                                    -> Editors -> StatusBar -> Show Scene Statistics
在Blender编辑器中，tooltips包含了对应的PythonAPI，例如 `bpy.ops.mesh.primitive_ico_sphere_add()`,其对应的C导出的函数为：`MESH_OT_primitive_ico_sphere_add`，可以直接搜索源码，看到实现。




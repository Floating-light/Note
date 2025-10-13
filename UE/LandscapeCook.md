# Patch size estimate
* [UE 热更新：资产管理与审计工具](https://imzlp.com/posts/3675/)
* [UE Cook分析](https://km.netease.com/v4/detail/blog/14195)
* [uasset，uexp](https://km.netease.com/wiki/1268/page/364050)
* AsyncWriteFileWithSplitExports() 生成uexp
* SaveBulkData() ubulk

打包后的文件通常有uasset，umap，uexp，ubulk等。
在编辑器下，通常就是一个uasset，打包时，在`AsyncWriteFileWithSplitExports()`中，会把数据的header和实际内容分开，header在uasset中，实际package中的对象数据在uexp中。

[uasset中就是package中记录的一些内容](https://km.netease.com/v4/detail/blog/3017)：
* Names
* Import Objects Array
* Export Object Array
即这个package引用的对象，以及它包含的对象。

uexp就是export object的内容。

## Material
通常美术直接修改的是UMaterial、UMaterialInstance。

对于UMaterial，如果修改了其中的节点：
* 如果修改了UMaterial引用的UObject(Texture)(path 发生了改变)，uasset也会发生变化
* uexp文件中，存了UMaterial本身，shadercode hash会变化，
* Project-PCD3D_SM5.ushaderbytecode文件，shadercode本身会变化。（开启share material shader code ）
* __Parent 改变一个texture引用，所有MI都会变化，UMaterialInstance::CachedReferencedTextures。__
  * UMaterialInstance::UpdateCachedLayerParameters()中，没有排除掉当前MI 排列没有用到的code path相关的texture。
  * Parent->GetReferencedTextures(); 仅仅是获取UMaterial中的所有引用的Texture。
  
其对应的MI：
* 如果MI有修改任何静态参数，static switch ,blendmode等，就需要用到与Parent不一样的代码路径，所以会生成自己的ShaderCode，导致对应的uasset，uexp都会发生改变(shader hash)
  * UMaterialInstance::UpdateStaticPermutation()
  * 这种情况下，Parent material的修改会导致对应MI也改变
  * parent __修改(非Texture)非静态path code__，则没有修改static option的MI（和Parent一样）不会变化。
  * parent 修改static path的code， 没有用到这个path的MI也会发生轻微改变 ？

* 如果MI没有修改静态排列
  * Parent material仅修改部分节点，增加一些材质参数，则不会导致MI的改变。（不改Texture引用）

> 难以判断具体修改内容，只能拿到一个改变过的文件List。

综上，先理解为：
1. UMaterial的改变会导致MI发生变化
2. MI的改变通常只会单独改变
   * 如果MI改变了Material property overrides，会导致shader code 改变
     * 怎么判断MI当前更改是改变了Param值还是Material property override？
3. 如果UMaterial被地形引用，那么当它改变时，对应的地形及其Map也会改变。
4. UMaterialFunction 的改变会导致引用它的UMaterial 和 MI 都变化，包括ShaderCode。
5. UMaterialParameterCollection 
6. USubsurfaceProfile 

## UObject序列化
Template object，differ

# reference
* [UE4里的ushaderbytecode](https://km.netease.com/wiki/1268/page/129533)
* [深入Unreal的UAsset - .uexp.ubulk](https://km.netease.com/v4/detail/blog/3017)



# 主流程
FSlateRHIRenderer::DrawWindow_RenderThread() 起源：`SlateUI Title = `, RenderThread

GameThread:

FontCache分为GameThread和RenderThread，可能是因为RenderThread有时候也会用上一些DebugDraw：
![rd_font_cache](../assets/UE/rd_font_cache.png)

每一帧的Tick中，`FSlateApplication::PrivateDrawWindows`是UI相关绘制的入口。`DrawWindowAndChildren(CurrentWindow, DrawWindowArgs)`搜集所有Window的绘制元素。

然后调用渲染器开始生成绘制数据`Renderer->DrawWindows( DrawWindowArgs.OutDrawBuffer );`
> FSlateRHIRenderer::DrawWindows()

对每一个元素调用`ElementBatcher->AddElements(*WindowElementList);`

> FSlateElementBatcher::AddElements(FSlateWindowElementList& WindowElementList)

其中就有Text相关的元素：

![rd_font_element](../assets/UE/rd_font_element.png)

`FSlateShapedTextElement`字形相关信息全在`FShapedGlyphSequencePtr ShapedGlyphSequence`，其父类`FSlateDrawElement`主要记录了当前元素的位置、缩放、Transform等信息。

`FSlateElementBatcher::AddShapedTextElement`

`FSlateElementBatcher::BuildShapedTextSequence()`，这里实际处理的是一个文本序列，对每一个字都会处理，会同时处理bitmap和sdf字体，创建字形数据，bitmap或sdf数据，并更新AtlasTexture。创建Vertex，Index数据，构建RenderBatch数据。

* Bitmap字体： `FSlateFontCache::GetShapedGlyphFontAtlasData`
* SDF字体：`FSlateFontCache::GetSdfGlyphFontAtlasData`

后面构建RenderBatch信息也只是简单if一下。这里会将每个字的RenderBatch信息记录到`FSlateElementBatcher`中。
这里WindowElement信息都处理完后，回到`FSlateRHIRenderer::DrawWindows_Private`，会调用`FSlateFontCache::UpdateCache()`，因为SDF字体数据的生成比较慢，前面生成是异步的，这里会调用`FSlateSdfGeneratorImpl::Update`处理所有生成的好的SDF数据，copy到Atlas纹理中。然后向渲染线程发送命令，更新字体Atlas纹理。

然后向渲染线程发送SlateDrawWindowsCommand的命令：

![rd_font_render_trigger](../assets/UE/rd_font_render_trigger.png)

# Bitmap字体生成和更新
`FSlateFontCache::GetShapedGlyphFontAtlasData`

`FSlateFontRenderer::GetRenderData(FShapedGlyphEntry...)`

`FSlateFontRenderer::GetRenderDataInternal()`

`FSlateFontCache::AddNewEntry()`
* `FontRenderer->GetRenderData()`得到当前字形的Pixels，直接从freetype2拿，
* 根据`ESlateFontAtlasContentType FCharacterRenderData::ContentType`，拿到对应的AtlasTexture，所有纹理都在FSlateFontCache::AllFontTextures，具体类型是`FSlateFontAtlasRHI`。
  * 对每一个Atlas都尝试调用`AddCharacter()`，现有的都没空间时，通过`FSlateRHIFontAtlasFactory::CreateFontAtlas`创建新的。
  * 如果新Entry的大小超过了Atlas的大小，就专门创建一个新Entry大小的Texture,`FSlateRHIFontAtlasFactory::CreateNonAtlasedTexture`

* `FSlateTextureAtlas::AddTexture()`将前面拿到的bitmap pixels数据copy到Atlas
  * 如何从Atlas中找到一块可用的区域`FSlateTextureAtlas::FindSlotForTexture`(InWidth, InHeight)：
  * 给宽高都加上Padding，量由FSlateTextureAtlas::PaddingStyle决定。
  * `AtlasEmptySlotsMap`中的元素是链表`FAtlasedTextureSlot`的头。
    * 其中的索引这样计算`FMath::CeilLogTwo(InWidth)`，按字形宽分桶。
    * 如果是1024*1024的Atlas，就会有10组。
    * 粗略分组减少了每次查找目标区域扫描的长度。
    * 仅用宽分组，因为实际字形大小的变化主要体现在宽度上？
    * 初始化的时候，直接把一整个Texture放到`AtlasEmptySlotsMap`中。
  * 从`FMath::CeilLogTwo(InWidth)`计算出开始搜索的桶索引，遍历`AtlasEmptySlotsMap`
    * 遍历链表
    * 找到一个可以容纳需要大小的Slot后，从左上角分出为本次找到的区域，剩下的区域分成两个Slot，再次放到`AtlasEmptySlotsMap`。
    * 更新本次找到的Slot的长宽，为需要使用的长宽，并Unlink，然后Link到`AtlasUsedSlots`
    * 首次分割后会形成两个空Slot，因为前面分桶的原因，经常可以看到字形在Atlas中是从上往下竖着排列的。
    * ![rd_font_split_slot](../assets/UE/rd_font_split_slot.png)
* `FSlateTextureAtlas::CopyDataIntoSlot()`，把Pixel数据copy到找到的Texture中的可用区域，`TArray<uint8> FSlateTextureAtlas::AtlasData`
* `FSlateTextureAtlas::MarkTextureDirty()` 标记一下，后面再上传到GPU。

# Bitmap字体生成和更新
`FSlateFontCache::GetSdfGlyphFontAtlasData`

利用freetype2和msdfgen生成距离场数据。和Bitmap不一样的只是生成的atlas数据代表的是距离场，渲染的时候用的Shader不一样罢了。


# Reference 
* [UE5 UMG的SDF字体渲染](https://zhuanlan.zhihu.com/p/3295334910)
* [【UE5】动态字体图集渲染](https://zhuanlan.zhihu.com/p/635341028)
* [Unreal Engine UI字体SDF渲染支持情况](https://zhuanlan.zhihu.com/p/1915374569801389273)
* [slug](https://sluglibrary.com)
* [UE中的Slug实现](https://codeartworks.com/2023/05/23/rendering-scalable-vector-text-in-unreal-engine/)
* [GPU Font Rendering](https://github.com/GreenLightning/gpu-font-rendering)
* [Multi-channel signed distance field generator](https://github.com/Chlumsky/msdfgen)
* [FreeType2](https://github.com/freetype)
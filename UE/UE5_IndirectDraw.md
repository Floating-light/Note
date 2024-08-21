利用FSceneViewExtensionBase，注入自定义的CS Pass，对大量Instance进行剔除，构建IndirectDrawArgumentBuffer，用ExecuteIndirect实现绘制。

关键是构建FMeshBatchElement::IndirectArgsBuffer.

* 参考引擎实现的FLandscapeSceneViewExtension、ApplyViewDependentMeshArguments()，实现地形Tile的剔除。

* FWaterViewExtension::PreRenderBasePass_RenderThread(）、FWaterMeshSceneProxy::GetDynamicMeshElements()也用到了IndirectDraw。

* FNiagaraRendererMeshes::CreateMeshBatchForSection()
应用场景：
* 
refernece: https://zhuanlan.zhihu.com/p/614758211


每个Primitive的数据，LocalToRelativeWorld，CustomPrimitiveData等数据，Shader中对应结构体：FPrimitiveSceneData。

FPrimitiveUniformShaderParameters

FPrimitiveSceneProxy::CreateUniformBuffer()
FPrimitiveSceneProxy::UpdateUniformBuffer()

FInstanceSceneShaderData ， Shader中FInstanceSceneData。


# Shader
BasePassVS中：
通过它获取`FInstanceSceneData`和`FPrimitiveSceneData`,

用的是`（FVertexFactoryInput）Input .InstanceIdOffset, Input .DrawInstanceId`，

uint InstanceIdOffset : ATTRIBUTE13 ; 			uint DrawInstanceId : SV_InstanceID;

FSceneDataIntermediates GetSceneDataIntermediates(uint InstanceIdOffset, uint DrawInstanceId)

被存到FVertexFactoryIntermediates.Common中。

开始绑定：FMeshPassProcessor::BuildMeshDrawCommands() 顶点数据，各种View。

在VS里，用GetPrimitiveData从PrimitiveIndex获取FPrimitiveSceneData。
对应到Shader中，G:\workspace\UnrealEngine\Engine\Shaders\Private\SceneData.ush Line351

```c++
float4 LoadPrimitivePrimitiveSceneDataElement(uint PrimitiveIndex, uint ItemIndex)
{
	uint TargetIdx = PrimitiveIndex + ItemIndex;

#if SHADER_USES_PRIMITIVE_UBO
	return 0; // FIXME LoadPrimitivePrimitiveSceneDataElement for UBO case
#elif USE_GLOBAL_GPU_SCENE_DATA
	checkStructuredBufferAccessSlow(GPUScenePrimitiveSceneData, TargetIdx);
	return GPUScenePrimitiveSceneData[TargetIdx];
#elif USE_GLOBAL_GPU_SCENE_DATA_RW
	checkStructuredBufferAccessSlow(GPUScenePrimitiveSceneDataRW, TargetIdx);
	return GPUScenePrimitiveSceneDataRW[TargetIdx];
#else
	checkStructuredBufferAccessSlow(Scene.GPUScene.GPUScenePrimitiveSceneData, TargetIdx);
	return Scene.GPUScene.GPUScenePrimitiveSceneData[TargetIdx];
#endif
}
```
Mesh的顶点数据中的PrimitiveId，即Shader中的InstanceIdOffset，在Draw的时候动态绑定，与其它数据在构建顶点工厂时就绑定不一样：
![GS_PrimitiveIdVertexBufferPool](../assets/UE/GS_PrimitiveIdVertexBufferPool.png)
```
DrawDynamicMeshPassPrivate()
  SortAndMergeDynamicPassMeshDrawCommands中构建Primitiveindex
	TranslatePrimitiveId处理索引，最高位。
  SubmitMeshDrawCommandsRange
    FMeshDrawCommand::SubmitDraw(
		FMeshDrawCommand::SubmitDrawBegin
			RHICmdList.SetStreamSource(Stream.StreamIndex, SceneArgs.PrimitiveIdsBuffer, SceneArgs.PrimitiveIdOffset);
		FMeshDrawCommand::SubmitDrawEnd 提交绘制
```

# Static Mesh 渲染流程：
Primitive通过ENQUEUE_RENDER_COMMAND发送渲染线程命令，FScene::PrimitiveSceneInfo_RenderThread()，添加到FScene的成员变量AddedPrimitiveSceneInfos中，在下一次渲染开始的FScene::Update()中处理。
```c++
FScene::Update()
	//对新加的Primitive
	FPrimitiveSceneInfo::AddStaticMeshes()
		Proxy->DrawStaticElements(&BatchingSPDI)
		dithered LOD
	FPrimitiveSceneInfo::CacheMeshDrawCommands(this, SceneInfosWithStaticDrawListUpdate);
	* 用一个DrawContext FCachedPassMeshDrawListContextImmediate DrawListContext(*Scene);
	* 对每个FPrimitiveSceneInfo，的每个StaticMeshes，搜集起来处理
		* 对每个Pass - EMeshPass::Num，创建CreateMeshPassProcessor
		* 对每个MeshBatch调用Processor->AddMeshBatch()
	* 最终每个StaticMeshes，创建与EMeshPass::Num相同数量的Command，在StaticMeshCommandInfos。
	* 最终是存在了FScene的成员变量上FCachedPassMeshDrawList CachedDrawLists[EMeshPass::Num];
```
[只有在绘制状态不每帧都改变，且可以在AddToScene内设置所有着色器绑定时，才能缓存的绘制命令。](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/mesh-drawing-pipeline-in-unreal-engine?application_version=5.3#%E7%BC%93%E5%AD%98%E7%9A%84%E7%BD%91%E6%A0%BC%E4%BD%93%E7%BB%98%E5%88%B6%E5%91%BD%E4%BB%A4)


# DynamicMesh渲染流程

// 先处理剔除，对所有通过剔除的Mesh进行
// 对所有GatherDynamicMesh进行操作
FVisibilityTaskData::SetupMeshPasses()//拿到所有可见的DynamicMesh
FSceneRenderer::SetupMeshPass()//对每一个Pass创建Processor
	// InstanceCullingContext，DynamicMeshElements
	// 对每个Pass进行处理，EMeshPass::Num
	FParallelMeshDrawCommandPass::DispatchPassSetup(Processor) 
	FMeshDrawCommandPassSetupTask::AnyThreadTask()
	GenerateDynamicMeshDrawCommands() // 构建FDynamicPassMeshDrawListContext
			PassMeshProcessor->SetDrawListContext(&DynamicPassMeshDrawListContext);
			PassMeshProcessor->AddMeshBatch(*MeshAndRelevance.Mesh, BatchElementMask, MeshAndRelevance.PrimitiveSceneProxy);
			FBasePassMeshProcessor::TryAddMeshBatch()
				FBasePassMeshProcessor::Process(
				FMeshPassProcessor::BuildMeshDrawCommands
					往DrawListContext添加FMeshDrawCommand，	FMeshCommandOneFrameArray& DrawList;


FViewInfo::(FParallelMeshDrawCommandPass)ParallelMeshDrawCommandPasses所有Pass对应的FMeshDrawCommandPassSetupTask，包含所有FMeshDrawCommand。

FDeferredShadingSceneRenderer::RenderBasePassInternal()
  //对所有View
  FParallelMeshDrawCommandPass::DispatchDraw()
	FDrawVisibleMeshCommandsAnyThreadTask
		FInstanceCullingContext::SubmitDrawCommands()
		FMeshDrawCommand::SubmitDraw()
		FMeshDrawCommand::SubmitDrawEnd()
  Nanite::DrawBasePass()//然后Nanite

# reference 
* https://zhuanlan.zhihu.com/p/657669302
* https://zhuanlan.zhihu.com/p/651173532
* 渲染管线原理机制源码剖析 https://zhuanlan.zhihu.com/p/641367884
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
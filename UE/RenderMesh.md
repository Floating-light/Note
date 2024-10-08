* https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-for-unreal-engine

* [InstancedDraw ISM HISM](https://www.jianshu.com/p/a086bd856634)
* [模型绘制机制](https://blog.csdn.net/ttod/article/details/134377780)


一种情况，创建一个AStaticMeshActor，调用SetStaticMesh:
* UStaticMeshComponent::SetStaticMesh()
  * 如果已经创建了渲染状态
    * MarkRenderStateDirty()
    * bRenderStateDirty = true;
    * MarkForNeededEndOfFrameRecreate()
  * 没有，则马上创建，RecreateRenderState_Concurrent()
    * UPrimitiveComponent::CreateRenderState_Concurrent()
    * 用UWorld上的`FSceneInterface* Scene;`进行创建
      * FScene::AddPrimitive() ->  FScene::BatchAddPrimitivesInternal<>()

这里通过这段代码创建`FPrimitiveSceneProxy`：

![RM_create_sceneproxy](../assets/UE/RM_create_sceneproxy.png)

这里的`GetPrimitiveComponentInterface()`是在`UPrimitiveComponent`中用一个宏实现的:
```c++
UE_DECLARE_COMPONENT_ACTOR_INTERFACE(PrimitiveComponent)
```
![SM_expand_interface](../assets/UE/SM_expand_interface.png)

就是定义了一个`FActorPrimitiveComponentInterface`成员变量，它实现了接口`IPrimitiveComponent`，全是获取`UPrimitiveComponent`状态相关的接口，里面的实现都是调用前面的宏实现的`UPrimitiveComponent::GetPrimitiveComponent()`，用地址偏移，得到当前`FActorPrimitiveComponentInterface`成员变量所在对象的地址，不理解为什么一定要这么做(=_=)。例如这里用到的
`CreateSceneProxy()`:

![RM_InterfaceCreateProxy](../assets/UE/RM_InterfaceCreateProxy.png)

将创建好的`FPrimitiveSceneProxy`和`FPrimitiveSceneInfo`构成一个`FCreateCommand`，用`ENQUEUE_RENDER_COMMAND`送到渲染线程处理：

![RM_addprimitivecommand](../assets/UE/RM_addprimitivecommand.png)

等RenderThread执行到这个命令时，最终把PrimitiveSceneInfo添加到FScene中：

![RM_scene_addPrimitive](../assets/UE/RM_scene_addPrimitive.png)

这里的AddedPrimitiveSceneInfos会在`FDeferredShadingSceneRenderer::Render()`中处理，在`FSceneRenderer::OnRenderBegin()`的最后调用`FScene::Update()`，更新渲染Scene的相关信息，处理各种Primitive上的数据改变，包括Primitve添加删除、Transform、VirtualTexture更新等。处理新加进来的`AddedPrimitiveSceneInfos`，完成后清空它。

构建GPUScene，场景有多少个`FPrimitiveSceneInfo`就要构建多少个实例。

AddStaticMesh, StaticPath

![RM_StaticPass](../assets/UE/RM_StaticPass.png)

初始化View，计算Visibiliy，搜集可见元素的Mesh。
```
FDeferredShadingSceneRenderer::BeginInitViews()
  FVisibilityTaskData::ProcessRenderThreadTasks()
    FVisibilityViewPacket::BeginInitVisibility()
      如果场景种FPrimitiveSceneInfo太多，这里会创建超级多的数据，用于计算每个Primitive的Visibility
      每个Primitive至少循环处理一次
      UpdateAlwaysVisible() 处理标记为总是可见的对象
      SceneVisibility.cpp FrustumCull() 处理所有Primitive的视锥剔除
        Scene.PrimitiveBounds[Index] 半径小于0直接剔除
        FPrimitiveSceneInfo::AddToScene()中初始化为Proxy->GetBounds()

```

void FRelevancePacket::ComputeRelevance(FDynamicPrimitiveIndexList& DynamicPrimitiveIndexList)
对所有可见的Primitive计算相关性，
  PrimitiveSceneProxy->GetViewRelevance(&View);
  DynamicPrimitiveViewMasks->Primitives[PrimitiveIndex]
  FDynamicMeshElementContext::GatherDynamicMeshElementsForPrimitive(FPrimitiveSceneInfo* Primitive, uint8 ViewMask)


FNaniteMeshProcessor

# Primitive的Transform和Bound的更新
```c++
UActorComponent::DoDeferredRenderUpdates_Concurrent()
		if(bRenderTransformDirty)
      void UPrimitiveComponent::SendRenderTransform_Concurrent()
        void FScene::UpdatePrimitiveTransformInternal(T* Primitive)
          ENQUEUE_RENDER_COMMAND Scene->UpdatePrimitiveTransform_RenderThread()
            添加到FScene::UpdatedTransforms
```
随后在FScene::Update中处理
* FPrimitiveSceneProxy::SetTransform()

# 移除Primitive
实际也是在FScene::Update中delete的。

![RM_DeletePrimitive](../assets/UE/RM_DeletePrimitive.png)

实际是先把要移除的Primitive添加到RemovedPrimitiveSceneInfos中。通过FScene::RemovePrimitiveSceneInfo_RenderThread()。


从GameThread触发：
```c++
void UActorComponent::DestroyRenderState_Concurrent()
void UPrimitiveComponent::DestroyRenderState_Concurrent()override
  FScene::RemovePrimitive(UPrimitiveComponent* Primitive)
    FScene::BatchRemovePrimitivesInternal(TArrayView<T*> InPrimitives)
      ENQUEUE_RENDER_COMMAND FScene::RemovePrimitiveSceneInfo_RenderThread()
```

GameThread多个地方会触发DestroyRenderState：
```
UActorComponent::ExecuteUnregisterEvents()
UActorComponent::RecreateRenderState_Concurrent()
```

UStaticMesh的IndexBuffer、VertexBuffer创建、释放时机：
* 创建，加载资源好了之后就从原始数据初始化各种Buffer、VertexFactory，发送渲染命令到RenderThread创建RHIBuffer，Resource->InitResource(RHICmdList);
* 释放， BeginDestroy的时候，发命令到RenderThread调用Resource->ReleaseResource();
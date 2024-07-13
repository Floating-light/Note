# 1. HLOD相关配置
Actor会BuildHLOD的条件

* Actor开启bIsSpatiallyLoaded
* AActor::IsHLODRelevant() return true
  * 开启Include Actor in HLOD
  * 至少一个Component的IsHLODRelevant() return true:
    * Mobility != EComponentMobility::Movable
    * 开启Include Component in HLOD
满足这些条件后，Actor必会生成HLOD，没有配置HLODLayer则会给它分配WorldSetting中默认的HLODLayer。

不同的RuntimeHash对HLOD的配置处理不一样。
* UWorldPartitionRuntimeHashSet
  * HLOD与Grids配置在一起
  * 每个Grid配置有单独的HLODLayers配置
  * 这里可以给需要SpatiallyLoaded的HLODLayer配置PartitionLayer，配置LoadingRange
    * 由此生成的HLODActor会被分配至名为`GridName:HLODLayerName`的Grid，
    * 也就是说这样的HLODLayer会单独有一个Grid，这可能造成Cell数量过多。
  * 不需要SpatiallyLoad的HLODLayer生成的HLODActor会直接放进持久Cell。
  * Grid中的Actor，如果开启了自动HLOD，则它配置的HLODLayer必须出现在自己所在Grid的配置中。
  
* UWorldPartitionRuntimeSpatialHash
  * WorldSetting上的Grid配置不处理任何HLOD配置
  * 在HLODLayer上有开启SpatiallyLoaded的配置，开启后可以配置对应的CellSize和LoadingRange。
  * Build HLOD时，会生成这些HLODActor的SpatialHashGrid配置，直接是一个`ASpatialHashRuntimeGridInfo`。
  * 在`UWorldPartitionRuntimeSpatialHash::GenerateStreaming`中，会先搜集所有`ASpatialHashRuntimeGridInfo`的Grid配置，把他们与WorldSetting上的GridSetting一起处理。

# BuildHLOD
通常通过命令行构建，主要功能入口在:
```
UWorldPartitionHLODsBuilder::RunInternal()
```

BuildHLOD的大致流程：
* UWorldPartitionHLODsBuilder::SetupHLODActors()
  * UWorldPartition::SetupHLODActors()
  * 前面有着和BuildStreamingCell时一样的流程
    * 利用`FWorldPartitionStreamingGenerator`将所有Actor划分好Cluster。构成ActorSet。
  * 然后用整理好AsetSet进入UWorldPartitionRuntimeHash::SetupHLODActors()处理。

对于`UWorldPartitionRuntimeHashSet`：
* 与`GenerateStreaming`中同样的方法，先生成StreamingCell。
* 用`FWorldPartitionHLODUtilities::CreateHLODActors`对每个Cell创建AWorldPartitionHLOD。
  * 对Cell中所有HLODRelevant的Actor，`UHLODLayer`相同的划分在一起。
  * 每一个HLODLayer都会创建一个对应的AWorldPartitionHLOD(如果存在用了这个Layer的Actor)。
  * 如果对应的AWorldPartitionHLOD存在，就用旧的，不存在就用new一个。
    * 生成的AWorldPartitionHLOD会和所在Cell有一样的DataLayer
    * AWorldPartitionHLOD的成员变量`UWorldPartitionHLODSourceActors`记录了它对应的所有SourceActor。
    * AWorldPartitionHLOD上的设置大部分都是从UHLODLayer上同步来的。
  * 如果这个HLODLayer有ParentLayer，这个ParentLayer会作为HLODActor的HLODLayer，相当于下一级HLOD，然后进入下一级HLOD的生成。
    * 用所有新生成的AWorldPartitionHLOD构成一个`FHLODStreamingGenerationContext`，替代原来的Context，再走一遍和之前同样的AWorldPartitionHLOD生成流程。

对于`UWorldPartitionRuntimeSpatialHash`:
* 遍历所有Actor，拿到所有UHLODLayer和ParentLayer。
* 对开启了bIsSpatiallyLoaded的HLODLayer生成对应的`FSpatialHashRuntimeGrid`，CellSize和LoadingRange等参数从对应HLODLayer的配置上获得。
* 和上面一样，先把所有Actor划分到对应的Grid。
* 然后对每个Cell创建对应的`AWorldPartitionHLOD`。
  * 先创建HLOD0的`AWorldPartitionHLOD`。
  * 然后对新的`AWorldPartitionHLOD`创建下一级HLOD。
结果与`UWorldPartitionRuntimeHashSet`保持一致，都是每个Cell的每个HLODLayer都创建了一个对应的`AWorldPartitionHLOD`。

然后进入下一步`UWorldPartitionHLODsBuilder::BuildHLODActors()`:
* 获取所有的`AWorldPartitionHLOD`，排序，Child HLOD必须在Parent HLOD之前Build。
* AWorldPartitionHLOD::BuildHLOD()
* FWorldPartitionHLODUtilities::BuildHLOD(AWorldPartitionHLOD* InHLODActor)
  * HLODActor中的`UWorldPartitionHLODSourceActorsFromCell* SourceActor`保存了这个HLODActor对应的所有原始Actor。
  * 先加载它们：`UWorldPartitionHLODSourceActorsFromCell::LoadSourceActors`
  * 获取所有HLODRelevantComponents。
  * 将HLODActor(可以复用以前的)的HLODHash，与HLODActor的信息和HLODRelevantComponents计算的Hash比较，不同则说明数据发生变化，需要重新构建。否则跳过构建。
  * 根据配置确定不同的Builder：FWorldPartitionHLODUtilities::GetHLODBuilderClass(),后面再单独介绍。
    * Instancing
    * MergedMesh
    * SimplifiedMesh
    * ApproximatedMesh
    * Custom
  * 构建`FHLODBuildContext HLODBuildContext;`其中包含了所有要构建HLOD的Component。
  * HLODLayer还有个HLODModifier配置，这里会在调用Builder的构建之前
    * UWorldPartitionHLODModifier::BeginHLODBuild(HLODBuildContext)
    * 在Builder构建完之后，对生成的HLODComponent调用UWorldPartitionHLODModifier::EndHLODBuild(Comps).
    * 这里可以对构建HLOD的Component和构建结果作自定义的修改。
  * 构建的结果是一堆`UActorComponent`，通常可能是`UStaticMeshComponent`或`InstancedStaticMeshComponent`。
  * 然后把这些Component Attach到`AWorldPartitionHLOD`上。
  * 对于构建好HLODComponent，会强行设置一些东西：
    * 关闭EverAffectNavigation
    * 关闭碰撞，关闭CanCharacterStepUpOn，关闭GenerateOverlapEvents

## UWorldPartitionHLODModifier的用法
引擎中有一个案例`UWorldPartitionHLODModifierMeshDestruction`。

当采用MergedMesh这种把多个Mesh合成一个的HLODLayerType时，将无法移除单个Mesh。Instance的合并方法是合并成`UHierarchicalInstancedStaticMeshComponent`，是可以移除单个的。

`UWorldPartitionHLODModifierMeshDestruction`里面实现了监听MergedMesh的生成，并将ActorId写入Mesh的VertexColor，通过材质的MaskOpacity控制合并后的Mesh中部分显示出来，达到移除部分Actor的效果。`EndHLODBuild()`时，会把`UWorldPartitionDestructibleHLODMeshComponent`添加进HLODComponents，里面实现了Runtime下对单个Actor造成伤害和DestroyActor的逻辑，本质就是控制对应的材质参数、`VisibilityTexture`，达到销毁单个Actor的效果。甚至还包括了网络同步。

## HLOD Layer type
不同的Type实现了不同的Mesh合并方式。

## Instance
`UHLODBuilderInstancing`将相同的Mesh合并成InstancedStaticMesh。默认使用的是`UHLODInstancedStaticMeshComponent`。可以在Edtior的Config中配置：
```
GConfig->GetString(TEXT("/Script/Engine.HLODBuilder"), TEXT("HLODInstancedStaticMeshComponentClass"), ConfigValue, GEditorIni);
```
合并成InstancedMesh是在`UHLODBuilder::BatchInstances`中实现的：
* 仅处理`UStaticMeshComponent`。
* 利用`UHLODInstancedStaticMeshComponent`对应的`FISMComponentDescriptor`，将StaticMesh的大量属性生成一个Hash，只有Hash相同，即几乎所有`UStaticMeshComponent`的属性设置都相同的才能合成一个ISM。
  * 所以必须谨慎考虑`UStaticMeshComponent`的属性设置，虽然Mesh相同，但大量不同的Component属性设置会导致无法合并成一个ISM。
* `UHLODBuilderInstancing`中还有一个配置`bDisallowNanite`，如果开启，且ISM的Mesh开启了Nanite，且LOD数量大于一，则会关闭这个ISM的Nanite。

## MeshMerge
把所有StaticMeshComponent合并成一个StaticMesh。主要功能实现在`MeshMergeUtilities`模块中。
![HLOD_MergeMesh](../assets/UE/HLOD_MergeMesh.png)

`UPrimitiveComponent`上有一个`HLODBatchingPolicy`，可以指定使用最后一级LOD用于合并。

`FMeshMergeUtilities::MergeComponentsToStaticMesh`实现了合并的逻辑。
* 
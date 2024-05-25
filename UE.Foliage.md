---
id: 37s5hfuk2dpjc0ibrx5xobo
title: Foliage
desc: ''
updated: 1680854587627
created: 1680838069263
---

# Procedural Foliage LandscapeLayer filtter

LandscapeLayerCheck()

## 密度处理

G:\UnrealEngine5\Engine\Source\Runtime\Foliage\Private\ProceduralFoliageTile.cpp Line: 487

先按固定的算法给出较为密集的散点，再以设置的密度概率过滤掉一些点。

- 获取一系列的散点
  - G:\UnrealEngine5\Engine\Source\Runtime\Foliage\Private\ProceduralFoliageSpawner.cpp Line: 111
- 按Tile拼接，并按密度曲线过滤 UProceduralFoliageComponent::GenerateProceduralContent() EditorOnly
  - UProceduralFoliageTile::ExtractDesiredInstances() Line 487  --> FDesiredFoliageInstance : StartTrace, EndTrace
  - 得到随机区域全量的随机散点的Trace信息
- FEdModeFoliage::AddInstances() EditorOnly
  - 构建Map `UFoliageType -> TArray<FDesiredFoliageInstance>`
  - `FEdModeFoliage::AddInstancesImp(UFoliageType, TArray<FDesiredFoliageInstance>)`
  - FEdModeFoliage::CalculatePotentialInstances_ThreadSafe()
    - 对每一个随机出来的点，进一步执行过滤条件 
      - AInstancedFoliageActor::FoliageTrace() 应该放在什么COmponent上，确定放的具体位置，可以放的Mesh类型的过滤
      - 得到`TArray<FPotentialInstance>`
      - 找到IFA添加对应的FoliageType ： AInstancedFoliageActor::AddFoliageType()
      - 得到`TArray<FFoliageInstance>`，

      - FoliageEdMode.cpp `SpawnFoliageInstance(UFoliageType, TArray<FFoliageInstance>)`
      - 从IFA找到FFoliageInfo IFA->AddFoliageType(Settings, &Info);
      - 添加实例： Info->AddInstances(FoliageSettings, PlacedLevelInstances.Value); // 仅在Editor 下保存Instance
        - FFoliageInfo::AddInstancesImpl
          - Implementation的实现为FFoliageStaticMesh
      
      
      
  

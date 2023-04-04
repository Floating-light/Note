---
id: 32yjtl1nooo27rdlxg0uk98
title: GameModes
desc: ''
updated: 1680572239823
created: 1680482435878
---

Lyra中不同的游戏模式由一个`ULyraExperienceDefinition`定义。里面定义了一种游戏模式所需的初始化数据，包括：
- 需要开启的`GameFetaure`
- 默认的`PawnData`
- 需要执行的`GemaFeatureAction`
- 其它需要执行的`ActionSet(ULyraExperienceActionSet)`
可以看出，除了PawnData，其它设置虽有不同，但本质都是执行GameFeatureAction，只是组织方式不同，方便不同程度的配置复用。

## 初始化
因为要支持多人游戏，所以配置的初始化最终应该在GameState上执行，且所有端都应该执行。所以定义了一个`ULyraExperienceManagerComponent`负责初始化一个`Experience`，其中的方法开始了这一过程：
```c++
void ULyraExperienceManagerComponent::ServerSetCurrentExperience(FPrimaryAssetId ExperienceId)
{
	ULyraAssetManager& AssetManager = ULyraAssetManager::Get();
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId);
	TSubclassOf<ULyraExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad());
	check(AssetClass);
	const ULyraExperienceDefinition* Experience = GetDefault<ULyraExperienceDefinition>(AssetClass);

	check(Experience != nullptr);
	check(CurrentExperience == nullptr);
	CurrentExperience = Experience;
	StartExperienceLoad();
}
```
显然这是在Server上调用的方法，给定一个Experience DataAsset的Id，然后开始加载，这个Id会被设置为CurrentExperience，这个变量会被同步到Client，Client收到这个Id后，在On_Rep中调用同样的初始化方法`StartExperienceLoad()`。

Server是GameMode在InitGame时执行了确定要使用的ExperienceId的方法`ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne()`， 这里会被延迟到下一帧执行，估计是为了确保GameMode系统完全初始化。通常，为了方便开发，需要为Experience的选择提供足够的灵活性，这里按优先级确定Experience：
- Matchmaking assignment (if present)
  - 玩家的正常游戏流程，会在给GameMode指定的OptionsString中设置Experience。
  - OptionsString 是OpenLevel时写在URL里的，可以用UGameplayStatics::HasOption()等方法解析。
  - 格式类似于：/ShooterMaps/Maps/L_Expanse?listen?Experience=B_ShooterGame_Elimination
- Developer Settings (PIE only)
- Command Line override 指定'Experience='
- World Settings 
- Default experience 写死的一个Experience

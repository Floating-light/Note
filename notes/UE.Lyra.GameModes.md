---
id: 32yjtl1nooo27rdlxg0uk98
title: GameModes
desc: ''
updated: 1680832945846
created: 1680482435878
---

Lyra中不同的游戏模式由一个`ULyraExperienceDefinition`定义。里面定义了一种游戏模式所需的初始化数据，包括：
- 需要开启的`GameFetaure`
- 默认的`PawnData`
- 需要执行的`GemaFeatureAction`
- 其它需要执行的`ActionSet(ULyraExperienceActionSet)`
可以看出，除了PawnData，其它设置虽有不同，但本质都是执行GameFeatureAction，只是组织方式不同，方便不同程度的配置复用。

## 初始化
在Server上，GameMode在InitGame时执行了寻找要使用的ExperienceId的方法`ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne()`， 这里会被延迟到下一帧执行，估计是为了确保GameMode系统完全初始化。通常，为了方便开发，需要为Experience的选择提供足够的灵活性，这里按优先级确定Experience：
- Matchmaking assignment (if present)
  - 玩家的正常游戏流程，会在给GameMode指定的OptionsString中设置Experience。
  - OptionsString 是OpenLevel时写在URL里的，可以用UGameplayStatics::HasOption()等方法解析。
  - 格式类似于：/ShooterMaps/Maps/L_Expanse?listen?Experience=B_ShooterGame_Elimination
  - [[UE.Lyra.UI.ExperienceSelection]]
- Developer Settings (PIE only)
- Command Line override 指定'Experience='
- World Settings 
- Default experience 写死的一个Experience

因为要支持多人游戏，所以配置的初始化最终应该在GameState上执行，且所有端都应该执行。所以定义了一个`ULyraExperienceManagerComponent`负责初始化`Experience`。

GameMode确定Experience后，找到GameState上的`ULyraExperienceManagerComponent`：
```c++
// 这里用FindComponent，而不是直接访问方法或成员变量，使得耦合更低，只依赖于AActor中的方法
ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
check(ExperienceComponent);		
ExperienceComponent->ServerSetCurrentExperience(ExperienceId);
```
开始执行真正的Experience初始化：
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

## StartExperienceLoad
在确定好CurrentExperience后，所有端都会开始加载Experience。这里主要就是处理加载Experience中的软引用资产。其中包括`FPrimaryAssetId`中的Bundle，根据项目自定义逻辑添加的与Experience直接相关的软引用资产：
```c++
TSet<FPrimaryAssetId> BundleAssetList;
TSet<FSoftObjectPath> RawAssetList;
```
`BundleAssetList`除了Experience本身，还包括所有它引用的DataAsset，因为这些DataAsset可能通过`UpdateAssetBundleData()`的自定义逻辑添加Bundle，而不是仅仅通过属性上的Bundle标记。
```c++
BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());
for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
{
	if (ActionSet != nullptr)
	{
		BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
	}
}
```
然后需要确定加载的Bundle:
```c++
TArray<FName> BundlesToLoad;
BundlesToLoad.Add(FLyraBundles::Equipped);

//@TODO: Centralize this client/server stuff into the LyraAssetManager
const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
const bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer);
const bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client);
if (bLoadClient)
{
	BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
}
if (bLoadServer)
{
	BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
}
```
通常这些Bundle的定义都应该放在AssetManager中，用Static变量定义。

随后用AssetManger的方法加载这些资产，并绑定回调：
```c++
const TSharedPtr<FStreamableHandle> BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(BundleAssetList.Array(), BundlesToLoad, {}, false, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
const TSharedPtr<FStreamableHandle> RawLoadHandle = AssetManager.LoadAssetList(RawAssetList.Array(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, TEXT("StartExperienceLoad()"));

// If both async loads are running, combine them
TSharedPtr<FStreamableHandle> Handle = nullptr;
if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
{
	Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({ BundleLoadHandle, RawLoadHandle });
}
else
{
	Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
}

FStreamableDelegate OnAssetsLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);
if (!Handle.IsValid() || Handle->HasLoadCompleted())
{
	// Assets were already loaded, call the delegate now
	FStreamableHandle::ExecuteDelegate(OnAssetsLoadedDelegate);
}
else
{
	Handle->BindCompleteDelegate(OnAssetsLoadedDelegate);

	Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda([OnAssetsLoadedDelegate]()
		{
			OnAssetsLoadedDelegate.ExecuteIfBound();
		}));
}
```

最后还可以加载一些不需要阻塞初始化流程的预加载资产：
```c++
// This set of assets gets preloaded, but we don't block the start of the experience based on it
TSet<FPrimaryAssetId> PreloadAssetList;
//@TODO: Determine assets to preload (but not blocking-ly)
if (PreloadAssetList.Num() > 0)
{
	AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
}
```
## OnExperienceLoadComplete
加载完资产后，就要开始处理其对应的初始化逻辑。主要是两类，GameFeatureAction和GaemFeaturesPlugin。除了Experience本身有配置这些，ActionSets中也有配置：
```c++
GameFeaturePluginURLs.Reset();

auto CollectGameFeaturePluginURLs = [This=this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
{
	for (const FString& PluginName : FeaturePluginList)
	{
		FString PluginURL;
		if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, /*out*/ PluginURL))
		{
			This->GameFeaturePluginURLs.AddUnique(PluginURL);
		}
		else
		{
			ensureMsgf(false, TEXT("OnExperienceLoadComplete failed to find plugin URL from PluginName %s for experience %s - fix data, ignoring for this run"), *PluginName, *Context->GetPrimaryAssetId().ToString());
		}
	}
};

CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);
for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
{
	if (ActionSet != nullptr)
	{
		CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
	}
}
```
搜集完需要激活的GameFeature后，就可以调用GaemFeaturesSubsystem的方法激活这些GameFeature：
```c++
NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();
if (NumGameFeaturePluginsLoading > 0)
{
	LoadState = ELyraExperienceLoadState::LoadingGameFeatures;
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		ULyraExperienceManager::NotifyOfPluginActivation(PluginURL);
		UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
	}
}
else
{
	OnExperienceFullLoadCompleted();
}
```
## OnExperienceFullLoadCompleted
所有GameFeature激活完后，需要把所有直接配置的Action激活：
```c++
auto ActivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
{
	for (UGameFeatureAction* Action : ActionList)
	{
		if (Action != nullptr)
		{
			//@TODO: The fact that these don't take a world are potentially problematic in client-server PIE
			// The current behavior matches systems like gameplay tags where loading and registering apply to the entire process,
			// but actually applying the results to actors is restricted to a specific world
			Action->OnGameFeatureRegistering();
			Action->OnGameFeatureLoading();
			Action->OnGameFeatureActivating(Context);
		}
	}
};

ActivateListOfActions(CurrentExperience->Actions);
for (const TObjectPtr<ULyraExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
{
	if (ActionSet != nullptr)
	{
		ActivateListOfActions(ActionSet->Actions);
	}
}
```
至此就完成了所有Experience的初始化操作，随后就可以调用OnExperienceLoaded的回调：
```c++
OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience);
OnExperienceLoaded_HighPriority.Clear();

OnExperienceLoaded.Broadcast(CurrentExperience);
OnExperienceLoaded.Clear();

OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience);
OnExperienceLoaded_LowPriority.Clear();
```
这里设置了三个Loaded回调，按顺序执行。当一些系统期望在另一些系统初始化之前执行操作时，就可以把自己注册为高优先级的回调，而另一些注册为低优先级的。、

在注册执行回调时，通常会根据当前的状态选择直接调用或者注册：
```c++
void ULyraExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_HighPriority(FOnLyraExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}
```
这样避免了注册的时序问题，统一了不同情况的操作，方便使用。

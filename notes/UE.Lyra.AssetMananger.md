---
id: m0982l7s6k4habx1sxccmdr
title: AssetMananger
desc: ''
updated: 1680828146214
created: 1680601476785
---

> Version: UE5.1.1

通常，我们会派生`UPrimaryDataAsset`定义一些`DataAsset`，里面会引用一些其它对象。一般有两种引用方式，即直接引用或者软引用：
```c++
UTexture2D* Texture; // 直接引用
TSoftObjectPtr<AActor> ActorClass; // 软引用
```
直接引用的对象会在加载这个DataAsset时直接加载进来：
```c++
ULyraAssetManager& AssetManager = ULyraAssetManager::Get();
FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId); // FPrimaryAssetId ExperienceId
TSubclassOf<ULyraExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad());
check(AssetClass);
const ULyraExperienceDefinition* Experience = GetDefault<ULyraExperienceDefinition>(AssetClass);
```
包括嵌套任何深度的直接引用，这和通常的UObject加载逻辑一样。对于软引用，这种情况下是不会加载的。

软引用的加载需要用UAssetManager的方法和Bundle，通常是通过方法`UAssetManager::LoadPrimaryAssets`加载，而软引用的属性上必须指明所属的Bundle，加载时指定要加载的Bundles，这样匹配的软引用的资源就会被加载上来：
```c++
// Define 
UPROPERTY(meta=(AssetBundles = "TestBundle"))
TSoftObjectPtr<AActor> ActorClass; // 软引用

// Load 
TSet<FPrimaryAssetId> BundleAssetList = {}; // Asset id to load 
TArray<FName> BundlesToLoad = { FName(TEXT("TestBundle")) };

AssetManager.LoadPrimaryAssets(BundleAssetList, RawAssetList); 
```
这样，`FPrimaryAssetId`所表示的DataAsset的所有软引用的属性，只要指明了Bundle为"TestBundle"的都会被加载出来。没有指明Bundle的软引用，默认实现下是不会加载的。不指明要加载的Bundle，也是任何软引用都不会加载。软引用属性中嵌套的软引用不会被加载。

其实现就是在保存一DataAsset时，会扫描当前DataAsset的所有属性中的元数据，并添加到AssetBundleData`(记录了BundleName -> TArray<FTopLevelAssetPath> 的Array)`中：
```c++
void UPrimaryDataAsset::UpdateAssetBundleData()
{
	// By default parse the metadata
	if (UAssetManager::IsValid())
	{
		AssetBundleData.Reset();
		UAssetManager::Get().InitializeAssetBundlesFromMetadata(this, AssetBundleData);
        // 扫描所有属性，直接引用的UObject也会递归地扫描它。
	}
}
```
Cook时AssetRegistry会把Bundle保存下来，打包出来的Game就可以直接通过AssetManager使用。`LoadPrimaryAssets()`就会查找PrimaryAsset对应的Bundles缓存，找到其对应的所有软引用资产并加载。
```c++
FAssetBundleEntry UAssetManager::GetAssetBundleEntry(const FPrimaryAssetId& BundleScope, FName BundleName) const
{
	if (const TSharedPtr<FAssetBundleData, ESPMode::ThreadSafe>* FoundMap = CachedAssetBundles.Find(BundleScope))
	{
		for (FAssetBundleEntry& Entry : (**FoundMap).Bundles)
		{
			if (Entry.BundleName == BundleName)
			{
				return Entry;
			}
		}
	}
	
	return FAssetBundleEntry();
}
```
一些情况下，我们希望通过其它的设置决定某个软引用属于什么Bundle。Lyra中，通过重载`UPrimaryDataAsset::UpdateAssetBundleData()`，向其中添加自定义的逻辑设置一些属性的Bundle。例如，`UGameFeatureAction_AddComponents`处理Client和Server Bundle:
```c++
// 引用了GameFeatureAction的DataAsset中需要实现：
#if WITH_EDITORONLY_DATA
void ULyraExperienceDefinition::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
}
#endif // WITH_EDITORONLY_DATA

// 具体的Action中实现自定义逻辑添加BundleAsset
#if WITH_EDITORONLY_DATA
void UGameFeatureAction_AddComponents::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
	if (UAssetManager::IsValid())
	{
		for (const FGameFeatureComponentEntry& Entry : ComponentList)
		{
			if (Entry.bClientComponent)
			{
				AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, Entry.ComponentClass.ToSoftObjectPath().GetAssetPath());
			}
			if (Entry.bServerComponent)
			{
				AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, Entry.ComponentClass.ToSoftObjectPath().GetAssetPath());
			}
		}
	}
}
#endif
```


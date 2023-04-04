---
id: m0982l7s6k4habx1sxccmdr
title: AssetMananger
desc: ''
updated: 1680601699830
created: 1680601476785
---

# UAssetManager::LoadPrimaryAssets
会将指定的FPrimaryAssetId加载到内存中，直到显式Unload。加载要指定Bundles，只有指定Bundle的Soft引用会被加载，嵌套的DataAsset不会执行Bundle的加载。
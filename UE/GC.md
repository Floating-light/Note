标记清除算法。

由`UWorld::Tick()`每帧调用`UEngine::ConditionalCollectGarbage()`，尝试GC，这里主要处理：
* 限制主流程一帧只能走一次
* 处理非Shipping下的控制台变量：
  * gc.StressTestGC
  * gc.ForceCollectGarbageEveryFrame
* 根据World的状态确定流程：
  * 
  * 
  * 
  * 处理gc.CollectGarbageEveryFrame N 每N帧执行一次GC
  * 处理gc.ContinuousIncrementalGC

对于每个对象，如何找到并记录成员的类型和地址？
首先回顾一下UE的反射系统。一个UObject类对应一个UClass，UClass中保存有这个UObject派生类型的所有反射信息。

`GetPrivateStaticClassBody()`创建UClass。



是否要记录所有成员的信息？



reference:

* https://zhuanlan.zhihu.com/p/219588301
* https://zhuanlan.zhihu.com/p/67055774
* https://zhuanlan.zhihu.com/p/402398260
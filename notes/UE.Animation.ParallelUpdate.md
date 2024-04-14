---
id: 0l0dv0z8mxr1karqb5y7gvf
title: ParallelUpdate
desc: ''
updated: 1713104220452
created: 1713086140565
---

通常，为了计算出最终的Pose，我们需要许多Gameplay数据驱动动画蓝图的计算，一个常用的办法是在动画蓝图的EventGraph中，重写"BlueprintUpdateAnimation", 在其中将需要用于驱动动画状态机计算的数据保存在动画蓝图中。其C++版本是"NativeUpdateAnimation()"， 这两个在主线程中USkeletalMeshComponent中先后调用：

![](/assets/images/UpdateAnimation.png)

随后我们就可以在AnimGraph中直接使用这些数据驱动Pose的计算。

如果我们开启了"bAllowMultiThreadedAnimationUpdate", AnimGraph中的节点都会在WorkerThread中执行，所以我们不能直接访问其他对象的属性，而必须事先把用到的数据保存下来，以确保线程安全。

> UAnimInstance::UpdateAnimation
>     //  USkeletalMeshComponent::HandleExistingParallelEvaluationTask
>     //  会等到并行的Task完成
>     FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
>     PreUpdateAnimation(DeltaSeconds); // 

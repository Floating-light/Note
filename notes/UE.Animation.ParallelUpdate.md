---
id: 0l0dv0z8mxr1karqb5y7gvf
title: UE5 多线程动画更新与Property Access system
desc: ''
updated: 1714123107166
created: 1713086140565
---

# 1. MultiThreadedAnimationUpdate

通常，为了计算出最终的Pose，我们需要许多Gameplay数据驱动动画蓝图的计算，一个常用的办法是在动画蓝图的EventGraph中，重写"BlueprintUpdateAnimation", 在其中将需要用于驱动动画状态机计算的数据保存在动画蓝图UAnimInstance自己的属性中，其C++版本是"NativeUpdateAnimation()"。它们在主线程中，由USkeletalMeshComponent::TickComponent调用UAnimInstance::UpdateAnimation触发更新：

![](/assets/images/UpdateAnimation.png)

```
UAnimInstance::UpdateAnimation
  // acquire the proxy as we need to update
  FAnimInstanceProxy& Proxy = GetProxyOnGameThread<FAnimInstanceProxy>();
    USkeletalMeshComponent::HandleExistingParallelEvaluationTask //  会等到并行的Task完成
  ... ...
  NativeUpdateAnimation(DeltaSeconds);
  BlueprintUpdateAnimation(DeltaSeconds);
```

在`UAnimInstance::UpdateAnimation`最前面会调用`GetProxyOnGameThread<FAnimInstanceProxy>()`，这个里面会处理正在异步更新AnimGraph的Task，等到它完成：

![](/assets/images/Animation_ParallelTask.png)

如果我们开启了"bAllowMultiThreadedAnimationUpdate", AnimGraph中的节点都会在WorkerThread中执行，而这些Node通常都会访问UAnimation中的属性。所以这里必须等到所有异步的Task完成，这样就确保了我们随后在`NativeUpdateAnimation`和`BlueprintUpdateAnimation`中更新UAnimInstance的属性是线程安全的。

# 2. Thread Safe Update Animation
前面提到，我们其实还是需要在主线程更新AnimGraph中需要用到的数据。但是，一些情况下，将会有大量的数据需要更新，且可能需要经过复杂的计算才能得到AnimGraph需要的数据，这一步就可能成为性能瓶颈。所以，我们需要将这一步也并行化。

在Work线程执行的`FAnimInstanceProxy::UpdateAnimation()`中，会调用UAnimInstance中的`NativeThreadSafeUpdateAnimation()`和`BlueprintThreadSafeUpdateAnimation()`这两个方法，它们与前面提到的在主线程中更新动画数据的方法相对应，这是它们的Work线程版本：

```
FAnimInstanceProxy::UpdateAnimation()
  FAnimInstanceProxy::UpdateAnimation_WithRoot
    GetAnimInstanceObject()->NativeThreadSafeUpdateAnimation()
    GetAnimInstanceObject()->BlueprintThreadSafeUpdateAnimation()
```

我们可以将复杂的计算逻辑放在这两个方法中，把更新动画需要的数据计算也并行化。为了保证线程安全，这两个ThreadSafe的方法中要访问和写的变量都必须是线程安全的，要确保在异步执行`FAnimInstanceProxy::UpdateAnimation()`时，GameThread不会修改这些变量。

所以，这里需要用到的所有GameThread的数据，例如Character，PlayerControler等其它对象的属性，需要在前面的GameThread版本的`UpdateAnimation`中保存下来，以供后面异步更新时使用。

在蓝图中，UE利用反射系统，实现了自动捕获在蓝图重载的`BlueprintThreadSafeUpdateAnimation()`函数中使用到的GameThread数据，只需要在访问别的数据时，使用`PropertyAccess`节点：

![](/assets/images/Animation_PropertyAccess.png)

所有在UAnimInstance的Context中能访问的函数和变量都能出现在这里，其中，函数必须有返回值，且必须是Pure函数。如果是蓝图定义的函数，返回值的名字必须是`ReturnValue`。

任何在这里通过PropertyAccess访问的属性或者方法，都会在GameThread中UpdateAnimation时将这些数据缓存到UAnimInstance中，随后在多线程执行BlueprintThreadSafeUpdateAnimation时，PropertyAccess Node会直接访问这些缓存的值，而不是实际指向的函数或变量，以保证线程安全。

![](/assets/images/Animation_PropertyAccessSubsystem.png)

# 3. AnimBlueprintExtension and FAnimSubsystem
这个`FAnimSubsystem_PropertyAccess`是如何Work的？

在创建每个AnimGraphNode时，会尝试向UAnimBlueprint注册各个Extension：

![](/assets/images/Anim_AddExtention.png)

其中就包含了`UAnimBlueprintExtension_PropertyAccess`。在创建UAnimBlueprint时，必定会创建一个`UAnimGraphNode_Root`节点，所以能保证这些Extension被添加。

随后在编译蓝图时，`FAnimBlueprintCompilerContext::ProcessExtensions()`会遍历所有Extensions，从`UAnimBlueprintExtension_PropertyAccess`中得到对应的`FAnimSubsystem`的类型，并创建对应FStructProperty*，并且将其放在另一个属性`NewAnimBlueprintConstants`中，这相当于向`NewAnimBlueprintConstants`这个Struct中动态添加了一个属性。

而`NewAnimBlueprintConstants`则是在编译蓝图时，为当前GeneretedClass创建的`SparseClassDataStruct`:

![](/assets/images/Anim_RecreateSparseClassData.png)

这个结构里面对应的数据是每个UClass一份，而不是每个Instance一份，在UClass中可以看到：

![](/assets/images/Anim_SparseClass.png)

因此，所有Extension对应的FAnimSubsystem都会存在于GenerateClass的SparseClassData中。

在UAnimBlueprintGeneratedClass的PostLoad中，会调用`UAnimBlueprintGeneratedClass::BuildConstantProperties()`，从`SparseClassDataStruct`将所有`FAnimSubsystem`保存下来，以方便后续通过GenerateClass上的`ForEachSubsystem`快速访问所有Subsystem。

![](/assets/images/Anim_BuildProperties.png)

在`ForEachSubsystem`中，就可以通过FProperty的`ContainerPtrToValuePtr`获取到对应的结构体实例：

![](/assets/images/Anim_ForEachSubsystem.png)

`FAnimSubsystem`定义了以下几个虚函数，分别在对应的线程和UpdateAnimation前后调用：

![](/assets/images/Anim_AnimSubsystem.png)

# 4. FAnimSubsystem_PropertyAccess

对于这里的`FAnimSubsystem_PropertyAccess`，他实现了在这几个不同的时间点进行属性缓存，而需要缓存的属性由`FPropertyAccessLibrary`在编译蓝图时记录下来，每一个不同的时机都记录了对应的需要缓存的属性，只有用到的属性会被缓存。

因为`FAnimSubsystem_PropertyAccess`是存在于UClass上的，所以缓存的属性不可能在由他自己保存，通过分析可以发现，`FPropertyAccessLibrary`只是记录了每个时机点需要Copy的属性，从哪一个属性Copy到哪一个属性，通过FProperty表示，还有相关的属性和函数的Path。`FPropertyAccessLibrary`上并没有实际保存赋值的属性的地方。

在`BlueprintThreadSafeUpdateAnimation()`中使用的`PropertyAccess`节点，实际对应于代码中的`UK2Node_PropertyAccess`，他实现了接口`IClassVariableCreator`，这个接口只有一个方法`CreateClassVariablesFromBlueprint()`，这会在编译蓝图前，重新生成`UAnimBlueprintGeneratedClass`时，遍历所有的Node，调用这个接口方法：

```c++
UK2Node_PropertyAccess::CreateClassVariablesFromBlueprint(InCreationContext)
  InCreationContext.CreateUniqueVariable(this, ResolvedPinType)
    	FProperty* NewProperty = FKismetCompilerUtilities::CreatePropertyOnScope(NewClass, VarName, VarType, NewClass, CPF_None, Schema, MessageLog);
```

`UK2Node_PropertyAccess`这里会给GenerateClass创建一个和它引用的属性一样类型的变量，即是给当前GenerateClass添加了一个属性，这个属性专用于缓存引用属性的值。

随后编译到这个节点时，获取到`UAnimBlueprintExtension_PropertyAccess`，调用它的`AddCopy`方法，指定Source属性Path，即这个节点引用的属性，和目标属性Path，即在`CreateClassVariablesFromBlueprint（）`中创建的属性。

```c++
UK2Node_PropertyAccess::ExpandNode()
  UAnimBlueprintExtension_PropertyAccess* PropertyAccessExtension = UAnimBlueprintExtension::FindExtension<UAnimBlueprintExtension_PropertyAccess>(AnimBlueprint);
  // ... ... 
  FPropertyAccessHandle Handle = PropertyAccessExtension->AddCopy(Path, DestPropertyPath, ContextId, this);
    PropertyAccessLibraryCompiler->AddCopy(InSourcePath, InDestPath, InContextId, InObject);
      // 将Copy信息保存在QueuedCopies中
```

搜集完所有Copy信息后，调用`UAnimBlueprintExtension_PropertyAccess::HandleFinishCompilingClass()`，
```c++
UAnimBlueprintExtension_PropertyAccess::HandleFinishCompilingClass()
  FPropertyAccessLibraryCompiler::FinishCompilation()
    // 这里会将所以记录到QueuedCopies的信息填充到`FPropertyAccessLibrary`中，而它是被`FAnimSubsystem_PropertyAccess`持有的。
    ::FPropertyAccessEditorSystem::CompileCopy(Class, OnDetermineBatchId, *Library, Copy);
```
所有PropertyAccess用到的属性都会有一条对应的Copy记录，表示应该从哪个Source属性Copy到Dst属性（由Node在GenerateClass上创建的）。在主线程更新动画时，就先调用`FAnimSubsystem_PropertyAccess`的更新方法，将所有属性Copy到UAnimInstance实例上。随后在Work线程真正访问这些属性时，就是访问的这些Copy目标属性。

# 5. Workflow
至此，整个PropertyAccess的Copy和同步流程就明了了：

![](/assets/images/Anim_MultThread.png)

这是蓝图使用`BlueprintThreadSafeUpdateAnimation()`和`PropertyAccessNode`的全部过程。

如果在c++中使用多线程的变量更新，需要重写对应的Native版本的方法：`NativeUpdateAnimation()`和`NativeThreadSafeUpdateAnimation()`。 在`NativeUpdateAnimation()`中，访问各种GameObject，将后续需要用到的属性保存在UAnimInstance中，这里最好只进行简单的变量赋值，不进行任何逻辑运算。在`NativeThreadSafeUpdateAnimation()`中，利用前面保存的变量值，进行复杂的逻辑计算，得到AnimGraph需要用到的数据。随后在AnimGraph中利用`NativeThreadSafeUpdateAnimation()`中得到的数据，计算Pose。
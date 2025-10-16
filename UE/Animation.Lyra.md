
## LeftHandPose_OverrideState
状态机计算出基本的Pose之后，第一个处理的是`LeftHandPose_OverrideState`。是用来覆写左手的Pose的。

在`ABP_ItemAnimLayersBase`中，它实现为：

![anim_lyra_lefthand_override](../assets/UE/anim_lyra_lefthand_override.png)

输入Pose与另一个Pose进行混合，`LeftHandPoseOverride`被设置为`ExplicitTime`，即每次这个节点初始化的时候都会读取`ExplicitTime`指定的时间点的`Pose`，即第0帧的Pose。

![anim_lyra_lh_read_pos](../assets/UE/anim_lyra_lh_read_pos.png)

`LayeredBlendPerBone`指定的[BlendMasks](https://dev.epicgames.com/documentation/en-us/unreal-engine/blend-masks-and-blend-profiles-in-unreal-engine)为`LeftFingersMask`，即指混合左手手指。同时混合之前会调用`SetLeftHandPoseOverride`更新混合权重，里面有个bool开关，决定要不要混合。

![anim_lyra_lh_blend](../assets/UE/anim_lyra_lh_blend.png)

大部分子动画蓝图的这个配置都是关了的，只有`ABP_ShotgunAnimLayers`是开的，他的父类是`ABP_RifleAnimLayers`，这两把枪的动作可以说是一模一样，除了左手握持的姿势有一点点区别，所以`ABP_ShotgunAnimLayers`就用这个功能，做一帧左手握持正确的动画，在这里混合进来，仅微调一下左手的姿势。

![lyraanim_lh_shotgun](../assets/UE/lyraanim_lh_shotgun.png)

## Upperbody/lowerbody split
有了基本的Localmotion基础Pose，这部分的功能是提供一些Montage播放的Slot，进行上下半身的姿势调整，

![animlyra_upper_lower_body](../assets/UE/animlyra_upper_lower_body.png)

1. `UpperBodyAdditive` Slot 播放的Montage，实现的是动态按权重`UpperbodyDynamicAdditiveWeight`叠加到Locomotion输出上。权重的更新`UpdateBlendWeightData()`实现的逻辑是：
* 玩家在地上，并且有montage在播的时候直接以1全量叠加。
* 如果不满足上面条件时，1渐渐插值到0，平滑地结束叠加，比如突然跳起来时。

    这里Slot的名字虽然叫`UpperBodyAdditive`，但它仍然是全身叠加的。这个Slot播放的动画必须是叠加动画，期望的是只有上半身的叠加动作，不然估计效果会不对。

2. `UpperBody` Slot 是不论这个Slot播的啥，最终都只会叠加上半身。因为后面的`Layered lend per bone`用的是`UpperBodyLowerBodySplitMask`，这个mask下半身都是0。虽然这里Blend Weight是写死的0，但是仍然会按Blend Mask里面对每个骨骼设置的Mask权重进行混合。

4. `FullBodyAdditivePreAim` 就是一个普通的Slot了，爱播啥就播啥，按Montage的逻辑处理。例如射击动作：`AM_MM_Pistol_Fire`。

5. 装弹动作的处理，装弹动作在`UpperBody`和`UpperBodyAdditive`都在播放，分别播放装弹动作的非叠加版本和叠加版本。这样做估计是想让换弹动作在叠加到Locomotion后，再和基础的换弹动作blend一下，使即使叠加了幅度很大的Locomotion，也让换弹动作看起来更平滑。

![animlyra_reload](../assets/UE/animlyra_reload.png)

## FullBody_Aiming

瞄准通常是基于idle状态的，Lyra中有四种Idle状态：没有武器的Idle、Idle Hipfire(腰射)、Idle ADS(Aim down sight开镜瞄准)、Crouch Idle，均要考虑瞄准。

![animlyra_aim_offset](../assets/UE/animlyra_aim_offset.png)

动画蓝图中，AimOffset只分了两种，RelaxedAimOffset和IdleAimOffset，RelaxedAimOffset在所有AimLinkedLayer的实现都是`AO_MM_Unarmed_Idle_Ready`，即没有武器的AO，只有头上下左右看。持有不同的武器，对应不同的AimLinkedLayer实现，有不同AO，`AO_MF_Pistol_Idle_ADS`，`AO_MM_Rifle_Idle_ADS`。在做AO中每个方向的动画时，[要满足这里提到的条件](Animation.MontageAndSlot.md#animation-additive-or-nonadditive)，BaseAnimation需要是对应的Idle状态。

这里为啥还要和没有武器的Idle瞄准Blend呢？而且AimOffsetBlendWeight的计算还挺复杂，HipFireUpperBodyOverrideWeight又是什么。

## AnimLinkedLayer - FullBodyAdditives

这是给AimLinkedLayer实现FullBodyAdditives接口，Layer中是按武器分不同的AimLinkedLayer的，这里是特定武器相关的动作叠加。

Layer给的一个实现例子是跳起来后的着陆动画:

![animlyra_lading_additives](../assets/UE/animlyra_lading_additives.png)

在这个后面还有一个`FullBody Slot`，可以用来播放全身Override的Montage，比如闪现，不用被瞄准等动作覆盖。

![animlyra_fullbody_inertialization](../assets/UE/animlyra_fullbody_inertialization.png)

而`Inertialization`，惯性混合，一些Blend节点、状态机的过渡条件可以配置过渡类型为`Inertialization`。传统混合在过渡期间会计算源姿势和目标姿势，然后合成混合姿势。

![animlyra_inertialia](../assets/UE/animlyra_inertialia.png)

而惯性混合不会再计算Source姿势，？

1. 记录切换瞬间的状态：当动画状态切换时（例如从“奔跑”切换到“行走”），系统会记录当前动画的骨骼位置、旋转及其变化速率（速度）。所以不会再计算SourcePose。
2. 应用惯性衰减：在新动画播放初期，系统会将记录的“速度”作为惯性力施加到新动画上，并随时间指数衰减（类似弹簧阻尼系统），使新旧动画的过渡更连续。
3. 通过数学公式（如阻尼振荡）计算过渡曲线，避免线性混合的突兀感。

## TurnInPlace

在玩家转动视角时，角色会先整个MeshComponent旋转到当前View，此时是滑步转过去的。所以这里用`RotateRootBone`转`RootYawOffset`回去，抵消这个滑步：

![animlyra_RotateRootBone](../assets/UE/animlyra_RotateRootBone.png)

然后用AimOffset，将`AimYaw`设为`-RootYawOffset`，使角色**上半身姿势**恢复到当前View方向。在`SetRootYawOffset()`中执行了`AimYaw`和`RootYawOffset`的计算。

然后每帧用全身的旋转动画，慢慢转回去。

# Reference

* https://www.jaydengames.com/posts/ue5-black-magic-game-core-animation/
* https://zhuanlan.zhihu.com/p/664971350
* https://zhuanlan.zhihu.com/p/654430436
* [Documentation Aim offset](https://dev.epicgames.com/documentation/en-us/unreal-engine/aim-offset-in-unreal-engine)
* [惯性化混合](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/blend-nodes?application_version=4.27#%E6%83%AF%E6%80%A7%E5%8C%96)
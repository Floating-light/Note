---
id: 2i18atlgt276hh2qe693yzc
title: Animation
desc: ''
updated: 1712329674929
created: 1712281493515
---

# 1. 状态机和动画别名

在Animation Graphics中，有个Output Pose节点, 输出最终的姿态.

locomotion 通常指角色或物体在空间中的移动. 走、跑。表示动画的基础步态周期，这些周期可以被循环使用模拟持续行走或跑动，这是进行后续角色动画处理的基本输出。所以状态机中通常有一个Locomotion状态机输出一个缓存Pose, 表示当前帧根据基本移动状态得到的基础Pose，在后续的计算中反复使用。

这里通常是Idle --> Walk/Run， 后者可由Speed在一维混合空间插值得出。



# 2. 重定向人体模型资产
将旧骨骼的动画Sequence重定向到新的骨骼，生成新的AnimSequence。
AnimSequence界面中，有Skeleton Tree可以显示额外的重定向选项，以修正部分骨骼。

# 3. 使用导入的动画

在Animation Graphics的状态机中，任何一个节点都需要一个输出Pose，直接将AnimSequence拖到State Graphic中即可生成一个Sequence节点，并连接到输出Pose，表示这个State的输出。

使用从不同的骨骼重定向过来的AnimSequence，有可能导致IK失效，因为旧的的AnimSequence认为IK骨骼的位置与当前骨骼的不同。IK通常在最终的Output Pose前计算，Control Rig。

如果新旧动画计算IK的脚部骨骼的位置不同，则会导致不正确的结果。所以在Control Rig计算前，要想办法让用于计算ik的骨骼ik_foot_l,ik_foot_r的位置，是当前骨骼上正确的位置，这个正确的位置，通常就是当前骨骼的foot_l,foot_r的位置：
![](/assets/images/controlrig_ik.png)

# 4. 混合空间和分析工具

1D BlendSpace，一维的一个变量，混合少量的动画（3个），根据速度混合持枪站立和持枪跑和持枪冲刺。中间如果有多个过渡AnimSequence,混合就会越平滑。

滑步，写一个调试按键，SetGlobalTimeDilation(0.2), 放慢时间，观察滑步。

混合空间分析，自动确定Axis的最大值：
![](/assets/images/AxisAnalysis.png)
随后在CharacterMovement中也设置同样的最大值即可解决滑步问题。

此时动画仍然会有个地方有突变的问题，这是冲刺动画的起始帧和结束帧的姿势不同导致的，在正常速度播放下看不出来。

# 5. 剪切和替换动画

现有的动画状态机中，跳被分为了三个动画，跳跃，下落，着陆。但是初学者包中的动画确实一个完整的跳跃动画。所以需要利用UE5中的工具(Edit in Sequence)将它分成这三个动画。

* Manny_jump_start 冲站立姿势到空中准备下落
* Manny_jump_loop 占位，暂时截一帧， Rate Scale 设为0，这个只有一帧，不应该动
* Manny_jump_land 24 - 最后一帧

由于新创建的land动画不是叠加动画，而是一个完整的动画，如果和之前一样叠加上Locomotion的输出，就会膨胀到非常大，所以这里直接去掉Locomotion的叠加。

# 6. 角色蹲伏功能-逻辑与动画

输入控制，胶囊体大小变化，charactermovement的设置。

动画部分：用一个bool值确定角色是否处于蹲下状态，而且需要从Character对象中更新这个值到AnimInstance中。角色可以从任何移动状态和Land状态过渡到蹲下状态(创建StateAlias)，处于蹲下时，可以蹲着不动，也可以蹲着移动，这里就有两种动画，所以Crouch可以是一个子状态机。子状态机有两个状态，由速度决定是静止还是移动。
![](/assets/images/Anim_Crouch.png)

7. 使用插槽给角色添加武器

两者都可以做到：
Sokets 
* 一个骨骼可以有任意多个Sockets
* 不能在Runtime修改它的位置，但是可以修改它Attach的骨骼的位置
Virtual Bones 
* 一个Bone只能有一个虚拟骨骼。
* 它可以在动画蓝图中使用，作为IK Target，或者注视Target，还可以修改位置。

我们用Sokets，直接在对应骨骼hand_r上创建。

武器也是一个Actor（子Actor），也有SkeletalMesh。将它Attach到Character的Mesh下，就可以选择ParentSokets

随后，在角色SkeletonMesh中，可以在WeaponSoket上选择一个预览的SkeltonMesh枪，然后选择预览动画为Idle。条则WeaponSoket的位置，以确保枪的姿态正确。

# 10. 切换不同的武器附着点

武器可以背在背上，在背上也创建一个Socket，调整这个Soket的位置，使枪放上去看起来刚刚好。

调用武器的AttachActorToComponent，设定对应的Soket名字，以把武器Attach到对应的地方。

# 11. 使用Montage过渡武器切换

持拿武器和解除武器状态下的idle和walk/run状态的动画应该是不同的，至少手应该是持拿或放松的状态。这可以在Locomation StateMachine中，通过判断是否处于收起武器状态，选择使用装备武器状态下的idel / walk/run动画，或者解除武器下的idle / walk/run动画。

从对应的收枪Animation创建Montage。在播放Montage时，有时希望只要一个Animation上的部分动作，比如上半身或全身。

在奔跑时，我们希望只要上半身的收枪动画，下半身要继续奔跑。
静止时，希望要全身的收枪动画。

但是播一个Montage时，又不想太复杂，还需要考虑是否静止，所以，一个Montage可以准备两种播放模式，即不同的Slot，可以有FullBodySlot，也有UpperBodySlot，为这两个Slot都准备好动画，由动画蓝图根据当前是否奔跑决定用哪一个slot的Montage。

在Montage中创建FullBody和UpperBody Group，设置对应的动画，这里都是一样的：
![](/assets/images/Montage_FullUpper.png)

在动画蓝图中，可以使用Slot节点，引用对应的Montage模式，Slot节点输入由状态机计算好的Pose，在对应Slot有Montage时，用Montage覆盖，没有则用输入的Source。

不同的Slot，定义了该取用动画的哪一部分。

FullBody则很好处理，直接使用MainStateCache输入到对应Slot即可：
![](/assets/images/FullBodyCache.png)

UpperBody则在读取了对应UpperBodySlot的Montage输入后，还需要将上半身混合到正常的MainStateCache：
![](/assets/images/UpperBodyCache.png)

最后根据速度，使用BlendPosesByBool。

为了在恰当的时机改变枪Attach的位置，我们需要为Montage创建Notify事件，在动画蓝图中，得到事件的通知，然后调用相应的Character的逻辑处理Attach位置的交换。

![](/assets/images/Montage_Notify.png)

这里两个Montage几乎是一样的，只有Notiy的时机和事件名字不同。利用不同的Montage很好地区分了两个不同地行为，代码不用关注过多的细节。

# 14. 编辑动画
上面的收枪中，最终Montage收枪的位置与我们实际希望的位置不一样。

首先调整后背Socket的位置和旋转，使其处于理想位置。

用Edit In Sequence，编辑动画，使其把枪放在背上时，与背上的枪的最终位置一致。

![](/assets/images/EditInSequence.png)

## reference 
状态机动画
* 角色当前状态直接决定动画状态（播什么动画）
* 玩家输入决定动画的切换
* 随着动画状态的增加，切换过渡也会急剧膨胀，动画状态机的复杂度上升，难以维护
 
Motion matching

key: 角色当前的状态：速度、动画姿势(pose)，轨迹可以决定动画帧。

对角色运动状态进行特征提取：
$$
e_c=(velocity,skeletonpose,trajectiory)
$$

$$
e_c=(v_i,p_i,t_i)
$$

对动画序列帧进行预处理，提取ei
* https://zhuanlan.zhihu.com/p/468756512
* https://zhuanlan.zhihu.com/p/586248403 

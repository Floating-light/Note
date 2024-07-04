# 左手系 or 右手系
本质是坐标轴的定义和方向的约定。影响图形变换，例如旋转矩阵的构建。

左手系
* X 轴：右
* Y 轴：上
* Z 轴：朝屏幕里（远离观察者）
左手叉乘。

右手系
* X 轴：右
* Y 轴：上
* Z 轴：朝屏幕外（朝向观察者）
右手叉乘。

绕某个轴的旋转，实际旋转方向左手系和右手系是相反的。

例如绕Z轴旋转30度:
* 左手系：逆时针30度
$$
R_z(\theta)=
\begin{pmatrix}
\cos(\theta) & \sin(\theta) & 0 & 0 \\
-\sin(\theta) & \cos(\theta) & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$

* 右手系：顺时针30度
$$
R_z(\theta)=
\begin{pmatrix}
\cos(\theta) & -\sin(\theta) & 0 & 0 \\
\sin(\theta) & \cos(\theta) & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$

在一个Application中，只需要确定使用哪一种坐标系，不要混用即可。

# View Transform
```
The only difference you will ever see from using a right-handed vs a left-handed coordinate system is the math behind how you construct your projection and camera matrices.
```
## 为什么看向-Z(右手坐标系)
> 左手坐标系下看向+Z

1. 简化投影Projection变换
2. View坐标系一致，
   * +X方向是屏幕右侧
   * +Y是屏幕上方
   * -Z是屏幕里面
3. 在右手坐标系中，观察方向通常是从世界空间的原点看向负Z方向。
4. 在左手坐标系中，观察方向通常是从世界空间的原点看向正Z方向。
5. 简化几何变换
固定观察方向使得几何变换（如物体变换、视口变换等）更直观和统一。在图形管线中，从模型坐标系到视图坐标系再到屏幕坐标系的变换可以更系统化地处理。

# row major 与 column major
https://stackoverflow.com/questions/16578765/hlsl-mul-variables-clarification

https://stackoverflow.com/questions/27390049/graphics-row-vs-column-major-transformations

https://seanmiddleditch.github.io/matrices-handedness-pre-and-post-multiplication-row-vs-column-major-and-notations/

定义的是矩阵在内存中的存储方式。假设有一个矩阵从数学上看是这样的：
```
[ a b c d ]
[ e f g h ]
[ i j k l ]
[ m n o p ]
```

如果以Row major存储，在内存中应该是这样的：
```
a b c d e f g h i j k l m n o p
```

如果以Column major存储，在内存中应该是这样的：
```
a e i m b f j n c g k o d h l p
```

HLSL读取输入的Matrix：

https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math#matrix-ordering

DirectX has historically used row-major matrices, row vectors, pre-multiplication, and left-handed coordinates. 

DirectXMath矩阵约定：

https://learn.microsoft.com/zh-cn/windows/win32/dxmath/pg-xnamath-getting-started

DirectXMath中对Matrix有以下约定：
* row-major
* row vector
* pre-multiplication
  * 这就要求matrix必须是Post-Multiplication情况下矩阵的转置

HLSL中读取传给CBV中的矩阵：

* 默认以column_major读取，读到constant registers后，`matrix order`不影响任何后续在Shader代码中的使用。

* row-major or column-major不影响Shader代码中构建的Matrix，总是row-major。
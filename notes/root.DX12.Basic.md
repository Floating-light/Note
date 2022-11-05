---
id: vlsuz7v50xf4mp2sui4xofo
title: Basic
desc: ''
updated: 1667524447192
created: 1667017115678
---

# Com接口

> https://learn.microsoft.com/en-us/windows/win32/com/com-technical-overview

COM(Component object model)本质上定义了一种标准。这种标准是和语言、平台无关的，许多编程语言都可以创建和使用COM对象。类似于C++标准和不同的编译器实现的关系。

COM对编程语言唯一的要求是：`可以创建指针结构，并通过指针调用函数`。C++天然支持这一点，所以它本身的语言特性（指针，继承，多态）使得实现和使用COM对象十分简单和自然。

所有COM接口都派生自IUnknow:
```c++
IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [annotation][iid_is][out] */ 
        _COM_Outptr_  void **ppvObject) = 0;
        
    virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
    virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
}
```
它主要提供多态和生命周期的管理。

其中，成员函数`QueryInterface`提供了多态性。由于COM中每个接口都有唯一的ID(GUID,在定义的时候就确定了)，所以传入一个接口ID就可以知道当前对象是否支持这个接口，如果可以就返回它的指针。因为这里只用一个GUID标识一个接口，就可以在旧版本的对象上查询新的没支持的接口。

COM 对象的生命周期由引用计数控制。`AddRef`增加计数，`Release`减少计数，当计数减少到0时，`Release`就会释放这个实例。通常不显式地自己调用这两个函数，而是使用类似于智能指针的`ComPtr<T>`，自动管理其生命周期，也可以自己实现这样的引用计数器。

DXGI,DirectX都是用COM实现的。

# DXGI(DirectX Graphics Infrastructure)

> https://learn.microsoft.com/en-us/windows/win32/api/dxgi/

主要处理枚举Graphics adapters、Display modes、显示渲染的图像到显示器。DirectX的各个版本都会用到它。

# DirectX12

> https://learn.microsoft.com/en-us/windows/win32/direct3d12/interface-hierarchy

DirectX提供的功能完全是基于COM实现的。


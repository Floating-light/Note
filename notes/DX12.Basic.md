---
id: vlsuz7v50xf4mp2sui4xofo
title: Basic
desc: ''
updated: 1667954953088
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

COM 对象的生命周期由引用计数控制。`AddRef`增加计数，`Release`减少计数，当计数减少到0时，`Release`就会释放这个实例。通常不显式地自己调用这两个函数，而是使用类似于智能指针的`ComPtr<T>`，自动管理其生命周期，也可以自己实现这样的引用计数器。COM对象在创建时，会调用一次`AddRef`，我们拿到COM对象后如果不需要了，就要显式调用一次`Release`。
> https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice

DXGI,DirectX都是用COM实现的。

# DXGI(DirectX Graphics Infrastructure)

> https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi

主要处理更低层级的任务，枚举Graphics adapters、Display modes、显示渲染的图像到显示器。这些是独立于DirectX的。

IDXGIFactory用于创建DXGI相关的对象。而它本身可以通过一个全局函数直接创建：
```c++
ComPtr<IDXGIFactory2> factory;
CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
```

## Adapter

Adapter是对渲染硬件（显卡）和软件（Direct3D软光栅化器）的抽象。一台主机通常有多个Adapter，独立显卡、集成显卡、软光栅化器。

可以通过枚举出所有Adapter，然后查询它是否支持我们需要的Direct3D API版本:
```c++
ComPtr<IDXGIAdapter1> adapter;
for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
{
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
    {
        // Don't select the Basic Render Driver adapter.
        continue;
    }

    // Check to see whether the adapter supports Direct3D 12, but don't create the
    // actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
    {
        break;
    }
}
```
新版本的Factory支持设定GPU preference枚举Adapter：
```c++
ComPtr<IDXGIAdapter1> adapter;
ComPtr<IDXGIFactory6> factory6;
if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
{
    for (
        UINT adapterIndex = 0;
        SUCCEEDED(factory6->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)));
        ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see whether the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }
}
```
## Presentation
图形应用会渲染出一帧图像，要求DXGI将它显示到显示设备上。通常至少会有两个Buffer，一个给DXGI显示（front buffer），另一个渲染用于渲染下一帧（back buffer）。创建和管理这两个Buffer的是SwapChain。一些应用取决于渲染一帧所花的时间或期望帧率肯能需要更多的Buffer。

![](/assets/images/Buffer.png)

Swap chain中的这些Buffer会被创建在显存中，然后通过DAC，把front buffer显示到最终的显示设备上。

![](/assets/images/DisplaySystem.png)

Swap chain在创建时必须与一个Window和Device（D3D12是CommandQueue）绑定，Device是Direct3D对Adapter的抽象，需要通过Adapter创建。当Device改变时，Swap chain也必须重新创建。Swap chain的Buffer以指定大小和格式创建，也可以随时修改。渲染时，从Swap chain取出Buffer，创建RenderTarget，即可通过Direct3D向其中渲染图像。

如果调用了`IDXGIFactory::MakeWindowAssociation`，用户可以通过Alt+Enter在全屏和窗口模式之间切换。

有关SwapChain的更多信息：

* https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model?redirectedfrom=MSDN
* https://www.intel.com/content/www/us/en/developer/articles/code-sample/sample-application-for-direct3d-12-flip-model-swap-chains.html

### ResizeBuffer
### HandleWindowResizing


# DirectX12

> https://learn.microsoft.com/en-us/windows/win32/direct3d12/interface-hierarchy
![](/assets/images/Direct3D12.png)

DirectX提供的功能完全是基于COM实现的。


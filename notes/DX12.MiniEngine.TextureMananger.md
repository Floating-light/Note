---
id: w1744cxqw4qx6l1njhujeew
title: TextureMananger
desc: ''
updated: 1668688851151
created: 1668597462467
---

![ads](notes/assets/images/TextureManager.png)

![asd](notes/assets/images/WP_RuntimePartition.jpg)

## GpuResource

*GpuResource*表示各种GPUResource，对应于D3D12的*ID3D12Resource*，可以表示各种Buffer。这一层直接持有了一个*ComPtr<<ID3D12Resource>ID3D12Resource>*。

## Texture

*Texture*则表示一个TextureResource，提供基础的创建方法，Texture的基本信息，宽，高，depth。还有它对应的SRV。

## ManangedTexture

这一层支持以引用计数的方式被管理，可以在没有被引用时自动销毁。这个对象不直接暴露给Client，而是用一个包装好的Reference。

```c++
class ManagedTexture : public Texture
{
    friend class TextureRef;

public:
    ManagedTexture( const wstring& FileName );

    // 支持多线程访问时，有可能需要等待别的线程创建好
    void WaitForLoad(void) const;
    void CreateFromMemory(ByteArray memory, eDefaultTexture fallback, bool sRGB);

private:

    bool IsValid(void) const { return m_IsValid; }
    void Unload(); // 引用计数为0后调用 

    // 相对路径，唯一标识这个Texture
    std::wstring m_MapKey;		// For deleting from the map later
    bool m_IsValid;
    bool m_IsLoading;
    size_t m_ReferenceCount;
};
```

## TextureRef

相当于是*ManagedTexture*的Handle，构造函数和析构函数会修改*ManagedTexture*的引用计数，计数为0时，将会调用*ManagedTexture::Unload()*，进而调用*TextureManager::DestroyTexture()*，从Manage的列表中移除，从而释放这个Texture。这里似乎*DESCRIPTOR_HANDLE*并没有从全局Allocator回收利用，而是直接不管了?

```C++
class TextureRef
{
public:

    TextureRef( const TextureRef& ref );
    TextureRef( ManagedTexture* tex = nullptr );
    ~TextureRef();

    void operator= (std::nullptr_t);
    void operator= (TextureRef& rhs);

    // Check that this points to a valid texture (which loaded successfully)
    bool IsValid() const;

    // Gets the SRV descriptor handle.  If the reference is invalid,
    // returns a valid descriptor handle (specified by the fallback)
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;

    // Get the texture pointer.  Client is responsible to not dereference
    // null pointers.
    const Texture* Get( void ) const;

    const Texture* operator->( void ) const;

private:
    ManagedTexture* m_ref;
};
```

## TextureMananger

所有ManangedTexture的管理器。TextureManager将所有Texture集中在一起，管理它们的创建和销毁。通过一个路径先从已创建的列表中查找是否已经创建，没有则加载文件，并创建Texture，这里会创建DeafultHeap上的Texture资源，通过UploadHeap把数据传上去，并等待命令执行完毕。

释放资源时，直接通过路径找到这个MangedTexture，直接释放即可。

```c++
namespace TextureManager
{
    wstring s_RootPath = L"";
    map<wstring, std::unique_ptr<ManagedTexture>> s_TextureCache;

    void Initialize( const wstring& TextureLibRoot )
    {
        s_RootPath = TextureLibRoot;
    }

    void Shutdown( void )
    {
        s_TextureCache.clear();
    }

    mutex s_Mutex;

    ManagedTexture* FindOrLoadTexture( const wstring& fileName, eDefaultTexture fallback, bool forceSRGB )
    {
        ManagedTexture* tex = nullptr;

        {
            lock_guard<mutex> Guard(s_Mutex);

            wstring key = fileName;
            if (forceSRGB)
                key += L"_sRGB";

            // Search for an existing managed texture
            auto iter = s_TextureCache.find(key);
            if (iter != s_TextureCache.end())
            {
                // If a texture was already created make sure it has finished loading before
                // returning a point to it.
                tex = iter->second.get();
                tex->WaitForLoad();
                return tex;
            }
            else
            {
                // If it's not found, create a new managed texture and start loading it
                tex = new ManagedTexture(key);
                s_TextureCache[key].reset(tex);
            }
        }

        Utility::ByteArray ba = Utility::ReadFileSync( s_RootPath + fileName );
        tex->CreateFromMemory(ba, fallback, forceSRGB);

        // This was the first time it was requested, so indicate that the caller must read the file
        return tex;
    }

    void DestroyTexture(const wstring& key)
    {
        lock_guard<mutex> Guard(s_Mutex);

        auto iter = s_TextureCache.find(key);
        if (iter != s_TextureCache.end())
            s_TextureCache.erase(iter);
    }

} // namespace TextureManager
```
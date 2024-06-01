---
id: lgq4zrk3klheefzxfkk57zq
title: DefineInHLSL
desc: ''
updated: 1669771515541
created: 1669722484328
---

# 反序列化RootSignature

如果已经有了序列化的RootSIgnature或者编译好的包含RootSignature的Shader，可以调用*D3D12CreateVersionedRootSignatureDeserializer*，获得一个*ID3D12VersionedRootSignatureDeserializer*接口，再调用*ID3D12VersionedRootSignatureDeserializer::GetRootSignatureDescAtVersion*可以得到反序列化的D3D12_ROOT_SIGNATURE_DESC1。

# An example HLSL Root Signature

Root Signature可以在HLSL中以字符串的形式指定，以逗号分隔，描述了RootSignature中的各个组件。

https://learn.microsoft.com/en-us/windows/win32/direct3d12/specifying-root-signatures-in-hlsl
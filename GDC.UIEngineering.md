---
id: v4hwklnxc6gzuhfip5dk1g2
title: UIEngineering
desc: ''
updated: 1682243437328
created: 1682242327212
---

# UI Engineering Patterns from 'Marvel's Midnight Suns'

https://www.gdcvault.com/play/1028880/UI-Engineering-Patterns-from-Marvel

* FrontEnd 存在于游戏核心循环之外的UI
* Control User可以与之交互的Visual
* Screen UI system的Base

## 如何获取数据
通过一个GlobalDatabase类，使用Identifiers获取指定数据:
![](assets/images/2023-04-23-17-36-21.png)

## The Challenge
* Scale of UI Development
* 150 screens, 90% of them in the frontend

需要一个可扩展的架构

## UICS - User Interface Component System

A UI Component is something that provides functionality to a UI screen.

![](assets/images/2023-04-23-17-45-19.png)

Example：Inventory Screen
![](assets/images/2023-04-23-17-47-32.png)

Editor View：
![](assets/images/2023-04-23-17-50-22.png)


https://www.gdcvault.com/play/1029143/-God-of-War-Ragnarok

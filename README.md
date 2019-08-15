# Agora Android Voice Tutorial - 1to1

*Read this in other languages: [English](README.en.md)*

这个开源示例项目演示了如何快速集成 Agora 音频 SDK，实现1对1音频通话。

在这个示例项目中包含了以下功能：

- 加入通话和离开通话；
- 静音和解除静音；
- 切换扬声器和听筒；

你可以在这里查看进阶版的示例项目：[OpenVoiceCall-Android](https://github.com/AgoraIO/OpenVoiceCall-Android)

你也可以在这里查看 iOS 平台的示例项目：

- [Agora-iOS-Voice-Tutorial-Swift-1to1](https://github.com/AgoraIO/Agora-iOS-Voice-Tutorial-Swift-1to1)

## 运行示例程序
首先在 [Agora.io 注册](https://dashboard.agora.io/cn/signup/) 注册账号，并创建自己的测试项目，获取到 AppID。将 AppID 填写进 "app/src/main/res/values/strings.xml"

```
<string name="agora_app_id"><#YOUR APP ID#></string>
```


其次，在 [Agora.io 下载](https://www.agora.io/cn/download/)语音通话SDK ，并按照libs.png放置在工程下。


最后用 Android Studio 打开该项目，连上设备，编译并运行。

也可以使用 `Gradle` 直接编译运行。

## 运行环境
- Android Studio 2.0 +
- 真实 Android 设备 (Nexus 5X 或者其它设备)
- 部分模拟器会存在功能缺失或者性能问题，所以推荐使用真机

## 联系我们
- 完整的 API 文档见 [文档中心](https://docs.agora.io/cn/)
- 如果在集成中遇到问题, 你可以到 [开发者社区](https://dev.agora.io/cn/) 提问
- 如果有售前咨询问题, 可以拨打 400 632 6626，或加入官方Q群 12742516 提问
- 如果需要售后技术支持, 你可以在 [Agora Dashboard](https://dashboard.agora.io) 提交工单
- 如果发现了示例代码的 bug, 欢迎提交 [issue](https://github.com/AgoraIO/Agora-Android-Voice-Tutorial-1to1/issues)

## 代码许可
The MIT License (MIT).

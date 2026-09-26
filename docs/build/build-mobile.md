## Build Instructions for Mobile (iOS / Android)

-----------------------------------
<br />


[How to build Android](#Android)

[How to build iOS](#iOS)

Please see param reference to learn about some additional options
[Build Params Reference](build-params-reference.md)

<br />

### `Prerequisites`

Besides the prerequisites needed by the desktop version, some mobile specific items are needed.
You will need a version of OpenSSL that is compatible with not only your device but also the arch that you are building.
It is up to you how to get these, but precompiled versions can be found here

[Openssl bins from TeskaLabs](https://teskalabs.com/blog/openssl-binary-distribution-for-developers-static-library)

By default we have them referenced in a UNIX style folder path `/usr/local/Cellar/openssl@DEVICE` where DEVICE is either `iphones` or `android`. You may need to change this in the makefile for your environment.

Depending on your device set up your development environment. This includes downloading xcode and iOS SDKS, or android studio with SDKs and NDKs

<br />

### `Android`


```sh
make -f makefile.cli -j 8 ANDROID=1 buildall
```

Running this command will build an `x86_64` and `aarch64` library and package together a folder which will contain everything needed to integrate into an Android project

4 params may need to be set to build correctly. `ANDROID_API`,`SDK_LOCATION`,`NDK_LOCATION`,and `HOST_ARCH`. `ANDROID_API` and `HOST_ARCH` may be left alone if they align with your environment. However, `SDK_LOCATION` and `NDK_LOCATION` will need to be set to where you have those applications installed. Both of these params need to be set to the version folder. For best results use the NDK that is installed with the SDK you downloaded.

If you do not want to build both Device and Simulator remove the `buildall` keyword and replace it with `ANDROID_ARCH="foo_bar` and supply the arch you want to build, `x86_64` or `aarch64`.

Unlike iOS, Android is not built statically and must be baked into your project. See CMAKE examples on android or our own nexus-mobile repo for an example.

<br />

### `iOS`



```sh
make -f makefile.cli -j 8 IPHONE=1 buildall
```

Running this command will build a `Simulator` and `Device` version of Nexus and package together a .framework directory that can be integrated into a Xcode project.

This framework will contain all libs and all headers needed.

if you wish to not build this framework and build one arch individually you must use these commands

`IOS_ARCH="foo" IOS_TYPE="bar"`

For ARCH `x86_64` or `arm64` are options, TYPE is either `ios-simulator` or `iphoneos` are options.

If you change any of these above values you **must remove the `buildall` keyword**

This makefile will default to iOS version 13.2 but you may need to change this depending on what sdk you downloaded. To do that set `IOS_SDK_VER` to the SDK number. ie `IOS_SDK_VER="13.2"`.

Nexus only supports x64 so you must disable support for armv7 and armv7a in the project's build settings.

After including the framework into your project you will need to write an entry point c++ file and convert the main objective c file of the app to objective-cpp. This Obj-cpp file will be the interface between the iOS layer and Nexus. See the nexus-mobile repo additional help.

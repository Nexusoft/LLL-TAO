## Optional Parameters for Compiling With Make
-----------------------------------
<br />

#### `FILE`

Sets the makefile param for Make.

Example:

```
-f makefile.cli
```

#### `JOBS`

Sets the number of jobs make will use to compile the core. Should be set to the number of useable cores of your cpu.
This param is apart of Make

Example:

```
-j 8
```

#### `32BIT`

Set to build Nexus as an 32bit application, you must also have the appropriate versions of Nexus dependencies

Example:

```
32BIT=1
```

#### `ARM64`

Set to build for ARM64 arch type.

Example:

```
ARM64=1
```

#### `OPENSSL LIBRARY PATH`

Set path to installed OpenSSL Library folder.

Example:

```
OPENSSL_LIB_PATH="\usr\foo\bar\openssl\lib"
```

#### `OPENSSL INCLUDE PATH`

Set path to installed OpenSSL Include folder.

Example:

```
OPENSSL_INCLUDE_PATH="\usr\foo\bar\openssl\include"
```

#### `BERKLEY DATABASE LIBRARY PATH`

Set path to installed Berkley Library folder.

Example:

```
OPENSSL_LIB_PATH="\usr\foo\bar\bdb\lib"
```

#### `BERKLEY DATABASE INCLUDE PATH`

Set path to installed Berkley Database Include folder.

Example:

```
OPENSSL_INCLUDE_PATH="\usr\foo\bar\bdb\include"
```

#### `MINIUPNPC LIBRARY PATH`

Set path to installed Mini UPNP C Library folder.

Example:

```
OPENSSL_LIB_PATH="\usr\foo\bar\miniupnpc\lib"
```

#### `ORACLE`

TBD

#### `USE_UPNP`

TBD

#### `WARNINGS_FAIL`

TBD

#### `RACE_CHECK`

TBD

#### `LEAK_CHECK`

TBD

#### `ADDRESS_CHECK`

TBD

#### `UNDEFINED_CHECK`

TBD

#### `ENABLE_DEBUG`

TBD

#### `ENABLE_WARNINGS`

TBD

#### `UNIT_TESTS`

TBD

#### `BENCHMARKS`

TBD

#### `LIVE_TESTS`

TBD

#### `NO_ANSI`

TBD

#### `STATIC`

Build Nexus core using Static c/c++ libraries. Useful to make a core that is independent of the host's environment

Example:

```
STATIC=1
```

#### `NO WALLET`

Do not include the legacy Nexus that used Databases

Example:

```
NO_WALLET=1
```

#### `IPHONE`

Used to run the iPhone portion of the makefile.

Example:

```
IPHONE=1
```

#### `IPHONE MINIMUM VERSION`

Sets the min iOS version needed to run Nexus. This can not go lower than 10 and you will lose support for some c++ methods.

Default: `10`

Example:

```
IPHONE IOS_MIN_VER="10"
```

#### `ANDROID`

Used to run the Android portion of the makefile.

Example:

```
ANDROID=1
```

#### `ANDROID ARCH`

Set the arch that the core will be built to for Android
Acceptable archs are: `x86_64` and `aarch64`
note that aarch64 arch is used for NDK and is equivalent to `arm64_v8a`

Example:

```
ANDROID=1 ANDROID_ARCH="x84_64"
```

#### `ANDROID API`

Set the Android API that NDK will use to compile the Android libraries

Default: `29`

Example:

```
ANDROID=1 ANDROID=29
```

#### `ANDROID SDK LOCATION`

Location to the installed Android SDK. Since multiple SDKs can be installed, this path should be to the specific SDK version.

Default: `/usr/local/Caskroom/android-sdk/4333796/`

Example:

```
ANDROID=1 SDK_LOCATION="/usr/local/Caskroom/android-sdk/4333796/"
```

#### `ANDROID NDK LOCATION`

Location to the installed Android NDK. Since multiple NDKs can be installed, this path should be to the specific NDK version.

Default: `/usr/local/Caskroom/android-sdk/4333796/ndk/21.3.6528147/`

Example:

```
ANDROID NDK_LOCATION="/usr/local/Caskroom/android-sdk/4333796/ndk/21.3.6528147/"
```

#### `ANDROID HOST ARCH`

The arch for the host computer, ie the one performing the build.

Default: `darwin-x86_64`

Example:

```
ANDROID=1 HOST_ARCH="darwin-x86_64"
```

#### `ANDROID TARGET`

When building the Nexus Core for Android you must specifically the target. By default it will use the ARCH and API to build this.

Default: `$(ANDROID_ARCH)-linux-android$(ANDROID_API)`

Example:

```
ANDROID=1 ANDROID_TARGET="x86_64-linux-android29"
```

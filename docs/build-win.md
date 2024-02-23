## Build Instructions for Windows

-----------------------------------
<br />
REQUIREMENTS - 	64bit CPU and OS.
				At least 6GB free Hard drive space.
				Stable internet connection.

If you have already completed `Part 1: Compiling the Dependencies`, please skip to `Part 2: Compiling Nexus LLL-TAO`. 

<br />


### `Part 1: Compiling the Dependencies`
<br />

#### `Getting Prerequisites`

Download and install MSYS2 with default settings.

https://repo.msys2.org/distrib/x86_64/msys2-x86_64-20230718.exe 

When it is finished, ensure "Run MSYS2 64bit now" is checked and press Finish.
In the MSYS command prompt that opens, enter:

```sh
pacman -Syyu
```

If you receive any messages saying things are in conflict, press y then enter until it asks "Proceed with installation?", then press y and enter one more time.

When it is done, it will ask you to close the window to restart.  Say "yes" or close the window.

After it closes, launch MinGW64 by clicking the Windows Start icon and going to "MSYS2 64bit" and selecting "MSYS2 MingGW 64-Bit". When the MingGW64 window appears, enter:

```sh
pacman -Syyu
```

When prompted to proceed with installation, press y then enter. After it finishes upgrading, enter:


```sh
pacman -S base-devel mingw-w64-x86_64-toolchain compression git python pv
```

Each time it says Enter a Selection, just press enter. When it says "Proceed with installation?" press y, then enter.
This will take some time depending upon your computers hardware and your internet speed. Please be patient until it finishes.

#### `Download Nexus Source and Dependency Script`

In the already open MinGW64 window, enter:

```sh
cd /c/
git clone --depth 1 https://github.com/Nexusoft/LLL-TAO
```

This will download the Nexus LLL-TAO source code to C:\LLL-TAO

#### `Compiling Dependencies` 

Run win_build.sh to download and compile dependencies by entering:


```sh
/c/LLL-TAO/win_build.sh install
```

Wait for process to complete. This can take a awhile depending on your computer and internet connection.
Please be patient while it completes the building process. This only needs to be successfully completed once.
After it completes successfully you will only need to follow the steps in Part 2: Compiling Nexus LLL-TAO.

<br />

### `Part 2: Compiling Nexus LLL-TAO`
<br />

If you just followed Part 1, skip to Building LLL-TAO below, depending on which you want.
If you're UPDATING your existing version do this first, then continue following Building LLL-TAO instructions.

#### `Getting New Copy of Nexus LLL-TAO Source`

To update your source code, launch MinGW64 by clicking the Windows Start icon and going to "MSYS2 64bit" and selecting "MSYS2 MingGW 64-Bit". When the MingGW64 window appears, enter:


```sh
cd /c/
mv --backup=numbered -T ./LLL-TAO ./LLL-TAO.bak
git clone --depth 1 https://github.com/Nexusoft/LLL-TAO
```

#### `-OR-` 

If you have win_build.sh, you can enter:


```sh
/c/LLL-TAO/win_build.sh update
```

#### `Building LLL-TAO` 
In the MinGW64 window, enter:


```sh
cd /c/LLL-TAO
make -f makefile.cli clean
make -f makefile.cli
```

This will create nexus.exe in the release folder.
	

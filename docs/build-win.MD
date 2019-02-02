REQUIREMENTS - 	64bit CPU and OS.
				At least 6GB free Hard drive space.
				Stable internet connection.

If you have already completed Part 1: Compiling the Dependencies, please skip to Part 2: Compiling Nexus LLL-TAO.

#### Part 1: Compiling the Dependencies ####

##### Getting Prerequisites #####

Download and install MSYS2 with default settings.

http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20161025.exe

When it is finished, ensure "Run MSYS2 64bit now" is checked and press Finish.
In the MSYS command prompt that opens, enter:
	
*	pacman -Syyu
	
If you receive any messages saying things are in conflict, press y then enter until it asks "Proceed with installation?", then press y and enter one more time.

When it is done, it will give you 2 warnings. Close the window with the X in the top right corner. After a few seconds it will warn you that processes are running. Click OK. Then keep clicking the X until it says "Terminal is not responding". Then, click "Close the Program" and wait for it to close.

After it closes, launch MinGW64 by clicking the Windows Start icon and going to "MSYS2 64bit" and selecting "MSYS2 MingGW 64-Bit". When the MingGW64 window appears, enter:

*	pacman -Syyu
	
When prompted to proceed with installation, press y then enter. After it finishes upgrading, enter:

*	pacman -S base-devel mingw-w64-x86_64-toolchain compression git python pv
	
Each time it says Enter a Selection, just press enter. When it says "Proceed with installation?" press y, then enter.
This will take some time depending upon your computers hardware and your internet speed. Please be patient until it finishes.

##### Download Nexus Source and Dependency Script #####

In the already open MinGW64 window, enter:

*	cd /c/
*	git clone --depth 1 https://github.com/Nexusoft/LLL-TAO
	
This will download the Nexus source code to C:\LLL-TAO

##### Compiling Dependencies #####

Run win_build.sh to download and compile dependencies by entering:

*	/c/Nexus/win_build.sh install
	
Wait for process to complete. This can take a awhile depending on your computer and internet connection.
Please be patient while it completes the building process. This only needs to be successfully completed once.
After it completes successfully you will only need to follow the steps in Part 2: Compiling Nexus LLL-TAO.

#### Part 2: Compiling Nexus LLL-TAO ####

If you just followed Part 1, skip to Building LLL-TAO below, depending on which you want.
If you're UPDATING your existing version do this first, then continue following Building LLL-TAO instructions.

##### Getting New Copy of Nexus LLL-TAO Source #####

To update your source code, launch MinGW64 by clicking the Windows Start icon and going to "MSYS2 64bit" and selecting "MSYS2 MingGW 64-Bit". When the MingGW64 window appears, enter:

*	cd /c/
*	mv --backup=numbered -T ./LLL-TAO ./LLL-TAO.bak
*	git clone --depth 1 https://github.com/Nexusoft/LLL-TAO

##### -OR- #####

If you have win_build.sh, you can enter:

*	/c/LLL-TAO/win_build.sh update

##### Building LLL-TAO #####
In the MinGW64 window, enter:

*	cd /c/LLL-TAO
*	make -f makefile.cli clean
*	make -f makefile.cli
	
This will create Nexus.exe in the release folder.
	

clear
#Detect OS. Run if MINGW64
export osdetect=$(uname -s | grep -o '^.......')
if [ $(uname -s | grep -o '^.......') == MINGW64 ]; then
	#if more or less than 1 argument, don't run.
	if [ $# != 1 ]; then
		echo -e "You must specify either install, update, or clean when running this script. \n"
		echo -e "install - Downloads and installs the required dependencies for compiling the Nexus LLL-TAO. \n"
		echo -e "update - Downloads newest version of Nexus LLL-TAO source code.\n"
        echo -e "clean - Removes all downloaded and installed dependents.\n"
		echo -e "For example '/c/LLL-TAO/win_build.sh install'"
		exit
	fi
	#Install and compile all dependencies for Nexus wallet
	if [ $1 == install ]; then
		#Create directories if they don't already exist.
			if [ ! -d /c/deps ]; then mkdir /c/deps; fi
			if [ ! -d /c/installfiles ]; then mkdir /c/installfiles; fi
			#Download Dependencies
			cd /c/installfiles
			if [ ! -d libdb ]; then
				echo -e "\nDownloading Berkeley DB"
				wget -c --append-output=/dev/null --show-progress -N http://download.oracle.com/berkeley-db/db-6.2.32.NC.tar.gz
			fi
			if [ ! -d openssl ]; then
				echo -e "\nDownloading OpenSSL"
				wget -c --append-output=/dev/null --show-progress -N https://www.openssl.org/source/openssl-1.1.0g.tar.gz
			fi
			#Extract Dependencies and show rough estimate progress bar.
			clear
			if [ ! -d libdb ]; then
				echo -e "\nExtracting Berkeley DB"
				tar zvxf db-6.2.32.NC.tar.gz | pv -l -s 11349 > /dev/null && mv db-6.2.32.NC libdb
			fi
			if [ ! -d openssl ]; then
				echo -e "\nExtracting OpenSSL"
				tar zvxf openssl-1.1.0g.tar.gz | pv -l -s 2496 > /dev/null && mv openssl-1.1.0g openssl
			fi
			#If files extracted to proper folders, delete the archives.
			if [ -d libdb ]; then rm db-6.2.32.NC.tar.gz; fi
			if [ -d openssl ]; then rm openssl-1.1.0g.tar.gz; fi
			clear
            echo -e "Building dependencies. This will take awhile.\n"
			#Compile libdb
			cd /c/installfiles/libdb/build_unix
			../dist/configure --prefix=/c/deps --silent --enable-mingw --enable-cxx --enable-static --enable-shared --with-cryptography=no --disable-replication
			make -j2
			make -j2 install
			#Compile openssl
			cd /c/installfiles/openssl
			./configure no-hw threads mingw64 --prefix=/c/deps
			make --silent
			make --silent install
			#Set dependent PATH environment variable and make it permanent in MinGW
			cd ~
			echo -e "export PATH=/c/deps/bin:$PATH" >> ~/.bashrc
			source ~/.bashrc
			#Clear screen and delete installfiles folder
			clear
			echo -e " "
			echo -e " Files are being cleaned up. This may take several minutes. Please be patient"
			echo -e " "
			cd /c/
			sleep 3
			rm -frv /c/installfiles | pv -l -s $(find /c/installfiles | wc -l) > /dev/null
			#Clear screen and alert user Nexus is now ready to compile
			cd /c/LLL-TAO
			clear
			echo -e " "
			echo -e "All dependencies are now built and Nexus LLL-TAO is ready to compile."
			echo -e " "
		#Update Nexus LLL-TAO source code from github and backup old Nexus LLL-TAO source code to LLL-TAO.bak.~#~ with
		#increasing numbers based on number of backups
		elif [ $1 == update ]; then
			clear
			echo -e "\nUpdating Nexus LLL-TAO source to latest version.\n"
			if [ -d /c/nxstempdir ]; then rm -rf /c/nxstempdir; else mkdir /c/nxstempdir; fi
			cd /c/nxstempdir
			echo "cd /c/" > /c/nxstempdir/temp.sh
			echo "mv --backup=numbered -T /c/LLL-TAO /c/LLL-TAO.bak" >> /c/nxstempdir/temp.sh
			echo "git clone --depth 1 https://github.com/Nexusoft/LLL-TAO &> /dev/null" >> /c/nxstempdir/temp.sh
			echo "cd /c/LLL-TAO" >> /c/nxstempdir/temp.sh
			echo "/c/LLL-TAO.bak/win_build.sh updatedsource" >> /c/nxstempdir/temp.sh
			/c/nxstempdir/temp.sh & disown
		#Special argument for finishing source update. Not to be specified manually.
		elif [ $1 == updatedsource ]; then
			clear
			rm -rf /c/nxstempdir
			echo -e "\nSource code updated.\n\nPress Enter to return to terminal"
		#Clean up all files and start over. Used if theres massive errors when building. Also shows a rough estimate progress bar.
		elif [ $1 == clean ]; then
			cd /c/
			if [ -d deps ]; then
				echo -e "\nDeleting C:\deps folder"
				rm -frv /c/deps | pv -l -s $(find /c/deps | wc -l) > /dev/null
			fi
			if [ -d installfiles ]; then
				echo -e "\nDeleting C:\installfiles folder"
				rm -frv /c/installfiles | pv -l -s $(find /c/installfiles | wc -l) > /dev/null
			fi
			if [ -d nxstempdir ]; then
				echo -e "\nDeleting C:\nxstempdir folder"
				rm -rf nxstempdir
			fi
			echo -e "\nSuccessfully removed all files.\n"
		#Invalid argument detection
		else
			echo -e "Invalid argument detected - " $1
			echo -e "\nYou must specify either install or update when running this script. \n"
			echo -e "install - Downloads and installs the required dependencies for compiling the Nexus LLL-TAO. \n"
			echo -e "update - Downloads newest version of Nexus LLL-TAO source code.\n"
			echo -e "clean - Removes all downloaded and installed dependents.\n"
			echo -e "For example '/c/LLL-TAO/win_build.sh install'"
		fi
#Do not run if NOT MINGW64
else
	clear
	echo -e "\nThis script should only be run on Windows using MinGW64. Exiting.\n"
fi

### Installing Dependencies ###

##### Install Xcode Tools and Brew #####

Open Finder, go to Utilities, and open Terminal.

To install the xcode command line tools, run:

*	xcode-select --install

Click Install, then Agree. When its done, run:

*	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

Press ENTER to continue when prompted, then type your password to install. When it finishes, run:

*	brew install berkeley-db git miniupnpc openssl
*	echo 'export PATH="/usr/local/opt/qt/bin:$PATH"' >> ~/.bash_profile
*	source ~/.bash_profile

### To build the command line version (CLI) #####

Make sure you are in your home directory, or the directory you would like the source code to be.

*	git clone --depth 1 https://github.com/Nexusoft/Nexus.git
*	cd Nexus
*	make -f makefile.cli

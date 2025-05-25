## Build Instructions for Linux on Raspberry Pi
-------------------------------------------------------------------------------
This is an instruction for using [LLL-TAO](https://github.com/Nexusoft/LLL-TAO) on Raspberry Pi as using CLI(Command Line Interface) and as using Remote Core for convenience controlling on Staking or Mining.

Also, This method can be applied for other NUC with Debian-based linux distributions or other ARM64-based devices with Debian-based linux distributions.

This instruction was written for LLL-TAO <code>5.1.5-rc14-8</code> and <code>merging</code> branch on <code>5/25/2025</code> currently.
<br />

-------------------------------------------------------------------------------
### `Requirements`
--------------------------------------------------------------------------------
* Raspberry Pi 4 or Above (Minimum **2GB RAM** Required)
	* The daemon was tested and works flawlessly on a Raspberry Pi 3B+ with 1GB RAM by [Dev Team](https://staging.nexus.io/ResourceHub/staking-guide), But use RPi4 or Above is better for performance.
	* **Below RPi2 Rev 1.1(~ARM Cortex A7) is not supported** because working as 32-bit only, The reason is ARM64(ARMv8) command set is not contained on AP. But after **RPi2 Rev 1.2 contains ARM Cortex A53 so it supports 64-bit(aarch64/ARM64)**.

* Minimum **1GB(For Lite Node)/30GB(For Full Node) Free Space** for installing blockchain database
	* Currently, The extracted full database is about 17GB approximately in size and grows over time. 
	* Lite node will only maintains block headers and signature chain data for minimizing the storage impact dramatically, So 250MB approximately on these days and also grows over time.

* **64-bit** Debian-based Linux for Operation System (Ubuntu Server is Recommanded)
	* All of contents of this instruction is written based for Ubuntu Server 20.04 LTS, But this method can be applied on every Debian-based linux distributions.
	* Also this method can be applied on other linux distributions with its own correspond commands, But not be tested.
	* **32-bit OS is not supported** because there is no <code>ARM32/ARMv7=1</code> parameter, also <code>32BIT=1</code> parameter means **x86** so this is not capatible.

* Stable internet connection
<br />

--------------------------------------------------------------------------------
### `Part 1. Installation of the Dependencies`
--------------------------------------------------------------------------------
Notice: You can skip this part if you already installed these dependencies.

First of all, You need to open terminal as user mode.(as <code>[username]@[devicename]:~$</code>)

Also, You can establish the SSH connection for using shell.(Especially for headless environment.)

After opening the terminal, Input this line below and press enter.
```
sudo apt-get install -y git make build-essential libssl-dev libdb-dev libdb++-dev
```
When the installation has finished, Go to next step.
<br />

--------------------------------------------------------------------------------
### `Part 2. Cloning Depository and Building Daemon`
--------------------------------------------------------------------------------
In this step, We are going to clone the [LLL-TAO](https://github.com/Nexusoft/LLL-TAO) on your destination folder from <code>merging</code> branch, 

Which is currently latest version except other branches that under developing.

First of all, Move to the destination directory where you want to place the sources.
```
cd ~/[your_favorite_directory]
```

Second, Input this command for clone the LLL-TAO repository.
```
# URL of this line is case sensitive. You should input this line carefully.
git clone https://github.com/Nexusoft/LLL-TAO
```

And will be shown like this;
```
Cloning to 'LLL-TAO'...
remote: Enumerating objects: 78604, done.
remote: Counting objects: 100% (582/582), done.
remote: Compressing objects: 100% (147/147), done.
remote: Total 78604 (delta 478), reused 436 (delta 435), pack-reused 78022 (from 3)
Getting objects: 100% (78604/78604), 73.79 MiB | 8.58 MiB/s, done.
Calculating delta: 100% (60471/60471), done.
$
```

After cloning the repository, you will get <code>LLL-TAO</code> directory which contains source files.

Move to this directory for next work.
```
cd LLL-TAO
```

Third, Checking out the desired branch.

On this time, We are going to using <code>merging</code> branch.

Because <code>master</code> branch is stable build but outdated.(<code>master</code> branch is 5.1.4 currently)

Also since <code>merging</code> branch is <code>origin</code> now, You may pass this section because git brings <code>origin</code> branch as default on cloning  always.

```
git checkout merging
```

Fourth, If you have done to checkout the branch, Let's compile the code.

You can add the parameters for building. See and check [Build Params Reference](https://github.com/Nexusoft/LLL-TAO/blob/merging/docs/build-params-reference.md).
```
# -j params means the 'Cores' for using the compile. Maximum is '4' on RPi2 Rev 1.2 and above
# You must input the 'ARM64=1' for recognizing the architecture.
# These params are case sensetive.
make -j 4 -f makefile.cli ARM64=1

# You also can input the NO_WALLET=1 for ignore the legecy Nexus that used databases,
# This will be default due to deprecating the legacy base, After hard forking to version 6.0.
make -j 4 -f makefile.cli ARM64=1 NO_WALLET=1
```
This compiling may takes long time.(~10 min approx.) Please wait patiently.

If compiling has done, File named <code>Nexus</code> will be came out. let's call this as 'Daemon'.

**Don't start daemon yet, We have to do something more before starting.**

Optional: You may add the <code>nexus</code> daemon as environment variable by coping it to <code>/usr/bin</code>, For calling daemon on globally.

```
# Register nexus daemon as environment variable for calling it on globally.
sudo cp nexus /usr/bin
# For removing it, you can use this;
sudo rm /usr/bin/nexus
```

When you had added it to environment variable before, Do this after recompiling the core for update or reinstallation purpose. 

If you don't, Old Nexus daemon will be called from <code>/usr/bin</code> and the version that you newly built will be ignored to be called.
<br />

--------------------------------------------------------------------------------
### `Part 3. Make Configuration File (nexus.conf)`
--------------------------------------------------------------------------------
Before starting the daemon, You should create the <code>nexus.conf</code>, Configuration file for daemon.

First, make <code>.Nexus</code> folder to on your home directory (~/) and move into.

This folder will store a lot of datas, Such as <code>logs</code>, <code>databases</code>, <code>transactions</code>, etc...

```
# Folder name is case sensetive
mkdir ~/.Nexus
cd ~/.Nexus
```

If you have done, Make <code>nexus.conf</code> file with your favorite text editor. (or using <code>echo '' >> nexus.conf</code>) 

On this time, I'm going to use VIM for editing codes.
```
sudo vim nexus.conf
```

After you opened the file with the editor(or open the body with using <code>echo</code>), Please input and fill these section to the file.

You must input the <code>ESSENTIAL</code> marked options for proper setup. 

Other options isn't necessary now because these could be turn on and off on the Nexus Interface if API has been connected correctly.

```
# # means comment, You don't need to put these on the file.
# Also for ignoring options, You can set the option as comment with adding '#' in front of the option line.
# Bracelet ('<', '>') are used a placeholder on this example. please remove when you input the value.
# example) rpcuser=<input your name here> -> rpcuser=examplename

#
# This is an example of nexus.conf for the instruction, Tailor it to your depolyment.
# DO NOT CHANGE THE NEXUS COMMAND LINE. Any options, put here
#
# This configuration file is for the 'merging' branch.
#
# YOU MUST ADD OPTIONS WHICH ARE "ESSENTIAL" MARKED FOR PROPER WORKING.
# Other options are optional.
#

# All of credential-related information will be masked on the log with letter 'X'. 

# [RECOMMANDED]This option will accept launch the process into the background. 
# You can turn off background process of daemon with 'killall nexus'. [Default : 0]
daemon=1

# [ESSENTIAL] Default RPC username and password.
# These are must to be difference with your wallet account because this information
# are used for accessing your node with RPC. (Not your wallet account.)
rpcuser=<input rpc username here>
rpcpassword=<input rpc password here>

# [ESSENTIAL] Default API username and password.
# These are must to be difference with your wallet account because this information
# are used for accessing API of your node. (Not your wallet account.)
apiuser=<input api username here>
apipassword=<input api password here>

# Allows API authentication to be disabled. [Default : 1]
apiauth=0

# Allows API with SSL connection for higher security. [Default : 0]
apissl=1

# Set custom port for RPC, API and API_SSL
# [Default: 8080 for API, 8443 for API_SSL, 9888 for RPC]
apiport=8080
apiportssl=8443
rpcport=9888

# Allow remote connection for API [Default : 0]
apiremote=1

# Allow remote connection for RPC [Default : 0]
rpcremote=1

# Sets log file verobosity for debugging purpose. [Default : 0]
verbose=1

# Enable mining server (for a remote miner to connect to) [Default : 0]
mining=1

# Enable staking (to generate block inside the process) [Default : 0]
stake=1

# If your application requires multiple users [Default : 0]
multiuser=1

# Allows you to specifically disable the processing of notifications [Default : 1]
processnotifications=0

# Allows the node to automatically login with Sigchain credentials. [Default : 0]
# WARNING : THIS IS NOT RECOMMANDED FOR SECURITY REASON.
# These value are remaining as plain text, So may this information will be leaked if
# the device of your node being hacked or this file has been taken by attacker.
# If you are not in situation that must use this option, Please login under Nexus Interface instead.
autologin=0
username=<username>
password=<password>
pin=<pin>

# If autologin is set to 1, this additional setting will create the signature chain if it does
# not exist. So use this option with autologin if you need. [Default : 0]
autocreate=0

# Preventing Orphan Block Flooding attack with listening the blocks and packet from
# connected node. [Default : both are 0.]
listen=1
listenport=0

# Manual node connection.
addnode=XXX.XXX.XXX.XXX

# Seed node to force connect to
connect=XXX.XXX.XXX.XXX

# Allow API connection for spectific IP
llpallowip=XXX.XXX.XXX.XXX

# Allow RPC connection for spectific IP
rpcallowip=XXX.XXX.XXX.XXX
```

If you have done, Save the file and move to the next step.
<br />

--------------------------------------------------------------------------------
### `Part 4. Opening API, SSL, RPC Ports`
--------------------------------------------------------------------------------

Well, This part is for network and firewall configration. 

You need to open these 3 ports below;

* API (Port 8080) : For controlling your wallet and account information, such as Transaction, Staking, Mining, ETC...
* API_SSL (Port 8443) : Secure version of API connection, Using SSL connection.
* RPC (Port 9888) : For getting ledgers(blocks) from other nodes and send these from yours.

This time, Since this instruction is written based for Ubuntu Server 20.24 LTS and it contains <code>netfilter</code> for base firewall, This method is for type of this environment.

So, Please just treat this method as reference, Customize and setup on your own system, With your own method.

These ports are **default value** that unmodified with overriding options.

If you have overridden these, Input the ports as your value properly.

This instruction will show you two methods; One is for editing rules on <code>iptables</code> directly and applying on the <code>netfilter</code>, 

And the other one is for editing rules on <code>UFW(Uncomplicated Firewall)</code>, The Front-end interface of <code>iptables</code> which is easier for using.
<br />

#### `Setting Rules on iptables Directly`
--------------------------------------------------------------------------------
##### `Prerequisites`
Since changing <code>iptables</code> rules is applied temporarily on runtime, When you restart after editing and applying the rules, It will be disappeared and the port will be blocked again.

So we are going to install <code>iptables-persistent</code> for preventing volatility.

```
sudo apt-get install -y iptables-persistent
```

##### `Update Rules`
So, If you have done for installing the <code>iptables-persistent</code>, Let's open the ports.

First, Open RPC port.
```
# All these command lines are case sensetive
# Open port for inbound
sudo iptables -I INPUT -p tcp --dport 9888 -j ACCEPT
# Open port for outbound
sudo iptables -I OUTPUT -p tcp --dport 9888 -j ACCEPT
```

Second, Open API port. 

For better security, You may whitelist the inbound/outbound ip.

In this time, Just open the port as globally to using as example.
```
# All these command lines are case sensetive
# Open port for inbound
sudo iptables -I INPUT -p tcp --dport 8080 -j ACCEPT
# Open port for outbound
sudo iptables -I OUTPUT -p tcp --dport 8080 -j ACCEPT
```

Third, Open API_SSL port.
```
# All these command lines are case sensetive
# Open port for inbound
sudo iptables -I INPUT -p tcp --dport 8443 -j ACCEPT
# Open port for outbound
sudo iptables -I OUTPUT -p tcp --dport 8443 -j ACCEPT
```

After done, Apply the rules and save to the <code>/etc/iptables/rules.v4</code> for making the rules could be loaded automatically.
```
sudo iptables-save > /etc/iptables/rules.v4
sudo service iptables restart
```
<br />

#### `Setting Rules on UFW`
--------------------------------------------------------------------------------
<code>UFW</code> is very easy to use firewall setting.

UFW is Front-end interface for <code>iptables</code>.

First, Open RPC port.
```
# Open port for inbound
sudo ufw allow from 9888/tcp
# Open port for outbound
sudo ufw allow out 9888/tcp
```

Second, Open API port. For better security, You may whitelist the inbound/outbound ip.
```
# Open port for inbound
sudo ufw allow from 8080/tcp
# Open port for inbound from specific IP
sudo ufw allow from XXX.XXX.XXX.XXX to any port 8080 proto tcp
# Open port for outbound
sudo ufw allow out 8080/tcp
# Open port for outbound from specific IP
sudo ufw allow out to XXX.XXX.XXX.XXX port 8080 proto tcp
```

Third, Open API_SSL port.
```
# Open port for inbound
sudo ufw allow from 8443/tcp
# Open port for inbound from specific IP
sudo ufw allow from XXX.XXX.XXX.XXX to any port 8443 proto tcp
# Open port for outbound
sudo ufw allow out 8443/tcp
# Open port for outbound from specific IP
sudo ufw allow out to XXX.XXX.XXX.XXX port 8443 proto tcp
```

Then, Restart service of <code>UFW</code>.
```
sudo service ufw restart
```

It's done for opening ports and ready for running daemon!
<br />

--------------------------------------------------------------------------------
### `Part 5. Running Nexus Daemon and Check the Logs`
--------------------------------------------------------------------------------
On this chapter, We are going to start the nexus daemon and learn about how to find the log files for maintaining nodes.

For starting daemon, You should go to the LLL-TAO folder that you compiled the daemon. 

And input <code>nexus</code> and enter gently.

Or if you registered the Nexus daemon as environment variable, Just input <code>nexus</code> anywhere.

Once you pressed, It shows the fully logs. 
```
[03:52:52.754] ---------------------------------------------------
[03:52:52.754] Startup time 2025-XX-XX XX:XX:XX UTC
[03:52:52.754] 5.1.5-rc14-8 Tritium++ CLI [LLD][ARM aarch64]
[03:52:52.754] Linux Build (created XXX XX 2025 XX:XX:XX)
[03:52:52.754] Using configuration file: /home/[user name]/.Nexus/nexus.conf
[03:52:52.754] Configuration file parameters: -apiauth=1....
```

On first startup, The daemon will download the bootstraps (Blockchain Database) for syncing the current block height.

It takes about few hours, So please waiting gently.

But you can't do the other work same time because the daemon running foreground.

When if you press <code>Ctrl+C</code> for canceling the daemon, The daemon will be stopped.

Don't be panic! The bootstraps will be downloaded again from where lastest you downloaded if you run the daemon again.

So for preventing this situation, You may to apply <code>daemon=1</code> flag to the <code>nexus.conf</code> for making daemon runs in background.

For knowing how to apply the flag to the <code>nexus.conf</code>, You may go upward and check the `Part 3. Make Configuration File (nexus.conf)`

Anyways, Once the daemon runs in background, You should have check the status of node from logging files because it doesn't appear on your shell.

First. go to <code>~/.Nexus/log</code> with your shell.
```
cd ~/.Nexus/log
```

If you are in the <code>log</code> folder, Check the log files with <code>dir</code> command. 

There should be some log files in there.

The file that named with lowest number is the latest log.

And if the logs are too long to read, The new file will be created with named as incremental number.

You can read these logs with <code>cat</code> command like this;
```
# For checking full latest logs
cat 0.log 
# For finding the lines that contains specific words.
cat 0.log | grep -a <magic words>
```

But this is not in login status yet, So let's find how to login on Nexus interface on different device.
<br />

--------------------------------------------------------------------------------
### `Part 6. Access Remote Core from Other Devices with Nexus Interface (Wallet)`
--------------------------------------------------------------------------------
As on introduction, LLL-TAO is only provided as CLI(Command Line Interface) for ARM64(aarch64) Architecture.

So, We need to connect to the node with other device such as Desktop PC, Android/iPhone or Mac.

First, Install the Nexus Wallet(Nexus Interface) on your device.

You can find this on [here](https://nexus.io/wallet).

Second, After installation, Go to <code>settings</code>.

Press <code>Core</code> tab, And select <code>Remote Core</code> mode.

Then you should input these informations on setting that you can see;

* IP Address : IP address of RPi that you are using as remote core(node).
  	* If you configured your network with IP router, You can input it's own local IP address for accepting local connection only. But it may block your connection when you are outside of the local ethternet. You may forward the port on your IP router settings for resolve the issue.
* API SSL : Using API SSL or not [Default : true]
	* For using API with SSL connection, The node should be configured as using SSL too.
* API non-SSL Port : Port value that you inputted on <code>nexus.conf</code>
* API SSL Port : Port value that you inputted on <code>nexus.conf</code>
* API username : API username that you set on <code>nexus.conf</code>
* API password : API password that you set on <code>nexus.conf</code>
* Log out on close : If you close the wallet (not node), Logging out from session automatically.
	* Default is false. If you are using stake or mining, turn off this option may help for turning on only nodes without logout. If you are on public, turn this option on for security purpose.
 
Once It done and setting is correct, The login window will be appeared.

Then you can use the wallet with transaction, staking and mining same method as Desktop Version Wallet.

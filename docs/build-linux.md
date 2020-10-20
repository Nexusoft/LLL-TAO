## Build Instructions for Ubuntu & Other Debian-based Linux Distributions

-----------------------------------
<br />

### `Install Dependencies`       

```sh
sudo apt-get install -y git make build-essential libssl-dev libdb-dev libdb++-dev
```
<br />

### `Build Process`


Please see param reference to learn about some additional options
[Build Params Reference](build-params-reference.md)

```sh

# Change directory to where you want the source to be.
cd ~/

# Clone the Nexus core repository
git clone https://github.com/Nexusoft/LLL-TAO

# Change directory into the source folder
cd LLL-TAO 

# Checkout the desired branch.  Here we are using master, but the in-development branch is `merging`
git checkout master

# Compile the code. You can increase the number of jobs (-j X) to speed up compilation, if you have CPU and RAM available 
make -j 4 -f makefile.cli

# Optional step - copy the daemon to usr/bin so that it is available globally
sudo cp nexus /usr/bin 

```
<br />

### `Optional daemon configuration file`

This creates a default nexus.conf with minimal configuration.  Please ensure to insert appropriate RPC and API username/passwords before executing

```sh

# Create the data directory in the default location
mkdir ~/.Nexus

cd ~/.Nexus/

# Write default configuration file
echo '
# run as background process
daemon=1

# accept connections
server=1

# set low logging verbosity
verbose=0

# set RPC and API credentials - PLEASE CHANGE THESE TO SUITABLE VALUES
rpcuser=<somerandomuser>
rpcpassword=<somerandompassword>
apiuser=<somerandomuser>
apipassword=<somerandompassword>

#process notifications (incoming transactions) automatically in background process
processnotifications=1

' >> nexus.conf

```
<br />

### `Optional download of latest blockchain bootstrap`

```sh
cd ~/.Nexus/

# Download the bootstrap.  This will be 5GB or larger, so might take some time depending on your connection
wget https://nexus.io/bootstrap/tritium/tritium.tar.gz

# Untar it
tar -xvf tritium.tar.gz

```
<br />

### `Run the core daemon process`

```sh
#if you have copied the binary to /usr/bin then you can simply start with this
nexus

# If you have not copied the binary, change directory to the LLL-TAO folder and run this
cd ~/LLL-TAO
# Run the core daemon 
./nexus
```
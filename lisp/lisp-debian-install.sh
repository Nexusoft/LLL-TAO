
#
# Install LISP dependecies and useful networking tools 
#
sudo apt-get update && sudo apt-get -yq install \
    gcc libc-dev python python-dev libffi-dev openssl libpcap-dev \
    curl wget iptables iproute2 tcpdump tcsh sudo traceroute iputils-ping \
    net-tools procps emacs jq

#
# Install LISP release in /lispers.net directory.
#
sudo mkdir /lispers.net; cd /lispers.net; curl --insecure -L https://www.dropbox.com/s/e87heamhl9t5asz/lisp-nexus.tgz | sudo gzip -dc | sudo tar -xf -

#
# Install python modules the lispers.net directory depends on.
#
sudo python /lispers.net/get-pip.py
sudo pip install -r /lispers.net/pip-requirements.txt



#
# Copy LISP startup config.
#
cd -
sudo cp RL /lispers.net/RL
sudo cp lisp.config.xtr /lispers.net/lisp.config.xtr
sudo cp lisp-join.py /lispers.net/lisp-join.py
sudo cp make-crypto-eid.py /lispers.net/make-crypto-eid.py

cd /lispers.net
sudo ./RL
sudo ./pslisp

#
# Show the EID's
#
curl --silent --insecure -u root: http://localhost:9090/lisp/api/database-mapping | jq .| egrep "eid-prefix"


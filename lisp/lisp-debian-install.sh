#!/bin/bash
#
# lisp-debian-install.sh
#
# This script is used to install the lispers.net LISP implementation on a
# debian system used with Nexus use-cases. It installs the python dependencies
# and networking tools to run LISP on a Nexus node.
#
# Run this script in the LLL-TAO/lisp directory.
#
# To start LISP, use the RL script.
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
sudo cp provision-lisp.py /lispers.net/provision-lisp.py
sudo cp lisp.config.xtr /lispers.net/lisp.config.xtr
sudo cp lisp-join.py /lispers.net/lisp-join.py
sudo cp make-crypto-eid.py /lispers.net/make-crypto-eid.py
exit

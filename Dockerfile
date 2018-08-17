# 
# Build Nexus/LISP docker image based on a Debian Linux kernel. This
# Dockerfile is for the Tritium release of the TAO. The docker image name
# should be called "tritium". To build, use the "docker build" command in
# the LLL-TAO git repo directory:
#
#     cd LLL-TAO
#     docker build -t tritium .
#
# If you don't have docker on your MacOS system, download it at:
#
#     https://download.docker.com/mac/stable/Docker.dmg
#
# Once you install it you can run docker commands in a Terminal window.
#

#
# We prefer Debian as a base OS.
#        
FROM debian:latest

#
# Get latest lispers.net release from Dropbox.
#
ENV LISP_URL https://www.dropbox.com/s/e87heamhl9t5asz/lisp-nexus.tgz

#
# Install tools we need for a networking geek.
#
RUN apt-get update && apt-get install -y \
    gcc libc-dev python python-dev libffi-dev openssl libpcap-dev \
    curl iptables iproute2 tcpdump tcsh sudo traceroute iputils-ping \
    net-tools procps emacs jq

#
# Install LISP release in /lispers.net directory.
#
RUN mkdir /lispers.net; cd /lispers.net; curl --insecure -L $LISP_URL | gzip -dc | tar -xf - 

#
# Install python modules the lispers.net directory depends on.
#
RUN python /lispers.net/get-pip.py
RUN pip install -r /lispers.net/pip-requirements.txt

#
# Make prompt hostname/container name, allow web interface to work, and put us
# in the /lispers.net directory when you attach to container.
#
#RUN echo 'PS1="`hostname | cut -d . -f 0` > "' >> /root/.profile
EXPOSE 8080
WORKDIR /lispers.net

#
# Put Nexus source-tree in docker image.
#
RUN mkdir /nexus
COPY ./ /nexus/
COPY nexus-config/nexus.conf /root/.Nexus/nexus.conf
COPY nexus-config/run-nexus /nexus/run-nexus

#
# Build Nexus.
#
RUN apt-get install -y build-essential libboost-all-dev libdb-dev libdb++-dev \
    libssl1.0-dev libminiupnpc-dev libqrencode-dev qt4-qmake libqt4-dev \
    lib32z1-dev
RUN cd /nexus; make -f makefile.cli

#
# Copy LISP startup config.
#
COPY lisp/RL /lispers.net/RL
COPY lisp/lisp.config.xtr /lispers.net/lisp.config.xtr
COPY lisp/lisp-join.py /lispers.net/lisp-join.py
COPY lisp/make-crypto-eid.py /lispers.net/make-crypto-eid.py

#
# Add some useful tcsh alias commands.
#
COPY nexus-config/.aliases /root/.aliases

#
# Startup lispers.net and nexus. Output some useful data and drop into tcsh.
#
ENV RUN_LISP    /lispers.net/RL
ENV RUN_NEXUS   /nexus/run-nexus
ENV RUN_GETINFO /nexus/nexus -lispnet getinfo
ENV RUN_PSLISP  /lispers.net/pslisp

CMD echo "Starting LISP ..."; $RUN_LISP; \
    echo "Starting Nexus ..."; $RUN_NEXUS; \
    sleep 1; $RUN_PSLISP; $RUN_GETINFO; tcsh

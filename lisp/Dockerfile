#
# Build LISP docker image based on ubuntu. Use one of the following commands:
#
#   docker build -t lispers.net/ubuntu .
#   docker build -t lispers.net/debian .
#   docker build -t lispers.net/centos .
#
# Then to run one of the above docker images in a container, type:
#
# docker run -p 8080 --privileged --name <name> -h <name> \
#            -v <host-dir>:/hostOS -ti lispers.net/ubuntu
#
# where:
#   <name> is a container name you select
#   <host-dir> is directory on your host OS that maps to directory called
#              /hostOS inside your container for easy file movement
#
FROM ubuntu:latest
#FROM debian:latest
#FROM centos:latest

#
# Install tools we need for a networking geek. Need the symlink so installing
# tzdata does not prompt for timezone input.
#
# If "apt-get update" gets signature verification issues, run:
#
#    docker system prune --force
#
# And then rebuild.
#
RUN apt-get update -y && apt-get install -y curl
RUN ln -snf /usr/share/zoneinfo/$(curl https://ipapi.co/timezone) \
    /etc/localtime
RUN apt-get install -y \
    gcc libc-dev python2 python2-dev golang libffi-dev openssl libpcap-dev \
    iptables iproute2 tcpdump tcsh sudo traceroute iputils-ping \
    net-tools procps emacs vim jq

#
# Install LISP release in /lispers.net directory. Two options exist to get
# a lispers.net release. Get tarball from Dropbox or from git repo in
# build/latest/lispers.net.tgz. Default is Dropbox. One of the two needs
# to be commented out or you get the git repo option.
#
RUN mkdir /lispers.net

#
# Get lispers.net release. Dropbox option.
#
#ENV LISP_URL https://www.dropbox.com/s/0t36qe03lh1t9c1/lispers.net.tgz
#RUN cd /lispers.net; curl --insecure -L $LISP_URL | gzip -dc | tar -xf -

#
# Get lispers.net release. Git repo option.
#
ENV LISP_TGZ ./latest/lispers.net.tgz
COPY $LISP_TGZ /lispers.net/.
RUN cd /lispers.net; cat `basename $LISP_TGZ` | gzip -dc | tar -xf -

#
# Install python2 modules the lispers.net directory depends on.
#
RUN curl https://bootstrap.pypa.io/pip/2.7/get-pip.py -o get-pip.py && \
    python2 get-pip.py
RUN python2 -m pip install cheroot bottle netifaces pcapy pcappy pyOpenSSL pycrypto requests curve25519-donna geopy ecdsa pycryptodome pytun future distro

#
# Install python3 modules the lispers.net directory depends on.
#
RUN apt-get install -y python3-pip
RUN pip3 install cheroot bottle netifaces pcapy pcappy pyOpenSSL pycrypto requests curve25519-donna geopy ecdsa pycryptodome python-pytun future distro

#
# Make prompt hostname/container name, allow web interface to work, and put us
# in the /lispers.net directory when you attach to container.
#
#RUN echo 'PS1="`hostname | cut -d . -f 0` > "' >> /root/.profile
EXPOSE 8080
WORKDIR /lispers.net

#
# Start up LISP when container is created.
#
COPY ./RL.docker /lispers.net/RL
COPY ./.cshrc /root/.cshrc
CMD /lispers.net/RL; sleep 1; /lispers.net/pslisp; tcsh

#------------------------------------------------------------------------------

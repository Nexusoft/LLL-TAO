
#Quickstart Guide to Building Nexus/LISP Docker Images
-----------------------------------------------------

The following document briefly describes the process to get tritium running on a docker image packaged with LISP.

##Building:

(1) Install docker on your host computer system. Download docker from the
    docker download website https://download.docker.com for your OS platform.

```
    Ubuntu 18.04

    sudo apt install -y apt-transport-https ca-certificates curl software-properties-common
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
    sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
    sudo apt update
    sudo apt install -y docker-ce
```

(2) From the LLL-TAO directory, type:

```
    docker build -t tritium .
```

This will build a docker image called "tritium".

##Running:    

(1) To create and start the container, type:

```
    docker run -p 9090 --privileged --name nexus-<fill-in> \
               -h nexus-<fill-in> -ti tritium
```

You should pick a unique container name so your container is unique with the
LISP mapping system.

If you run the script config/docker-run-tritium on your host OS then the
container name will be chosen for you and will be unique. And you then can
run multiple containers if you wish to have multiple Nexus nodes running.    

##Monitoring:

(1) To get into the container command-line shell, type:

```
    docker exec -it nexus-<fill-in> bash
```

If you type control-D or exit, you exit the container and it remains running.

(2) To look at your docker build images, type:

```
    docker images
```

You should see an image called "tritium" in the list.

(3) To look at your created containers, type:

```
    docker ps
```

You should see a container named "nexus-&lt;fill-in&gt;"

(4) You can monitor the LISP subsystem by pointing your browser to:

```
    http://localhost:9090/lisp
```

By default, the username is "root" with no password on the lispers.net login
webpage. Note if port number 9090 is used by another application on your
system, just choose another port number on the "docker run" command and
reference that port in the above URL.

When the container is started, the Nexus daemon and the LISP subsystem will
be operational and the Nexus daemon can run on the LISP overlay and/or the
Internet underlay concurrently.

##Important!:

To get IPv6 to work in the container (which Nexus uses for crypto-EID support),
you need to configure your docker daemon on your OS platform with the
following:

```
{
    "ipv6": true, "fixed-cidr-v6": '2001:db8:1::/64"
}
```

Look for preferences in Windows and MacOS docker daemons and on Linux put/add
the above clause in /etc/docker/daemon.json.

-------------------------------------------------------------------------------

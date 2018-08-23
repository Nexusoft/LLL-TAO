
Quickstart Guide to Building Nexus/LISP Docker Images
-----------------------------------------------------

Building:
        
(1) Install docker on your host computer system. Download docker from the
    docker download website https://download.docker.com for your OS platform.

(2) From the LLL-TAO directory, type:

    docker build -t tritium .

This will build a docker image called "tritium".

(3) To create and start the container, type:

    docker run -p 9999:8080 --privileged --name nexus-<fill-in> \
               -h nexus-<fill-in> -ti tritium

You should pick a value for <fill-in> so your container is unique with the
LISP mapping system.

If you run the script config/docker-run-tritium on your host OS then the
container name will be chosen for you.

Monitoring:

(1) To get into the container command-line shell, type:

    docker exec -it nexus-<fill-in> bash

If you type control-D or exit, you exit the container and it remains running.

(2) To look at your docker build images, type:

    docker images

You should see an image called "tritium" in the list.

(3) To look at your created containers, type:

    docker ps

You should see a container named "nexus-&lt;fill-in&gt;"

(4) You can monitor the LISP subsystem by pointing your browser to:

    https://localhost:9999/lisp

By default, the username is "root" with no password on the lispers.net login
webpage. Note if port number 9999 is used by another application on your
system, just choose another port number on the "docker run" command and
reference that port in the above URL.

When the container is started, the Nexus daemon and the LISP subsystem will
be operational and the Nexus daemon can run on the LISP overlay and/or the
Internet underlay concurrently.

-------------------------------------------------------------------------------






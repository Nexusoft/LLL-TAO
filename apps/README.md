This directory contains a set of web applications that use the Nexus
blockchain. They are written in python 2.7.x and use the Nexus python SDK.

Point your browser to:

    http://localhost:<port>

when you run your browser and applications on the same system. Otherwise,
change "localhost" to the hostname where the application is running. You
can run all applications in a docker container using the URL above when
you start the container with the following parameters:

    docker run \
        -p 1111:1111 -p 2222:2222 -p 3333:3333 -p 4444:4444 -p 5555:5555 \
        -p 8080:8080 -p 9090:9090 --privileged \
        --name nexus-apps -h nexus-apps -ti tritium

When the docker image "tritium" is built (from the dockerfile in LLL-TAO/
Dockerfile), the container will come up running the Nexus daemon and LISP
but you will have to initiate the applications manually. See how to bring up
each application below. And note the port numbers used for each distinct web
app.    
    
-------------------------------------------------------------------------------

cookbook - default listens on port 1111

Cookbook is an application that will teach you the Nexus APIs. It will use
the API directory using curl or use the python SDK. The user simply clicks
the API or SDK button. You can point cookbook to any Nexus node running
anywhere on the Internet or via a LISP overlay. Just change the URL to direct
SDK and API requests to the node. Bring up cookbook by typing:

    cd LLL-TAO/apps
    python cookbook.py &

Cookbook can run natively on any host OS that supports python 2.7.x.

-------------------------------------------------------------------------------
    
qrsc - default listens on port 2222

Qrsc (QR-code Supply Chain) is an application for allocating QR-codes for
supply-chain use cases. A QR code can be allocated to a user's genesis-ID as
well as any supply-chain item created. The app demonstrates how transfer and
claim of supply-chain item ownership can be transferred. Bring up qrsc by
typing:        

    cd LLL-TAO/apps
    python qrsc.py &

-------------------------------------------------------------------------------
    
mmp - default listens on port 3333

Mmp (Music Market Place) is an application for demonstrating how a 4 party
system works in the music economy. Artists, Music Labels, Distributors, and
Music Buyers interact with each other by transferring music assets and allowing
split payments to occur. Bring up mmp by typing:
    
    cd LLL-TAO/apps
    python mmp.py &
    
-------------------------------------------------------------------------------

edu - default listens on port 4444

Edu (Token Awards for Education) is an application that allows a Student to
earn award tokens by taking tests and then can use them to buy scholarship
tokens from universitiies. Teachers grant award tokens when a student receives
a score of 75% or better on a test. Universiites post their cost of scholarship
tokens in USD and in Award Tokens. Bring up edu by typing:

    cd LLL-TAO/apps
    python edu.py &

-------------------------------------------------------------------------------

watermark - default listens on port 5555

Watermark is an application that allows you to load in documents so they
can be watermarked. Watermarked documents can only be read by genesis-IDs
the creator allows. If you try to read a watermark document from a standard
document tool, it will fail. But if you read the document from the watermark
application from a loggoed in genesis-ID that has been white-listed by the
document creator, it will render readabile. Bring up watermark by typingzZ:
    
    cd LLL-TAO/apps
    python watermark.py &

-------------------------------------------------------------------------------

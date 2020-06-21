# P2P API
-----------------------------------

The P2P API can be used to establish a secure peer-to-peer connection between two nodes and then send and receive arbitrary data messages between them.  Callers do **not** need to know the IP address of the peer node as connections are requested only by a users username or genesis hash. Once the peer accepts a connection request, a direct connection is made between the two nodes and communication commences.

To begin a p2p session an initial connection request message is broadcast to the network, which propagates until the node where the peer is logged in is found.  The connection request is then added to the peers connection request list, which the peer can then choose to accept or ignore.  The connection request contains the IP address of the sending node, along with the senders genesis hash.    

If the peer accepts a connection request, a new socket connection is established directly from the peer to the sender.  **NOTE** Since a direct socket connection is being made, the requesting node must be reachable from the peer.  This means that they either need to have a public IP address or, if behind a router, have UPNP enabled or else manually configure the required port forwarding. 

Upon initial connection an immediate "handshake" takes place where both nodes will send a message, signed using the `network` key from their crypto object register.  The receiving node can then verify the message signature and also check that the public key matches what is stored the crypto object register of the sending signature chain.  This mechanism guarantees the identity of each peer, securing the communication.

Once a p2p connection is established, users can send a message to the other user via the `send/message` method.  The message is added to a FIFO queue in the receiving users session.  The user can then read the message using `peek/message` (to read the message leaving it in the queue) or `pop/message` to read and remove it from the queue.

**NOTE** Messages are ephemeral.  They are not stored on chain and not stored in the users local database.  If a connection is terminated then any unread messages are permanently lost.  

## App ID
When sending the initial connection request the caller must include an `appid` parameter. The app id allows calling applications to identify messages that belong to their application only.  Connections to a peer also include the appid, allowing different applications to establish separate connections between the same two peers.  It is recommended that integrators generate an appid that will be unique to their application and then use this for all p2p sessions that it establishes. 


## `Methods`

The following methods are currently supported by this API

[`request/connection`](#requestconnection)   
[`list/requests`](#listrequests)   
[`list/connections`](#listconnections)  
[`get/connection`](#getconnection)  
[`accept/connection`](acceptconnection)   
[`ignore/connection`](#ignoreconnection)   
[`terminate/connection`](#terminateconnection)   
[`send/message`](#sendmessage)   
[`peek/message`](#peekmessage)   
[`pop/message`](#popmessage)   


-----------------------------------
****

# `request/connection`

Makes a new outgoing connection request to a peer.  This method can optionally wait (for up to 30s) for the peer to accept the request. 


### Endpoint:

`/p2p/request/connection`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) is making the request. For single-user API mode the session should not be supplied.

`username` : The username identifying the signature chain that the connection should be made to.  Optional if genesis is supplied.

`genesis` : The genesis hash identifying the signature chain that the connection should be made to.  Optional if username is supplied.

`appid` : The application ID identifying the calling application that the connection and subsequent messages relate to.  It is recommended that integrators generate an appid that will be unique to their application and then use this for all p2p sessions that it establishes

`wait` : Optional boolean parameter instructing the method to wait for the peer to accept the request.  If this is set to true, the API method will block until a secure connection with the peer is established or a timeout occurs.  The timeout occurs 5 seconds before the API server's timeout (default 30s), ensuring that there is time for the method to return before the API call itself times out. If this omitted or set to false, the method will return immediately after broadcasting the connection request




### Return value JSON object:
```    
{
    "success": true
}
```

### Return values:

`success` : Always set to true if the API call was successful. 

****

# `list/requests`

Returns an array of all pending connection requests, either outgoing (you have made) or incoming (made to you) 


### Endpoint:

`/p2p/list/requests`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to list requests for. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application to return requests for.  

`incoming` : Optional boolean parameter indicating wether to return incoming (default) or outgoing requests.


### Return value JSON object:
```    
[
    {
        "appid": "myapplication1234",
        "genesis": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff1",
        "session": 2280617590063431598,
        "address": "102.181.38.207",
        "port": "8326",
        "timestamp": 1591270866
    }
]
```

### Return values:

`appid` : The calling applications unique identifier. 

`genesis` : The genesis hash of the signature chain requesting the connection. 

`session` : A session ID used to identify duplicate or stale requests.

`address` : The IP address of the node requesting the connection. 

`port` : The port that should be use to establish the connection. 

`timestamp` : The unified timestamp of when the request was received. 

****


# `list/connections`

Returns a list of all active P2P connections for the logged in user.

### Endpoint:

`/p2p/list/connections`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to list connections for. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application to return connections for.  



### Return value JSON object:
```    
[
    {
        "appid": "myapplication1234",
        "genesis": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff1",
        "session": 10842444336167407774,
        "messages": 0,
        "address": "102.181.38.207",
        "port": "8326",
        "latency": 0,
        "lastseen": 1591270498
    }
]
```

### Return values:

`appid` : The calling applications unique identifier. 

`genesis` : The genesis hash of the peer signature chain connected to. 

`session` : A session ID used to uniquely identify the connection.

`messages` : The number of unread messages received from the peer currently in the queue. 

`address` : The IP address of the peer. 

`port` : The remote port being used for the connection. 

`latency` : The estimated latency in milliseconds between the two nodes. 

`lastseen` : Timestamp of when a message (including a ping-pong) was last received from the node. 


****


# `get/connection`

Returns information about a single connection active P2P connection for the logged in user.

### Endpoint:

`/p2p/get/connection`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to list connections for. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the peer.  Optional if genesis is supplied.

`genesis` : The genesis hash of the peer.  Optional if username is supplied.


### Return value JSON object:
```    
{
    "appid": "myapplication1234",
    "genesis": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff1",
    "session": 10842444336167407774,
    "messages": 0,
    "address": "102.181.38.207",
    "port": "8326",
    "latency": 10,
    "lastseen": 1591270498
}
```

### Return values:

`appid` : The calling applications unique identifier. 

`genesis` : The genesis hash of the peer signature chain connected to. 

`session` : A session ID used to uniquely identify the connection.

`messages` : The number of unread messages received from the peer currently in the queue. 

`address` : The IP address of the peer. 

`port` : The remote port being used for the connection. 

`latency` : The estimated latency in milliseconds between the two nodes . 

`lastseen` : Timestamp of when a message (including a ping-pong) was last received from the node. 

****


# `accept/connection`

Accepts an incoming connection request from a peer and establishes a new secure connection.  If successful the new connection information is returned.

### Endpoint:

`/p2p/accept/connection`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the peer that made the request being accepted.  Optional if genesis is supplied.

`genesis` : The genesis hash of the peer that made the request being accepted.  Optional if username is supplied.


### Return value JSON object:
```    
{
    "appid": "myapplication1234",
    "genesis": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff1",
    "session": 10842444336167407774,
    "messages": 0,
    "address": "102.181.38.207",
    "port": "8326",
    "latency": 0,
    "lastseen": 1591270498
}
```

### Return values:

`appid` : The calling applications unique identifier. 

`genesis` : The genesis hash of the peer signature chain connected to. 

`session` : A session ID used to uniquely identify the connection.

`messages` : The number of unread messages received from the peer currently in the queue. 

`address` : The IP address of the peer. 

`port` : The remote port being used for the connection. 

`latency` : The estimated latency in milliseconds between the two nodes. 

`lastseen` : Timestamp of when a message (including a ping-pong) was last received from the node. 


****


# `ignore/connection`

Removes an incoming connection request from the pending list.

### Endpoint:

`/p2p/ignore/connection`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the peer that made the request being ignored.  Optional if genesis is supplied.

`genesis` : The genesis hash of the peer that made the request being ignored.  Optional if username is supplied.


### Return value JSON object:
```    
{
    "success": true
}
```

### Return values:

`success` : Always set to true if the API call was successful. 


****


# `terminate/connection`

Closes an existing P2P connection.  This will also delete any unread messages for both users.

### Endpoint:

`/p2p/terminate/connection`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the peer connected to.  Optional if genesis is supplied.

`genesis` : The genesis hash of the peer connected to.  Optional if username is supplied.


### Return value JSON object:
```    
{
    "success": true
}
```

### Return values:

`success` : Always set to true if the API call was successful. 


****


# `send/message`

Sends a data message to a connected peer

### Endpoint:

`/p2p/send/message`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the peer to send the message to.  Optional if genesis is supplied.

`genesis` : The genesis hash of the peer to send the message to.  Optional if username is supplied.

`data` : A string containing the message data to be sent.  If callers wish to send binary data then it is recommended that they base64 encode it before passing into the method.


### Return value JSON object:
```    
{
    "success": true
}
```

### Return values:

`success` : Always set to true if the API call was successful. 

****


# `peek/message`

Returns the oldest message received from the peer without removing it from the queue.  

### Endpoint:

`/p2p/peek/message`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the connected peer to retrieve the message from.  Optional if genesis is supplied.

`genesis` : The genesis hash of the connected peer to retrieve the message from.  Optional if username is supplied.



### Return value JSON object:
```    
{
    "data": "hello world"
}
```

### Return values:

`data` : A string containing the message data received.  


****


# `pop/message`

Returns the oldest message received from the peer and removes it from the queue.  

### Endpoint:

`/p2p/pop/message`


### Parameters:

`session` : In multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) to use. For single-user API mode the session should not be supplied.

`appid` : The application ID identifying the calling application.  

`username` : The username of the connected peer to retrieve the message from.  Optional if genesis is supplied.

`genesis` : The genesis hash of the connected peer to retrieve the message from.  Optional if username is supplied.



### Return value JSON object:
```    
{
    "data": "hello world"
}
```

### Return values:

`data` : A string containing the message data received.  


****
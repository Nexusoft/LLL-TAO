# Hybrid API
-----------------------------------

The Hybrid API provides methods for connecting, checkpointing, and managing hybrid networks on the main chain. 


## `Methods`

The following methods are currently supported by this API

[`create/checkpoint`](#create/checkpoint)   

-----------------------------------
***

# `create/checkpoint`

Creates a checkpoint for the hybrid network on the mainnet work.  This carries out a small proof of work to calculate a nonce value, which is then added to the next block.


### Endpoint:

`/system/get/info`


### Parameters:

`txid` : The checkpoint transaction ID hash to store in the mainchain.


### Return value JSON object:
```
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "nonce": 273627366395484
}

```

### Return values:

`txid` : If successful, the checkpoint transaction ID hash echoed back from the request.

`nonce` : The 64-bit nonce value calculated as a result of the proof-of-work hashing carried out on the txid  


***

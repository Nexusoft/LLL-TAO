# Crypto API
-----------------------------------

The Crypto API provides methods to read and manage the cryptographic keys held in the signature chain's crypto object register, in addition to methods to encrypt and decrypt data using those keys. 

Every signature chain owns a special register known as the crypto object register.  Held in this register are a number of public keys that can be used for a variety of purposes, both internally by the Nexus core and externally by 3rd party applications.  Each key is stored in a named slot allowing integrators to identify which key they wish to use.  

By default the crypto object register contains nine named keys: `auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`.   The crypto object register is created automatically as part of the genesis transaction when a signature chain is created via the `users/create/user` API method.  During initial creation the `auth`, `network`, and `sign` keys are generated and stored in the register.  The remaining keys can be populated as necessary using the `crypto/create/key` API method.

Each key stored in the crypto register is a 256-bit hash of the public key of an asymmetric (public-private) key pair. The cryptographic scheme used to generate the key pairs is configurable via the `crypto/change/scheme` API method. Nexus currently supports both Brainpool (ECDSA-based) and FALCON (Lattice-based) schemes.

**NOTE** All key pairs generated via the crypto API are based on the username, password, and pin of the users signature chain.  The **`private keys are NOT stored on-chain`**, but instead re-constructed as necessary based on the username, password, and pin of the logged in user.  This is important, as changing the password and pin of a signature chain (including recovering an account) will invalidate any existing keys held in the crypto object register.  Therefore, when a user changes their password/pin or uses their recovery seed to set a new password/pin, we will **`automatically regenerate`** new public keys for the following keys, if they have been previously set: `auth`, `lisp`, `network`, `sign`, `verify`, and `cert`.   

**NOTE** The `app1`, `app2`, and `app3` keys are `not` updated when a user changes their password/pin.  This is intentional as any 3rd party application using these keys to encrypt data would be left unable to decrypt them again.  To help 3rd party applications manage this situation, the API includes a method to retrieve the private key for the app1, app2, and app3 keys, allowing them to be stored off-chain and later used to decrypt data in the event that the user changes their password/pin. 


## `Methods`

The following methods are currently supported by this API

[`list/keys`](#listkeys)  
[`create/key`](#createkey)   
[`get/key`](#getkey)   
[`get/publickey`](#getpublickey)   
[`get/privatekey`](#getprivatekey)   
[`change/scheme`](#changescheme)   
[`encrypt/data`](#encryptdata)  
[`decrypt/data`](#decryptdata)   
[`sign/data`](#signdata)   
[`verify/signature`](#verifysignature)   
[`get/hash`](#gethash)   


-----------------------------------
****

# `list/keys`

Returns a list of all public keys in the crypto object register for the specified signature chain.  If no username/genesis is supplied then the method will default to listing keys for the logged in signature chain.


### Endpoint:

`/crypto/list/keys`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`session` : If no username/genesis is supplied, in multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) to list the keys from. For single-user API mode the session should not be supplied.


### Return value JSON object:
```    
[
    {
        "name": "auth",
        "scheme": "FALCON",
        "hashkey": "37wdeKaudAnBQrtrUTvWxnDtDjvYGWf6ttXp6QNsvf5t"
    },
    {
        "name": "network",
        "scheme": "FALCON",
        "hashkey": "AQXVkTMaDtncHW3pA7HR8bL5BfRqumW29JaAYn9QEWkV"
    },
    {
        "name": "app1",
        "scheme": "BRAINPOOL",
        "hashkey": "CbXyKRQbGTj7NaQHqhxDri5himo52L1vZT3rHerp6eBP"
    },
  
]

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The public key, hashed (using Skein-Keccak 256), and then base58 encoded.



****


# `create/key`

Generates a new public-private key pair and stores it in the crypto object register.  Users must be logged in to create a new key


### Endpoint:

`/crypto/create/key`


### Parameters:

`name` : The name of the key to create.  If a key already exists in the crypto object register for this key name then the method will return an error.  Accepted values are `auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be created for. For single-user API mode the session should not be supplied.

`scheme`: Optional.  When creating the app1, app2, app3, or any other key name apart from the six default keys, users can specify which scheme to use to generate the key pair.  Values can be `BRAINPOOL` or `FALCON`.   


### Return value JSON object:
```    
{
    "name": "app1",
    "scheme": "BRAINPOOL",
    "hashkey": "CbXyKRQbGTj7NaQHqhxDri5himo52L1vZT3rHerp6eBP"
}

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The newly created public key, hashed (using Skein-Keccak 256), and then base58 encoded.


****

# `get/key`

Returns the public key from the crypto object register for the specified key name, from the specified signature chain.  If no username/genesis is supplied then the method will default to searching the logged in signature chain.


### Endpoint:

`/crypto/get/key`


### Parameters:

`name` : The name of the public key to retrieve.

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`session` : If no username/genesis is supplied, in multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) to get the key from. For single-user API mode the session should not be supplied.


### Return value JSON object:
```    
{
    "name": "auth",
    "scheme": "FALCON",
    "hashkey": "CbXyKRQbGTj7NaQHqhxDri5himo52L1vZT3rHerp6eBP"
}

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The public key, hashed (using Skein-Keccak 256), and then base58 encoded.


****


# `get/publickey`

Returns the public key (as opposed to the hashed key that is returned by get/key) for a key stored in the users crypto object register. This can only be issued by the owner of the key, so the caller must be logged in to call this method.  The un-hashed public key is required when encrypting and decrypting data via the API. 


### Endpoint:

`/crypto/get/publickey`


### Parameters:

`name` : The name of the public key to return.  

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be returned from. For single-user API mode the session should not be supplied.


### Return value JSON object:
```    
{
    "publickey": "EZipQHgMrmyDo78Ahp7S7zCdpdwp6Q1abVXSv3GxiF6THvhGhvXmATrzMWwdimdrbkKS9LrbEa4foGxPNrizKrer"
}

```

### Return values:


`publickey` : The 64-bit public key, base58 encoded.


****


# `get/privatekey`

Returns the private key for a public key stored in the users crypto object register. This method can only be used to obtain the private key for the `app1`, `app2`, or `app3`keys. **`Cannot be used to obtain the private key for the six default keys (`auth`, `lisp`, `network`, `sign`, `verify`, and `cert`).`**  


### Endpoint:

`/crypto/get/privatekey`


### Parameters:

`name` : The name of the private key to return.  

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be returned from. For single-user API mode the session should not be supplied.


### Return value JSON object:
```    
{
    "privatekey": "2687a494478c3ffdbc5fdf73139a710470097777aca080572a68f2a6204eb27d431a6d81d9f3f15d4e6b1c4a70eb6fb2c345fa462b2a717397514a8190484001"
}

```

### Return values:


`privatekey` : The 512-bit private key, hex encoded.


****

# `change/scheme`

Change the signature scheme used to generate the public-private keys for the users signature chain as well as keys stored in the crypto object register.  Users must be logged in to create a new key.

**NOTE** Changing the cryptographic scheme will also regenerate the six default keys (`auth`, `lisp`, `network`, `sign`, `verify`, and `cert`) stored the crypto object register.  The `app1`, `app2`, `app3` keys are `not` modified. 


### Endpoint:

`/crypto/change/scheme`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be created for. For single-user API mode the session should not be supplied.

`scheme`: The new scheme to use.  Values can be `BRAINPOOL` or `FALCON`.   


### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d"
}

```

### Return values:

`txid` : The ID (hash) of the transaction that includes the scheme change.


****


# `encrypt/data`

This method can be used to encrypt arbitrary data using the AES256 encryption function and a shared symmetric key.  The shared key is generated using ECDH from a public key (from a peer) and local private key.  The resulting encrypted data can then be decrypted by either the party, as long as the other users public key is known. 

The `data` must be supplied as a base64 encoded string.  This will be decoded into a byte array before encryption.  

### Endpoint:

`/crypto/encrypt/data`


### Parameters:

`name` : The name of the private key to use to generate the shared key.  

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the private key should be taken from. For single-user API mode the session should not be supplied.

`publickey` : The public key used to to generate the shared key. This should be passed in as a base58-encoded string

`data` : The data to encrypt. This should be passed in as a base64-encoded string

### Return value JSON object:
```    
{
    "data": "qjJ4B+cOI/AK6hzmpIvDRyZX7sbXiWtvbutCBYSU9v4=",
    "publickey1": "Dd4Cb9gTgsT5wk6e1eCw9tTPMKSyCa9e7p2mxAggSRBWHY2cWBxExRMrjGk7PXY6TiysmvRoBvSSKsyybNdGPXFm",
    "publickey2": "EZipQHgMrmyDo78Ahp7S7zCdpdwp6Q1abVXSv3GxiF6THvhGhvXmATrzMWwdimdrbkKS9LrbEa4foGxPNrizKrer"
}


```

### Return values:

`data` : The AES256 encrypted data, base64 encoded.

`publickey1` : The first of the public keys used in the shared symmetric key.

`publickey2` : The second of the public keys used in the shared symmetric key.

****


# `decrypt/data`

Decrypt data previously encrypted using AES256 and a shared symmetric key.  The shared key is regenerated by API method using the specified private key and the supplied public (peer) key.  

This method can work in two "modes", either by generating the private key based on the logged in signature chain credentials (username/password/pin), or by passing in the known private key.  The first mode is the most secure as the private key is never exposed, but requires that the user must not change their password and/or pin after the initial encryption of the data.  This mode is useful when encrypted data is transient.  

Where the encrypted data is persistent, to safeguard against users changing their password/pin and no longer being able to decrypt the data, calling applications should make a copy of the private key used to decrypt the data and store it in their own application.  The second mode of this method allows callers to provide the private key to use to decrypt the data.  The private key for any key in the crypto object register (except the default six keys) can be obtained via `crypto/get/privatekey`   

The `data` must be supplied as a base64 encoded string.  This will be decoded into a byte array before decryption.  


### Endpoint:

`/crypto/decrypt/data`


### Parameters:

`name` : The name of the public key in the crypto object register that was used to encrypt the data.  

`pin` : The PIN for this signature chain. 

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) should be used to decrypt the data. For single-user API mode the session should not be supplied. 

`privatekey` : Optional, the private key to use to decrypt the data.  If this is provided then this will be used instead of generating the private key from the signature chain credentials. The private key should be hex encoded. 

`data` : The encrypted data to decrypt. This should be passed in as a base64-encoded string

### Return value JSON object:
```    
{
    "data": "aGVsbG8gd29ybGQ="
}

```

### Return values:

`data` : The decrypted data, base64 encoded.


****


# `sign/data`

Generates a signature for the data based on the private key for the keyname/user/pass/pin combination.  The signature can be used to prove ownership of the public key by passing the public key and data to the `crypto/verify/signature` API method 

The `data` must be supplied as a base64 encoded string.  This will be decoded into a byte array before being signed.  


### Endpoint:

`/crypto/sign/data`


### Parameters:

`name` : The name of the key in the crypto object register to use to sign the data.  

`pin` : The PIN for this signature chain. 

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) should be used to sign the data. For single-user API mode the session should not be supplied. 

`data` : The data to sign. This should be passed in as a base64-encoded string

### Return value JSON object:
```    
{
    "publickey": "Dd4Cb9gTgsT5wk6e1eCw9tTPMKSyCa9e7p2mxAggSRBWHY2cWBxExRMrjGk7PXY6TiysmvRoBvSSKsyybNdGPXFm",
    "scheme": "BRAINPOOL",
    "signature": "MIGEAkAZSA/O9DSiwmrCZ3p6xTQau5F1EiSsVcp+Vvetw1oCqJyxb5BwnyksPcQ4kHiWOzFZls45o4bQnhHED3VYH+xaAkBUJbg4+HbRFNXnqCFZt2Ud6NqIOGKCPDjrP1bG2NQSbdaWcyRTWm7Qgm1RJ+gohQXdkkCdajaYevqA2CreAYSE"
}


```

### Return values:

`publickey` : The corresponding public key that can be used to verify the signature, base58 encoded.

`scheme` : The cryptographic scheme used to generate the signature.  This can be `FALCON` or `BRAINPOOL`.

`signature` : The signature, base64 encoded.


****


# `verify/signature`

Verifies the signature is correct for the specified public key and data.  This method accepts the public key in either the hashed or raw formats

The `data` must be supplied as a base64 encoded string.  This will be decoded into a byte array before verifying.  


### Endpoint:

`/crypto/verify/signature`


### Parameters:
 

`publickey` : The public key to use to verify the signature, base58 encoded.

`scheme` : The cryptographic scheme that should be used to verify the signature.  Options are `FALCON` or `BRAINPOOL`.  

`signature` : The signature to be verified, base64 encoded.

`data` : The signed data. This should be passed in as a base64-encoded string

### Return value JSON object:
```    
{
    "verified": "true"
}

```

### Return values:

`verified` : Boolean flag indicating whether the signature was successfully verified or not.

****


# `get/hash`

Generates a hash of the data using the requested hashing function.

The `data` must be supplied as a base64 encoded string.  This will be decoded into a byte array before hashing.  


### Endpoint:

`/crypto/get/hash`


### Parameters:

`data` : The data to generate the hash for. This should be passed in as a base64-encoded string

`function` : Optional hashing function to use to generate the hash.  Options can be `SK256`, `SK512`, `ARGON2`.  If no function is provided then it will default to SK256.  


### Return value JSON object:
```    
{
    "hash": ""
}

```

### Return values:

`hash` : The hash of the data, returned as a hex encoded string.  The length of the hash will depend on the function used to generate it.  `SK256` results in an unsigned 256-bit integer, `SK512` results in an unsigned 512-bit integer, and `ARGON2` results in an unsigned 512-bit integer.   

****




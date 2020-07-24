# Crypto API
-----------------------------------

The Crypto API provides methods to generate public-private key pairs, encrypt and decrypt data, and manage the public key hashes held in the signature chain's crypto object register. 

Every signature chain owns a special register known as the crypto object register.  Held in this register are a number of public key hashes that can be used for a variety of purposes, both internally by the Nexus core and externally by 3rd party applications.  The crypto object register provides the ability to verify which identity if the user sending a public key, since the hash of the public key is stored in the crypto object register of the signature chain.  Each key is stored in a named slot allowing integrators to identify which key they wish to use.  

The crypto object register contains nine named keys: `auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`.   The crypto object register is created automatically as part of the genesis transaction when a signature chain is created via the `users/create/user` API method.  During initial creation the `auth`, `network`, and `sign` keys are generated and stored in the register.  The remaining keys can be populated as necessary using the `crypto/create/key` API method.

Each key stored in the crypto register is a 256-bit hash of the public key of an asymmetric (public-private) key pair. The cryptographic scheme used to generate the key pairs is configurable via the `crypto/change/scheme` API method. Nexus currently supports both Brainpool (ECDSA-based) and FALCON (Lattice-based) schemes.

**NOTE** All key pairs generated via the crypto API are based on the username, password, and pin of the users signature chain.  The **`private keys are NOT stored on-chain`**, but instead re-constructed as necessary based on the username, password, and pin of the logged in user.  This is important, as changing the password and pin of a signature chain (including recovering an account) will invalidate any existing keys held in the crypto object register.  Therefore, when a user changes their password/pin or uses their recovery seed to set a new password/pin, we will **`automatically regenerate`** new public key hashes for the following keys, if they have been previously set: `auth`, `lisp`, `network`, `sign`, `verify`, and `cert`.   

**NOTE** The `app1`, `app2`, and `app3` keys are `not` updated when a user changes their password/pin.  This is intentional as any 3rd party application using these keys to encrypt data would be left unable to decrypt them again.  To help 3rd party applications manage this situation, the API includes a method to retrieve the private key for the app1, app2, and app3 keys, allowing them to be stored off-chain and later used to decrypt data in the event that the user changes their password/pin. 

**NOTE** You are *not* required to create a key in the crypto object register (via `crypto/create/key`) in order to use the other methods in the crypto API (with the exception of `crypto/get/key`).  This is because the public and private keys are reconstructed as necessary from the logged in user credentials and key name.  However, callers will need to provide the key type (`scheme`) as a request parameter so that the correct key can be constructed.  


## `Methods`

The following methods are currently supported by this API

[`list/keys`](#listkeys)  
[`create/key`](#createkey)  
[`create/certificate`](#createcertificate)   
[`get/key`](#getkey)   
[`get/publickey`](#getpublickey)   
[`get/privatekey`](#getprivatekey)   
[`get/certificate`](#getcertificate)   
[`change/scheme`](#changescheme)   
[`encrypt/data`](#encryptdata)  
[`decrypt/data`](#decryptdata)   
[`sign/data`](#signdata)   
[`verify/signature`](#verifysignature)   
[`get/hash`](#gethash)   


-----------------------------------
****

# `list/keys`

Returns a list of all hashed public keys in the crypto object register for the specified signature chain.  If no username/genesis is supplied then the method will default to listing keys for the logged in signature chain.


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
        "hashkey": "017c682bd04e4f96f6d5988e7de205ea47a53ee0de21ab938bf2ff1bccb0ad2a"
    },
    {
        "name": "network",
        "scheme": "FALCON",
        "hashkey": "0198306717f228037301b63c3de29f4e0865ccd3ec81c4a3c6a329b8131cf1b1"
    },
    {
        "name": "app1",
        "scheme": "BRAINPOOL",
        "hashkey": "0231012604f483de2ba4e5c901563e0cfb2e0e6da69a220c407e5951ac49529d"
    },
  
]

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The public key, hashed (using Skein-Keccak 256).



****


# `create/key`

Generates a new public-private key pair and stores the hashed public key in the crypto object register.  Users must be logged in to create a new key


### Endpoint:

`/crypto/create/key`


### Parameters:

`name` : The name of the key to create.  If a key already exists in the crypto object register for this key name then the method will return an error.  Accepted values are `auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`. 

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be created for. For single-user API mode the session should not be supplied.

`scheme`: Optional.  When creating the app1, app2, app3, or any other key name apart from the six default keys, users can specify which scheme to use to generate the key pair.  Values can be `BRAINPOOL` or `FALCON`.   


### Return value JSON object:
```    
{
    "name": "app1",
    "scheme": "BRAINPOOL",
    "hashkey": "0231012604f483de2ba4e5c901563e0cfb2e0e6da69a220c407e5951ac49529d"
}

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.  **NOTE** The scheme is ignored if creating the `cert` key as this requires the use of BRAINPOOL.

`hashkey` : The newly created public key, hashed (using Skein-Keccak 256).


****

# `get/key`

Returns the hashed public key from the crypto object register for the specified key name, from the specified signature chain.  If no username/genesis is supplied then the method will default to searching the logged in signature chain.


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
    "hashkey": "0231012604f483de2ba4e5c901563e0cfb2e0e6da69a220c407e5951ac49529d"
}

```

### Return values:

`name` : The name of the key entry.

`scheme` : The cryptographic scheme used to generate the key pair.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The public key, hashed (using Skein-Keccak 256).


****


# `get/publickey`

Returns the public key (as opposed to the hashed public key that is returned by get/key) for a given key name.. This can only be issued by the owner of the key, so the caller must be logged in to call this method.  The un-hashed public key is required when encrypting and decrypting data, or verifying signatures, via the API. 


### Endpoint:

`/crypto/get/publickey`


### Parameters:

`name` : The name of the public key to return.  

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be returned from. For single-user API mode the session should not be supplied.

`scheme`: Optional.  When creating the app1, app2, app3, or any other key name apart from the six default keys, users can specify which scheme to use to generate the key pair.  Values can be `BRAINPOOL` or `FALCON`. If no scheme parameter is provided then it will be determined from the crypto object register (for previously created keys) or taken from the users signature chain configuration. 

### Return value JSON object:
```    
{
    "publickey": "EZipQHgMrmyDo78Ahp7S7zCdpdwp6Q1abVXSv3GxiF6THvhGhvXmATrzMWwdimdrbkKS9LrbEa4foGxPNrizKrer"
}

```

### Return values:


`publickey` : The 64-bit public key, base58 encoded.

`scheme` : The cryptographic scheme used to generate the key.  This can be `FALCON` or `BRAINPOOL`.


****


# `get/privatekey`

Returns the private key for a key pair. This method can only be used to obtain the private key for keys NOT stored in the crypto object register, as those are intended for signature verification only. **`Cannot be used to obtain the private key for the nine default keys (`auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`).`**  


### Endpoint:

`/crypto/get/privatekey`


### Parameters:

`name` : The name of the private key to return.  

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the key should be returned from. For single-user API mode the session should not be supplied.

`scheme`: Optional.  When creating the app1, app2, app3, or any other key name apart from the six default keys, users can specify which scheme to use to generate the key pair.  Values can be `BRAINPOOL` or `FALCON`. If no scheme parameter is provided then it will be determined from the crypto object register (for previously created keys) or taken from the users signature chain configuration.

### Return value JSON object:
```    
{
    "privatekey": "2687a494478c3ffdbc5fdf73139a710470097777aca080572a68f2a6204eb27d431a6d81d9f3f15d4e6b1c4a70eb6fb2c345fa462b2a717397514a8190484001"
}

```

### Return values:


`privatekey` : The 512-bit private key, hex encoded.


****


# `get/certificate`

Returns a self-signed x509 certificate for this signature chain valid for 365 days from the time the method is called.  The certificate is signed using the `cert` key from the crypto object register.  Users must be logged in to retrieve a certificate.  


### Endpoint:

`/crypto/get/certificate`


### Parameters:

`pin` : The PIN for this signature chain. 


### Return value JSON object:
```    
{
    "certificate": "LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk1JSUIzakNDQVVRQ0FRRXdDUVlIS29aSXpqMEVBVEJiTVE0d0RBWURWUVFLREFWT1pYaDFjekZKTUVjR0ExVUUKQXd4QVlUSmxOVEZsWkdOa05ERmhPREUxTW1KbVpXUmlNalJsTTJNeU1tVmxOV0UyTldRMlpEZGtOVEkwTVRRMgpZak01T1RFME5XSmpaV1F5TmpsaFpXWm1NREFlRncweU1EQTNNak13TURVM05EaGFGdzB5TVRBM01qTXdNRFUzCk5EaGFNRnN4RGpBTUJnTlZCQW9NQlU1bGVIVnpNVWt3UndZRFZRUURERUJoTW1VMU1XVmtZMlEwTVdFNE1UVXkKWW1abFpHSXlOR1V6WXpJeVpXVTFZVFkxWkRaa04yUTFNalF4TkRaaU16azVNVFExWW1ObFpESTJPV0ZsWm1ZdwpNRm93RkFZSEtvWkl6ajBDQVFZSkt5UURBd0lJQVFFT0EwSUFBZ3hsY0J5Y3QxVno0NHloQkF6ZGVQdDdaY29GCnNLRTVvUzBJcUlVd1NyQlc2YlIwRXUwY3dNNVBjOGlERDlHMC84N3haay9EV091TFZ3UzdxLytjMXVVd0NRWUgKS29aSXpqMEVBUU9CaUFBd2dZUUNRR1BTZmx5WVUvdGp4N0xZYzhUQzhlNXpLNjl6V1pZTnJJeHpyZGVOZGdvcgpQOXZ1azRnWkJ6SFJrNEJKZkdlVGIzSk1FbUVzSmw5bG1UWkU3eUFDRVMwQ1FBcDdVMEYvSGJtQWNBcnNya3lrCkovTGdnQU5rUWc1cHpGT3oxdFV1WWxJWUN2aHJRT3hFVit2SHE1Qk5vUUIzWUpiemxuS1BSWHRxYmYyRUprdXUKQ2dJPQotLS0tLUVORCBDRVJUSUZJQ0FURS0tLS0tCg=="
}

```

### Return values:

`certificate` : The certificate data.  This is returned in PEM format, and base64 encoded.  Calling applications should base64 decode the data before using/parsing the certificate. 

****

# `change/scheme`

Change the signature scheme used to generate the public-private keys for the users signature chain as well as keys stored in the crypto object register.  Users must be logged in to create a new key.

**NOTE** Changing the cryptographic scheme will also regenerate the five default keys (`auth`, `lisp`, `network`, `sign`, and `verify`) stored the crypto object register.  The `app1`, `app2`, `app3` keys are `not` modified. 


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

This method can be used to encrypt arbitrary data using the AES256 encryption function and a symmetric key.  The method supports three options for providing the symmetric key:
1) by providing an explicit key using the `key` parameter
2) by providing the key `name` and `pin` parameters.
3) by providing the key `name`, `pin`, and `peerkey` parameters, in which case a shared key is generated. 


If the `key` parameter is used then this key is used to encrypt the data.

If the `name` and `pin` parameters are provided (without `peerkey`) then the private key from the corresponding key is generated and used as the symmetric key to encrypt the data.

If the `peerkey` parameter is supplied then a shared symmetric key is generated from the peer public key and a local private key using ECDH (Elliptic Curve Diffie Hellman) key agreement.  This is useful for exchanging data securely with a single recipient, as the resulting encrypted data can then be decrypted by either the party, as long as the other users public key (peerkey) is known. For example Bob can encrypt data to be sent to Jane as long as Jane provides Bob with her public key.  Once encrypted, Jane can generate the same shared key to decrypt the data as long as she knows Bob's public key and her own private key.  Similarly Bob can decrypt the data using Jane's public key and his private key to generate the shared key.  Encrypting using a shared symmetric key is only supported for Elliptic Curve keys (i.e. `BRAINPOOL`) but not currently supported for lattice-based keys (i.e. `FALCON`).


### Endpoint:

`/crypto/encrypt/data`


### Parameters:

`key` : A 256-bit (32 character) symmetric key to use to encrypt the data.  Should only be provided if an external encryption key is known.  

`name` : The name of the private key to use to generate the encryption key. Not required if `key` is provided.  **`The name parameter can not be one of the nine default keys (`auth`, `lisp`, `network`, `sign`, `verify`, `cert`, `app1`, `app2`, and `app3`).`**

`pin` : The PIN for this signature chain. Not required if `key` is provided.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the private key should be taken from. For single-user API mode the session should not be supplied. Not required if `key` is provided.

`peerkey` : Optional, the public peer key used to to generate a shared key. This should be passed in as a base58-encoded string. Should only be provided if a shared key should be generated.

`data` : The data to encrypt. This is a string field, so callers wishing to pass in binary data data should base64-encode the data first.

### Return value JSON object:
```    
{
    "publickey": "Dd4Cb9gTgsT5wk6e1eCw9tTPMKSyCa9e7p2mxAggSRBWHY2cWBxExRMrjGk7PXY6TiysmvRoBvSSKsyybNdGPXFm",
    "hashkey": "cd3555b6f2a7d86a0cd8c239ff0c4d0e872fc26f44fab1b77ff40f52a1a1159b",
    "data": "vQVmV1FXoKwSLRS3IDq8ZRuaQMjqtlG0zvGob64a6EY="
}



```

### Return values:

`publickey` : The public key corresponding to the users private key used for encryption.  This is not returned if the caller provided an external `key` to use for encryption.

`scheme` : The cryptographic scheme used to generate the key.  This can be `FALCON` or `BRAINPOOL`.

`hashkey` : The public key, hashed (using Skein-Keccak 256). This is not returned if the caller provided an external `key` to use for encryption.

`data` : The AES256 encrypted data, base64 encoded.

****


# `decrypt/data`

Decrypt data previously encrypted using AES256 and a symmetric key.  

Similar to the encrypt/data method, the decrypt method supports three options for supplying the symmetric key used to decrypt the data:

1) by providing an explicit key using the `key` parameter
2) by providing the key `name` and `pin` parameters.
3) by providing the key `name`, `pin`, and `peerkey` parameters, in which case a shared key is generated. 

If the `key` parameter is used then this key is used to decrypt the data.

If the `name` and `pin` parameters are provided (without `publickey`) then the private key from the corresponding key is generated and used as the symmetric key to decrypt the data.

If the `peerkey` parameter is supplied then a shared symmetric key is generated from the peer public key and a local private key using ECDH (Elliptic Curve Diffie Hellman) key agreement.  The shared key is regenerated by API method using the specified private key (`name`) and the supplied public key (`peerkey`).  



### Endpoint:

`/crypto/decrypt/data`


### Parameters:

`key` : A 256-bit (32 character) symmetric key to use to decrypt the data.  Should only be provided if an external encryption key is known.  

`name` : The name of the private key to use to generate the decryption key. Not required if `key` is provided.

`pin` : The PIN for this signature chain. Not required if `key` is provided.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the private key should be taken from. For single-user API mode the session should not be supplied. Not required if `key` is provided.

`peerkey` : Optional, the peer public key used to to generate a shared key. This should be passed in as a base58-encoded string. Should only be provided if a shared key should be generated to decrypt the data.

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


### Endpoint:

`/crypto/sign/data`


### Parameters:

`name` : The name of the key in the crypto object register to use to sign the data.  

`pin` : The PIN for this signature chain. 

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) should be used to sign the data. For single-user API mode the session should not be supplied. 

`data` : The data to sign. This is a string field, so callers wishing to pass in binary data should base64-encode the data first.

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


### Endpoint:

`/crypto/verify/signature`


### Parameters:
 

`publickey` : The public key to use to verify the signature, base58 encoded.

`scheme` : The cryptographic scheme that should be used to verify the signature.  Options are `FALCON` or `BRAINPOOL`.  

`signature` : The signature to be verified, base64 encoded.

`data` : The signed data. This is a string field, so callers wishing to pass in binary data should base64-encode the data first.

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

`data` : The data to generate the hash from. This is a string field, so callers wishing to pass in binary data should base64-encode the data first.

`function` : Optional hashing function to use to generate the hash.  Options can be `SK256`, `SK512`, `ARGON2`.  If no function is provided then it will default to SK256.  


### Return value JSON object:
```    
{
    "hash": "73cc74e67a8b840039d992c0ec7d6637f67c3ed0ac78e8f375b292fbb23cc6c7"
}

```

### Return values:

`hash` : The hash of the data, returned as a hex encoded string.  The length of the hash will depend on the function used to generate it.  `SK256` results in an unsigned 256-bit integer, `SK512` results in an unsigned 512-bit integer, and `ARGON2` results in an unsigned 512-bit integer.   

****




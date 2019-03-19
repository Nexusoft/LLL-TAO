The Accounts API Interface Definition
-------------------------------------

### Create

This will create a new account for use on the network.

```
<method>:
  /accounts/create

<key>=<value> parameter pairs:
  username=<user>
  password=<password>
  pin=<pin>
```

Return value JSON object:
```
{
    "genesis": "c8749c4e748f3a9a3ca2b3199ed34d9501ef8a5df07d9dadcbacbef74202deb9",
    "hash": "9652d37fcd0ebd9cea0b4ee282aacdb0751c8bc37c10e866f8a8e57f39f397680752ec5295b23fea0252234f494066ab2ded541709c8224f9ec30438b47c8691",
    "nexthash": "257a3811555410db58922e206d2fb9e727c00e63074966effa5acc4247879bcd",
    "prevhash": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "pubkey": "031cdb48d689ac689e4d3cc7b3ff863d6cfb969623226584d7231b8842e514a357c5e1ed9784e96dc5496e04a39ac17ebe6b28cfb98416175980b34bb535ad5304",
    "sequence": 0,
    "signature": "30818402404117fdb407d26996f035e670e12f7ca820b073a7d172f6caf93ed8c7b20d5c99d1ffebdbc61ef348400b503c6305217da96fa01ed51da6279f4022293a74a6c102406a19edbb56427a58a2560b6048c2dfcfd120df2a0509f31420f9b6f9cb8864112a7dee458ad0505020fa81f293c31d2469cc7e3ebd3967b575df299f0b610993",
    "timestamp": 1545445970,
    "version": 1
}
```

This is a transaction object containing the information of your genesis transaction.

### Login

This will log you into this specific API instance for the building of new transactions.

```
<method>:
  /accounts/login

<key>=<value> parameter pairs:
  username=<username>
  password=<password>
  pin=<pin>
```

Return value JSON object:    
```
{
    "genesis": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "session": "16953804956473956578"
}

```

This is your genesis ID, or the identification number of your specific account.
The session is what is required to submit for every command thereafter, to allow
the managing of multiple login sessions. Your PIN is required for authentication,
which this version uses to validate your credentials.

### Logout

This will log you out of this specific API, and delete your credentials stored
in a secure allocator.

```
<method>:
  /accounts/logout

<key>=<value> parameter pairs:
   session=<session-id>
```

Return value JSON object:
```
{
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
}
```

This is your genesis ID, or the identification number of your specific account.

### Lock

This will lock your signature chain, making it unavailable for use in mining.
It takes a session and will check if the sig-chain has already been locked.

```
<method>:
  /accounts/lock

<key>=<value> parameter pairs:
  session=<session-id>
```

Return value JSON object:
```
{
    true
}
```

Session is required to identify what session you are locking wallet from.

### Unlock

This will unlock your signature chain for use in mining. It takes a session
and your pin, and caches it in a secure allocator.

```
<method>:
  /accounts/unlock

<key>=<value> parameter pairs:
  session=<session-id>
  pin=<pin>
```

Return value JSON object:
```
{
    true
}
```

Session is required to identify what session you are unlocking wallet from.
The PIN credential is also required for wallet unlock.

### Transactions

This will list off all of the transactions created by the logged in account.
You DO NOT need to be logged in to use this command.

```
<method>:
  /accounts/transactions

<key>=<value> parameter pairs:
  genesis=27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1

  or

  username=<my-account>

  page=<page-number>
  limit=<records-per-page>
  verbose<verbose-level>
```

Only the genesis or the username are required to fulfill this command. If the
genesis is supplied first, it will supersede the username. We recommend you to
only use one of the parameters, as typing in both genesis and username will
only resolve the genesis.

The verbose parameter defines how much you receive in return.

Verbose 0: version, sequence, timestamp, hash, and operation.
Verbose 1: genesis, nexthash, and prevhash.
Verbose 2: pubkey and signature.

Return value JSON object:
```
{
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "hash": "f92fd92b7671d8358b82633fdf147e82a95ced60b54ceb4465fb34410345534bff3e9a3224032af08e01cb3b2c1dd4f6dcbb2a600e4b8b3c1109ea11b59ae7b8",
    "nexthash": "943aaa1f98c14852ff301c743bf25b962aaf5bc04912dd3e6fb925f1f7d8147e",
    "prevhash": "ef2cde465647953604d7bd49a76b5dde26b55eab8de88661451164f6a8d242f4282b3af9b1893e73a635b51c062a5c8f99217384bf05abbddbf55e1a0e86ffd5",
    "pubkey": "0338a2acd3dde1a845ac9b959797f891556747a317f6f71caea39977eb2f4d5b15d6cad37aa3a3acc61ed7306b06631a038f6fcd6b585c7fb1d55cd96acc82db7b",
    "sequence": 6,
    "signature": "30818402405f1b486b556912edc1b87139c822409e4be7e1aae73a92aadf4ad36f46c90d3c4190ee9bff04e6e79fe27fad4557670ea04533d1ccdb4581de252f04403e36b402401734b064192c106ff070d922349a11eec82c57942824a8b92e44fcb6f5702fa31f190946f9b3bb98815be12f98649cdf7b7204942d1437557635060fb91095c0",
    "timestamp": 1545445264,
    "version": 1
    "operation", "Coinbase(nAmount=55)"
},
```

NOTE: if you use the username parameter, it will take slightly longer to
      calculate the username genesis with our brute-force protected hashing
      algorithm. For higher performance, use the genesis parameter.


### Notifications

This will list off all of the transactions sent to a particular genesis or
username. It is useful for identifying transactions that you need to accept
such as credits.

```
<method>:
  /accounts/notifications

<key>=<value> parameter pairs:
  genesis=27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1

  or

  username=<my-account>

  page=<page-number>
  limit=<records-per-page>
  verbose<verbose-level>
```

Only the genesis or the username are required to fulfill this command. If the
genesis is supplied first, it will supersede the username. We recommend you to
only use one of the parameters, as typing in both genesis and username will
only resolve the genesis.

The verbose parameter defines how much you receive in return.

Verbose 0: version, sequence, timestamp, hash, and operation.
Verbose 1: genesis, nexthash, and prevhash.
Verbose 2: pubkey and signature.

Return value JSON object:
```
{
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "hash": "f92fd92b7671d8358b82633fdf147e82a95ced60b54ceb4465fb34410345534bff3e9a3224032af08e01cb3b2c1dd4f6dcbb2a600e4b8b3c1109ea11b59ae7b8",
    "nexthash": "943aaa1f98c14852ff301c743bf25b962aaf5bc04912dd3e6fb925f1f7d8147e",
    "prevhash": "ef2cde465647953604d7bd49a76b5dde26b55eab8de88661451164f6a8d242f4282b3af9b1893e73a635b51c062a5c8f99217384bf05abbddbf55e1a0e86ffd5",
    "pubkey": "0338a2acd3dde1a845ac9b959797f891556747a317f6f71caea39977eb2f4d5b15d6cad37aa3a3acc61ed7306b06631a038f6fcd6b585c7fb1d55cd96acc82db7b",
    "sequence": 6,
    "signature": "30818402405f1b486b556912edc1b87139c822409e4be7e1aae73a92aadf4ad36f46c90d3c4190ee9bff04e6e79fe27fad4557670ea04533d1ccdb4581de252f04403e36b402401734b064192c106ff070d922349a11eec82c57942824a8b92e44fcb6f5702fa31f190946f9b3bb98815be12f98649cdf7b7204942d1437557635060fb91095c0",
    "timestamp": 1545445264,
    "version": 1
    "operation", "Coinbase(nAmount=55)"
},
```

NOTE: If you use the username parameter, it will take slightly longer to
      calculate the username genesis with our brute-force protected hashing
      algorithm. For higher performance, use the genesis parameter.

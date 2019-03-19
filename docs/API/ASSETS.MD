The Assets API Interface Definition
-----------------------------------


### Create

This will create a new asset or digital item.

```
<method>:
  /assets/create

<key>=<value> parameter pairs:
    pin=<pin>
    session=<session-id>
    data=<data>
    name=<name>
```

Return value JSON object:
```
{
    "txid": <asset-txid>
    "address": <asset-address>
}
```

The pin, session and data are required for this command. If no name is provided,
a random 256-bit register hash will be generated.


### Get

This will get the data from a digital asset.

```
<method>:
  /assets/get

<key>=<value> parameter pairs:
    name=<name>
    or
    address=<address>

```

Return value JSON object:
```
{
    "timestamp": <asset-timestamp>
    "owner": <asset-owner>
    "metadata": <asset-metadata>
}
```

An asset name or asset register address hash is required for this command. Will get
timestamp and ownership information as well as metadata for this asset.


### Transfer

This will transfer ownership of an asset or digital item.

```
<method>:
  /assets/transfer

<key>=<value> parameter pairs:
    pin=<pin>
    session=<session-id>

    destination=<destination>
    or
    username=<username>

    name=<name>
    or
    address=<address>


```

Return value JSON object:
```
{
    "txid": <asset-txid>
    "address": <asset-address>
}
```

A user pin and session id are required for this command. A destination or username,
and a name or address are also required for this command. This will transfer asset
ownership to another asset register or to a user account by specifying either the
asset name or the asset register address. Upon successful operation, a txid and
register address will be given.


### History

This will get the history of an asset or digital item as well as it's ownership.

```
<method>:
  /assets/history

<key>=<value> parameter pairs:
    name=<name>
    or
    address<asset-address>

```

Return value JSON object:
```
[
    {
        "owner": <asset-owner>
    }
    {
        "timestamp": <asset-timestamp>
    }
]
```

An asset name or address are required for this command. Upon successful operation,
this will return a list of register state information about ownership with
timestamps.


### Tokenize

This will tokenize an asset into fungible tokens that represent ownership.

```
<method>:
  /assets/tokenize

<key>=<value> parameter pairs:

```

Return value JSON object:
```
{
    null
}

```

TODO: Final remarks.

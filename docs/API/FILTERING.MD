# Filtering
-----------------------------------

All API endpoints support filtering the results for getting specific parts of the dataset. The following sections describe how to filter the JSON of given output.

The minimum required components of the URI to filter results are:

```
commands/verb/noun/<filter>
```

The above filter becomes an expression either on the end of the URI, or between an operator:

```
commands/verb/noun/<filter>/operator
```

## `Filtering`

In order to filter an object, one must know the JSON key they are filtering for.

```
{
    "id": 69,
    "OP": "CREDIT",
    "for": "COINBASE",
    "txid": "01dcaa27022f435d34d1aadf423c9673a46ae48e50740e9971e390e7187dd1bae23ad3d3d65900cc10f98ada7b8f0bec70693a3f8f49b30b7aeeba0cd485c10e",
    "contract": 2,
    "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
    "amount": 231.861868,
    "token": "0",
    "ticker": "NXS"
}
```

The above JSON statement can be filtered by the following:

```
commands/verb/noun/OP,to,amount,ticker
```

This would result in the following filtered object:

```
{
    "OP": "CREDIT",
    "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
    "amount": 231.861868,
    "ticker": "NXS"
}
```

One can include as many filters as desired, as long as keys exist in the returned JSON and they follow a strict no-spaces *.csv (Comma Separated Values) format. This allows you to choose specific data that's needed without needing to require filtering client side.

## `Recursive Filtering`

JSON hierarchy can be maintained and traversed recursively to any depth of the data structure. Let us take the following JSON object, a verbose transaction:

```
{
    "txid": "019427d69187a02a23305a152c167c0c3fb547426afbdbee47ae674c7ea5a3d8f70e9a29c818f213edf1ce2721ae8bb81652e8cc77aac842df3c371027fa4e60",
    "type": "tritium user",
    "version": 2,
    "sequence": 1731,
    "timestamp": 1610469941,
    "blockhash": "c469d4373da8a53f7ea73657e9498c1a4c967cb1f5a95cec7586263b564d7f1bdf21a0b595ab95102450282179e7622e18739ceecadac078e3a92ecf5969466e6731753312efe5f0b83833d3bbcff70d5d89576f88d750216944b4e8f8cffbb8c623fb6e677092c05347f4e22f1bfcd49f74e757dd32c5cfd50cefd2530ee9fb",
    "confirmations": 832340,
    "genesis": "a1bc1623d785130bd7bd725d523bd2bb755518646c3831ea7cd5f86883845901",
    "nexthash": "b30d3f2c9857513a7de27cdb077568cb80af17a411c465742c1faa30ffa6491e",
    "prevhash": "0128928b2727142fbdb7b43021e26395f66d34e56806fec3ae47533d887dcbf8307109656e4321747204b25d27d6a1d86331b855939655525bf964e3b191a8bd",
    "recovery": "0a3bc3cc04ceac76f93ed0aa37ee84af7110eb49339a9319467962dad582607a",
    "pubkey": "0299cdb659f1b3a00e3cf704f628701e803ee5743246079cf937712cc6e44275653cdae80e9a680904f030fa0691c581fec506409b22be1c39416df9d438d0476d",
    "signature": "30818402401e38f9b4eb0c8f1e98294890fe6cdb3ceeb0a73171d02767d03d1c62fed806466daca8f4f4e6e8c44f83b62b5ec647804edf1f00113bcb0c478686de0ebd69140240372cc4fc0dfc7d4bbf0f50df4ac934347591911be46bd85154aff5224af68e67738890187a997e8cdc04d5b41af5483e310ddb1d9c05b7c645bd750e4e325196",
    "contracts": [
        {
            "id": 0,
            "OP": "CREDIT",
            "for": "COINBASE",
            "txid": "01d7b744868473cab52285c45940bc8fd1ca6dc59ed42c535fb9e196d80c574b230745ce395d146ad261ef5736100d027a5595504c886deb975f7dca8ab97e81",
            "contract": 2,
            "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
            "amount": 234.732164,
            "token": "0",
            "ticker": "NXS"
        }
    ]
},

```

These nested JSON objects and arrays can be filtered recursively using the `.` operator. Please ensure endpoint does not include spaces following a strict *csv format.

```
commands/verb/noun/type,confirmations,genesis,contracts.OP,contracts.to,contracts.amount,contracts.ticker
```

When using recursive filtering, the nested hiearchy is retained.

```
{
    "type": "tritium user",
    "confirmations": 832340,
    "genesis": "a1bc1623d785130bd7bd725d523bd2bb755518646c3831ea7cd5f86883845901",
    "contracts": [
        {
            "OP": "CREDIT",
            "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
            "amount": 234.732164,
            "ticker": "NXS"
        }
    ]
},

```

-----------------------------------

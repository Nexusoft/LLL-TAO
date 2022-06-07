# Sorting
-----------------------------------

All commands that generate a list of values supports sorting. Sorting can drill down to specific parts of the JSON hierarchy to provide advanced sorting and paging abilities. All sorting happens as the data-set is built, allowing sorting on just about any part of the JSON object.

## `Parameters`

The following parameters can be used to apply **sorting** to the returned data-set.

`limit`: The number of records to return. *Default: 100*.

`page`: Zero-indexed page number that depends on `limit` for page boundaries.

`offset`: Alternative to `page`, offset can be used to page the results by index.

`order`: Descending **desc** or ascending **asc** as only permitted values.

`sort`: The column or field-name to apply the sorting logic to.

## `Alternative input`

The `limit` and `offset` parameters can be given with the following format:

```
limit=100.10
```

This above will map to the parameters of `limit=100` and `offset=10`.


## `Recursive Sorting`

This parameter supports moving up levels of JSON keys by using `.`. This is a recursive function so therfore allows traversing any amount of levels in a JSON hierarchy. Let us take the following JSON object:

```
{
    "modified": 1621782289,
    "json": {
        "account": "8Cdr874GBd8t6MaQ4BVK8fXVVpzVHrGwZpQquUVzUXZroruYdeR",
        "date": "12-21-2020"
    }
}
```

The following parameter would allow sorting through the hierarchy:

```
sort=json.date
```

Using this parameter we would apply a sort to a nested JSON object that would sort it by the date field. The Nexus API supports any number of nested statements. Take the following object:

```
{
    "txid": "01c1e74718bbcbc83f46a0e03a86538350779f61df2e385a4456d3926808d3a29e3ac9d8d3ba2ea4b625b52b525d22fb675066184c4552c40824214b4f575e1c",
    "type": "tritium user",
    "version": 3,
    "sequence": 3,
    "timestamp": 1635813560,
    "blockhash": "1341043478e2850c8d3f232e7f4602d609e9cba29e93e9a409ed4ba9e52bca97030426122dc55c2ab88c3da914f38409e5f3248b301950251c2b578e07992b19b192a7b7377a294e3e24c6ff68a6058c425074b3278307574568c6bccbc63c7dfde3d6933b4f1e041c7ddc05953c12988be6bb47f0ca2cd822eb033f4bf6e197",
    "confirmations": 18,
    "genesis": "b7918d3942a14bd98ceb0f1be4c8675cc833eda354665167140050eee935b1a2",
    "nexthash": "bafb32b0a09f726d5bd1a23f69bdd605fa53037b0247f1cbb45e3faf6c8f202c",
    "prevhash": "0126a0201d0e0dc27506a2fb90c6deb008c1ec99c6cf401d9397d973151c3c56207d184e059fe3cde9abdb029a77f632c220f28472faa413e4c06ba37c950fb8",
    "pubkey": "0310281593331ce546efb5b275cf77bec88b7f59f2b8e90552cdf2c66fde3d4e5a965ce2e3c501560ff5fe6c9d8db1245071a5c608bc728896ee1fff3b660c5e68",
    "signature": "308184024075ec99396598b601adfcba0a9e9a23ba4dfc68fbe5164a71e4e676e301532338934f4cd7822de687b96168536ed901951debeac6ec40f7aa5d36cb4076ddfef50240518ba3c8dc3a457463919ddc8bd3a62aed82a70f60082bdd36e1e725a58e21895835b70968a38f2b07349e9cc89e6c61807d4aa28bb0a38b9a542a9bfbe456bc",
    "contracts": [
        {
            "id": 0,
            "OP": "CREATE",
            "address": "8Ca7nWMah3tes3tmnXkz7H25FLrJ31JoaFxcJUsFhz2PyKMSxaL",
            "type": "OBJECT",
            "standard": "ACCOUNT",
            "object": {
                "balance": 0.0,
                "token": "0",
                "ticker": "NXS"
            }
        }
    ]
},
```
This list of transactions (only one shown for convenience) could be sorted by the object's token when created by using the following parameter:

```
sort=contracts.object.token
```

This command above would move up each level of JSON with each period, resulting in a sort by the contract object's token value.

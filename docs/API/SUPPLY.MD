### GetItem

This will get an item on the network by its register memory address.

```
/supply/getitem

address=0xffffffffffff
```

This will return a json object such as:
```
{
    "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "state": "newregister"
}
```

This is a transaction object containing the information about the current owner and the state data.

### Transfer

This will transfer an item from current logged in account to a new user. The new user must have an account for this to work.

```
/supply/transfer

pin=XXXX
address=0xffffffffffff
to=NEW_OWNER
```

This will return a json object such as:
```
{
    "address": "fabe394cd30cc8ba7ae51d07aac5a7458ee3a20649ba387c80df5515b2817769",
    "txid": "650b797ea923e13107b5cdcbea84da555905aac10df96715906a1a15e3cb0064fa0318f5a03a40c2f0e1c3c6c1ce5bbcacdd99752b0e72384f154b440a4c2337"
}
```

This shows the address transferred, and the txid that was associated with the transfer taking place.

### Submit

This will submit new data into the network, assigning ownership to the currently logged in account.

```
/supply/submit

pin=XXXX
data=XXXXX
```

This will return a json object such as:
```
{
    "address": "e6d9529c4cb27857d6f86159f53f5fa5a06dd1de4a52c8a4260a6f287f41b77c",
    "txid": "70e5d8b6227d209fafe4c90ed0ed7e63b23b72dc1349c60b37a00ed06e18215d5fa384da1b6522e24cb1467b11b0b0e8ac4e9db8374f09718ab1218e8da33a11"
}
```

This shows the new memory address created, and the txid that was associated with the transfer taking place.

### History

This will show all the state changes given to a register address including the change of ownership of the writing of new data to it.

```
/supply/history

address=XXXXX
```

This will return a json object such as:
```
[
    {
        "checksum": 9070277379865698600,
        "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
        "state": "0b6e657764617461696e746f",
        "type": 0,
        "version": 1
    }
]
```

This shows the raw data of a state register and its associated changes. This command does not require login to execute.

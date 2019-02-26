The Token API Interface Definition
-----------------------------------

### Create

Create a token object register.

```
<method>:
/token/create

<key>=<value> parameter pairs:
name=<name>
amount=<supply-count>
amount-pieces=<supply-count-divisor>
session=<session-id>
pin=<pin-number>
```

Return value JSON object:
```    
{
    "address" : "<register-address>"
}    
```

### Get Address

Get a previous created object register address by name.

```
<method>:
/token/getaddress

<key>=<value> parameter pairs:
name=<name>
```

Return value JSON object:
```
{
    "address": "<register-address>"
}
```

### Get Data

Get the contents of a previous created object register. The lookup is by name,
register address, or txid.

```
<method>:
/token/getdata

<key>=<value> parameter pairs:
name=<name>
address=<address>
txid=<txid>
```

Return value JSON object:
```
{
    "data" : '<contents-of-register>"
}
```

### Debit

Deduct a token amount from owner’s object register and send it to owner of
another object register address.

```
<method>:
/token/debit

<key>=<value> parameter pairs:
name=<name>
address=<address>
amount=<token-decrement>
to-name=<receive-name> or to-address=<receive-address>
session=<session-id>
pin=<pin-number>
```

Return value JSON object:
```
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}            
```

### Credit

Get the contents of a previous created object register. The lookup is by name,
register address, or txid.

```
<method>:
/token/credit

<key>=<value> parameter pairs:
name=<name>
address=<address>
amount=<token-increment>
from-name=<send-name> or from-address=<send-address>
txid=<txid>
session=<session-id>
pin=<pin-number>    
```

Return value JSON object:
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}            
```
                
### History

Return a history of recent token-based transactions that have occurred.

```
<method>:
/token/history

<key>=<value> parameter pairs:
count=<number of recent txids>
start=<optional-start-offset>
```

Returns:
```
[
    <list-of-token-txids>
]
```
    
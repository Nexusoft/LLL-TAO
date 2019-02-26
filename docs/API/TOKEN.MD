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

Input parameter descritpion:
<name>:
    Is an API caller's selected name for the object register.
<supply-count>:
    The initial token supply count.
<supply-count-divisor>:
    The granularity of fungibility of a single token in decimal places.
<session-id>:
     The value returned from the accounts/login API call.
<pin-number>:
     The input value used on the accounts/create API call.
```
    
Return value JSON object:
```    
{
    "address" : "<register-address>"
}    

Output parameter descritpion:
<register-address>:
    The address used for the register. Any address references in other token
    API calls use this address.    
```
    
### Get Address

Get a previous created object register address by name.

```
<method>:
/token/getaddress

<key>=<value> parameter pairs:
name=<name>

Input parameter descritpion:
<name>:
    A token name supplied on the token/create API call.        
```

Return value JSON object:
```
{
    "address": "<register-address>"
}

Output parameter descritpion:    
<register-address>:
    The address used for the register. Any address references in other token
    API calls use this address.    
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

Input parameter descritpion:
<name>:
    A token name supplied on the token/create API call.        
<address>:
    A token object register address returned from the token/create API call.
<txid>:
    A transaction-id obtained from token API return parameters or out of band.
```

Return value JSON object:
```
{
    "data" : "<contents-of-register>"
}

Output parameter description:
<contents-of-register>:
    The contents of the object register is returned to the API user.
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

Input parameter descritpion:
<name>:
    A token name supplied on the token/create API call.        
<address>:
    A token object register address returned from the token/create API call.
<token-decrement>:
    The amount number to decrement from the object register contents.
<receive-name>:
    The name of the object register that receives the tokens.
<receive-address>:
    The address of the object register that receives the tokens. Either this
    parameter or <receive-name> is used.
<session-id>:
     The value returned from the accounts/login API call.
<pin-number>:
     The input value used on the accounts/create API call.
```

Return value JSON object:
```
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}

Output parameter descritpion:
<new-amount>:
    The new amount after decrement of the object register.
<txid>:
    The transaction-id associated with this debit operation.            
```

### Credit

Increment a token amount to owner’s object register received from an owner of
another object register address.


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

Input parameter descritpion:
<name>:
    A token name supplied on the token/create API call.        
<address>:
    A token object register address returned from the token/create API call.
<token-increment>:
    The amount number to increment to the object register contents.
<send-name>:
    The name of the object register that receives the tokens.
<send-address>:
    The address of the object register that receives the tokens. Either this
    parameter or <send-name> is used.
<session-id>:
     The value returned from the accounts/login API call.
<pin-number>:
     The input value used on the accounts/create API call.

```

Return value JSON object:
```
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}            

Output parameter descritpion:
<new-amount>:
    The new amount after increment of the object register.
<txid>:
    The transaction-id associated with this credit operation.            
```
                
### History

Return a history of recent token-based transactions that have occurred.

```
<method>:
/token/history

<key>=<value> parameter pairs:
count=<number-of-recent-txids>
start=<optional-start-offset>

Input parameter descritpion:
<number-of-recent-txids>:
    The number or recent transactions the history API call should return.
<optional-start-offset>:
    The offset from the beginning of token transactions occurred for the
    user account. For example, {count=5,start=0} returns the first 5
    transactions, {count=10} returns the most recent 10 transactions.
```

Return value JSON object:
```
[
    <list-of-token-txids>
]

Output parameter descritpion:

<list-of-token-txids>:
    A dictionary array of values equal to what is returned from the supply/
    history API call.
```
    
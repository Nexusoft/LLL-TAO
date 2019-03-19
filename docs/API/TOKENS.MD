The Token API Interface Definition
-----------------------------------

### Create

Create a new token object register.

```
<method>:
    /token/create

<key>=<value> parameter pairs:
    session=<session-id>
    pin=<pin>
    identifier=<token-id>
    type=<account/token>
    name=<selected-name>


If type=token specified:
    supply=<total-supply>

Input parameter description:
<name>:
    Is an API caller's selected name for the object register.
<supply>:
    The initial token supply count.
<session>:
     The value returned from the accounts/login API call.
<pin>:
     The input value used on the accounts/create API call.
<type>:
     An account or token. Token will create a token object register,
     account will create an account to receive that token identifier.
```

Return value JSON object:
```    
{
    "txid"    : "<transaction-id>"
    "address" : "<register-address>"
}    

Output parameter description:
<address>:
    The address used for the register. Any address references in other token
    API calls use this address.

<txid>:
    The transaction ID that create was executed on.
```

### Credit

Increment a token amount to owner’s object register received from an owner of
another object register address.


```
<method>:
/tokens/credit

<key>=<value> parameter pairs:
name=<name>
address=<address>
amount=<token-increment>
from-name=<send-name> or from-address=<send-address>
txid=<txid>
session=<session-id>
pin=<pin>

Input parameter description:
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
<pin>:
     The input value used on the accounts/create API call.

```

Return value JSON object:
```
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}            

Output parameter description:
<new-amount>:
    The new amount after increment of the object register.
<txid>:
    The transaction-id associated with this credit operation.            
```

### Debit

Deduct a token amount from owner’s object register and send it to owner of
another object register address.

```
<method>:
/tokens/debit

    <key>=<value> parameter pairs:
name=<name>
address=<address>
amount=<token-decrement>
to-name=<receive-name> or to-address=<receive-address>
session=<session-id>
pin=<pin>

Input parameter description:
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
<pin>:
     The input value used on the accounts/create API call.
```

Return value JSON object:
```
{
    "amount" : <new-amount>
    "txid" : "<txid>"
}

Output parameter description:
<new-amount>:
    The new amount after decrement of the object register.
<txid>:
    The transaction-id associated with this debit operation.            
```


### Get

Get data from a digital token asset or token account such as version,
identifier, balance, max supply, and current supply.

```
<method>:
/tokens/get

type=<type>
name|address=<name>|<address>


Input parameter description:
<type>:
    Must be either "token" or "account" for the token register type.
<name>:
    A token name supplied on the token/create API call.   
<address>:
    A token object register address returned from the token/create API call.     
```

Return value JSON object:
```
{
    "version":       "<token/account-version>"
    "identifier":    "<token/account-identifier>"
    "balance":       "<account-balance>"
    "maxsupply":     "<token-maxsupply>"
    "currentsupply": "<token-currentsupply>"
}
```
The type can either be token or account. The type is required for this command.
The name or address can be specified and is require for this command. This will
give information about version and identifier in either case. If the type is an
account, this will give information about token account balance. If the type is
instead a token, this will give token information about max supply and current
supply.

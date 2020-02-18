# Invoices API
-----------------------------------

The Invoices API provides users and application developers the ability to issue and pay invoices on the Nexus blockchain. This API is a demonstration of how registers can be used to store arbitrary user data, and conditional contracts can be used to implement use-case rules. Invoices can be created and then sent to a recipient signature chain to be paid. The sending of the invoice is implemented using a TRANSFER contract, transferring ownership of the invoice register to the recipient.  A conditional contract is applied to the TRANSFER operation preventing the recipient from claiming ownership unless the invoice amount is paid to the issuer.   

The API supports invoices issued in NXS or any other token on the Nexus blockchain. The token currency of the payment account determines the token which must be used to pay the invoice.  

Invoices have mandatory fields that must be included when creating the invoice, such as recipient, payment account, and invoice line items each with a unit amount and quantity. Beyond these, callers are free to include any other fields in the JSON that they desire.  This provides a flexible and extensible API where calling applications can create invoices as basic or detailed as they require.  

**NOTE**: The invoice JSON data must not exceed the maximum register size of 1Kb.       

Once created an invoice is stored in a read-only register, so a permanent and immutable record of the invoice is stored on the blockchain.  The invoice is then transferred to the recipient who can see it as an outstanding invoice to be paid.  The transfer transaction cannot be claimed - i.e. it remains outstanding - until the required invoice amount is paid to the nominated payment account.  Once paid, the transfer is automatically claimed, passing ownership of the invoice register to the recipient signature chain.    



## `Methods`

The following methods are currently supported by this API

[`create/invoice`](#createinvoice)    
[`get/invoice`](#getinvoice)    
[`pay/invoice`](#payinvoice)    
[`cancel/invoice`](#cancelinvoice)    
[`list/invoice/history`](#listinvoicehistory)    

-----------------------------------
***

# `create/invoice`

Create a new invoice. The API supports an alternative endpoint that can include the invoice name in the URL.  For example `/invoices/create/invoice/INV123` will resolve to `invoices/create/invoice?name=INV123`.

In addition to the parameters documented here, callers are free to include any other fields in the JSON that they desire and these will be stored in the invoice register on chain. When later retrieving an invoice, all fields that were initially passed to the create/invoice method are returned in the response.    

### Endpoint:

`/invoices/create/invoice`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the invoice should be created with. For single-user API mode the session should not be supplied.

`recipient` : The genesis hash of the signature chain to issue the invoice to. This is optional if the recipient_username is provided.

`recipient_username` : The username identifying the user account (sig-chain) to issue the invoice to. This is optional if the recipient is provided.

`account_name` : The name identifying the account the invoice should be paid to. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`account` : The register address of the account the invoice should be paid to. This is optional if the name is provided.

`name` : An optional name to identify the invoice. If provided a Name object will also be created in the users local namespace, allowing the invoice to be accessed/retrieved by name. If no name is provided the account will need to be accessed/retrieved by its 256-bit register address.

`items` : Array of line items that make up this invoice.  At least one item in the items array must be included.  The total invoice amount is calculated as the sum of all item amounts, and each item amount is calculated as the `unit_amount` multiplied by the `units`    
{    
&nbsp;&nbsp;&nbsp;`unit_amount` : The unit amount to be invoiced for this line item.  This amount should be supplied in the currency of the payment account

&nbsp;&nbsp;&nbsp;`units` : The number of units to be invoiced at the unit amount.   
}




### Example:

The following example creates an invoice with two line items.  The first item is for a total of 3.3 NXS and the second for 5.5 NXS.  **NOTE** that there are several non-mandatory fields included here that the calling system wishes to store in the invoice, such as `number`, `PO`, `contact`, `sender_detail`, `recipient_detail`, and in the items `description`, `base_price`, and `tax`

```
{
    "account": "8Bx6ZmCev3DsGjoWuhfQSNmycdZT4cyKKJNc36NWTMik6Zkqh7N",
    "recipient": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "number": "0004",
    "PO": "Purch1234",
    "contact": "accounts@mycompany.com",
    "sender_detail": "My Company, 32 Some Street, Some Place",
    "recipient_detail": "Some recipient details such as address or email",
    "items": [
        {
            "description": "First item description",
            "base_price": 1.0,
            "tax": 0.1,
            "unit_amount": "1.1",
            "units": 3
        },
        {
            "description": "Second item description",
            "base_price": 5.0,
            "tax": 0.5,
            "unit_amount": "5.5",
            "units": 1
        }
    ]
}
```


### Return value JSON object:
```
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```


### Return values:

`txid` : The ID (hash) of the transaction that includes the invoice creation.

`address` : The register address for the invoice. The address (or name that hashes to this address) is needed when retrieving or paying the invoice.


***

# `get/invoice`

Retrieves information about an invoice. The API supports an alternative endpoint that can include the invoice name or address in the URL.  For example `/invoices/get/invoice/inv123` will resolve to `invoices/get/invoice?name=inv123`.

### Endpoint:

`/invoices/get/invoice`


### Parameters:

`name` : The name identifying the invoice. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the invoice was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the invoice.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the invoice. This is optional if the name is provided.

`fieldname`: This optional field can be used to filter the response to return only a single field from the invoice data.



### Return value JSON object:
```
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "account": "8Bx6ZmCev3DsGjoWuhfQSNmycdZT4cyKKJNc36NWTMik6Zkqh7N",
    "recipient": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "number": "0004",
    "PO": "Purch1234",
    "contact": "accounts@mycompany.com",
    "sender_detail": "My Company, 32 Some Street, Some Place",
    "recipient_detail": "Some recipient details such as address or email",
    "items": [
        {
            "description": "First item description",
            "base_price": 1.0,
            "tax": 0.1,
            "unit_amount": "1.1",
            "units": 3
        },
        {
            "description": "Second item description",
            "base_price": 5.0,
            "tax": 0.5,
            "unit_amount": "5.5",
            "units": 1
        }
    ],
    "amount": 8.8,
    "token": "0",
    "status": "PAID"
    }
```


### Return values:

`owner` : The genesis hash of the signature chain that owns this invoice.

`created` : The UNIX timestamp when the invoice was created.

`modified` : The UNIX timestamp when the invoice was last modified.

`name` : The name identifying the invoice. For privacy purposes, this is only included in the response if the caller is the owner of the invoice

`address` : The register address of the invoice. 

`recipient` : The genesis hash of the signature chain to issue the invoice to. 

`account` : The register address of the account the invoice should be paid to. 

`items` : Array of line items that make up this invoice.  
{    
&nbsp;&nbsp;&nbsp;`unit_amount` : The unit amount to be invoiced for this line item.  

&nbsp;&nbsp;&nbsp;`units` : The number of units to be invoiced at the unit amount.   
}

`amount` : The total invoice amount.  This is the sum of all line item total amounts (unit_amount x units).

`token` : The register address of the token that this invoice should be paid in.  Set to 0 for NXS transactions

`status` : The current status of this invoice.  Values can be `OUSTANDING` (the invoice has been issued but not paid), `PAID` (the invoice has been paid by the recipient), or `CANCELLED` (the invoice was cancelled by the issuer before payment).

***

# `pay/invoice`

Make a payment (debit) to settle an invoice. The API supports an alternative endpoint that can include the invoice name or address in the URL.  For example `/invoices/pay/invoice/inv123` will resolve to `invoices/pay/invoice?name=inv123`.

The API only supports paying an invoice in full, i.e. the amount to pay is determined by the invoice total.  

If payment is successful, ownership of the invoice is claimed by the caller's signature chain and the invoice status will change to `PAID`.

### Endpoint:

`/invoices/pay/invoice`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the account owner. For single-user API mode the session should not be supplied.

`name` : The name identifying the invoice to pay. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the invoice to pay. This is optional if the name is provided.

`name_from` : The name identifying the account to debit (pay the invoice from). This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_from` : The register address of the account to debit (pay the invoice from). This is optional if the name is provided.




### Return value JSON object:
```
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```


### Return values:

`txid` : The ID (hash) of the transaction that includes the DEBIT to pay the invoice AND the CLAIM to claim ownership of the invoice.

***

# `cancel/invoice`

Cancels an unpaid invoice so that ownership returns back to the issuers signature chain. The API supports an alternative endpoint that can include the invoice name or address in the URL.  For example `/invoices/cancel/invoice/inv123` will resolve to `invoices/cancel/invoice?name=inv123`.

**NOTE** Only the creator of an invoice has the ability to cancel, and only if it is unpaid, i.e. it has an `OUTSTANDING` status.

**NOTE** Cancelled invoices no longer appear in the recipients invoice list, but will continue to appear in the issuers list so that they have a record of the cancellation.


### Endpoint:

`/invoices/cancel/invoice`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the account owner. For single-user API mode the session should not be supplied.

`name` : The name identifying the invoice to cancel. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the invoice to cancel. This is optional if the name is provided.


### Return value JSON object:
```
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```


### Return values:

`txid` : The ID (hash) of the transaction that includes the claim contract back to the issuers signature chain.


***

# `list/invoice/history`

This will get the history of the transactions for an invoice showing when it was created & transferred (two contracts that happen at the same point in time when the invoice is created) and claimed (paid or cancelled).  The API supports an alternative endpoint that can include the invoice name (if known) or register address in the URI.  For example `/invoices/list/invoice/history/inv123` will resolve to `/invoices/list/invoice/history?name=inv123`.  

### Endpoint:

`/invoices/list/invoice/history`


### Parameters:

`name` : The name identifying the invoice. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the invoice was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the invoice.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the invoice. This is optional if the name is provided.


### Return value JSON object:
```
[
    {
        "type": "CLAIM",
        "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
        "modified": 1581389015,
        "checksum": 2752588510729237979,
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "created": 1581389015,
        "data": "{\"account\":\"8Bx6ZmCev3DsGjoWuhfQSNmycdZT4cyKKJNc36NWTMik6Zkqh7N\",\"recipient\":\"a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440\",\"number\":\"0004\",\"PO\":\"Purch1234\",\"contact\":\"paul@nexus.io\",\"items\":[{\"description\":\"item1 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":1},{\"description\":\"item2 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":3}],\"amount\":4.4,\"token\":\"8DC5b4uRPYmfF8DWJy6ja2qPbtN7kNpdonHQeKx1nfauRAWDMUG\"}"
    },
    {
        "type": "TRANSFER",
        "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
        "modified": 1581389015,
        "checksum": 3200103280975089335,
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "created": 1581389015,
        "data": "{\"account\":\"8Bx6ZmCev3DsGjoWuhfQSNmycdZT4cyKKJNc36NWTMik6Zkqh7N\",\"recipient\":\"a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440\",\"number\":\"0004\",\"PO\":\"Purch1234\",\"contact\":\"paul@nexus.io\",\"items\":[{\"description\":\"item1 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":1},{\"description\":\"item2 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":3}],\"amount\":4.4,\"token\":\"8DC5b4uRPYmfF8DWJy6ja2qPbtN7kNpdonHQeKx1nfauRAWDMUG\"}"
    },
    {
        "type": "CREATE",
        "owner": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
        "modified": 1581389015,
        "checksum": 3200103280975089335,
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "created": 1581389015,
        "data": "{\"account\":\"8Bx6ZmCev3DsGjoWuhfQSNmycdZT4cyKKJNc36NWTMik6Zkqh7N\",\"recipient\":\"a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440\",\"number\":\"0004\",\"PO\":\"Purch1234\",\"contact\":\"paul@nexus.io\",\"items\":[{\"description\":\"item1 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":1},{\"description\":\"item2 description\",\"base_price\":1.0,\"tax\":0.1,\"unit_amount\":\"1.1\",\"units\":3}],\"amount\":4.4,\"token\":\"8DC5b4uRPYmfF8DWJy6ja2qPbtN7kNpdonHQeKx1nfauRAWDMUG\"}"
    }
]

```

### Return values:

The return value is a JSON array of objects for each entry in the invoice's history:

`type` : The action that occurred - CREATE | TRANSFER | CLAIM

`owner` : The genesis hash of the signature chain that owned the invoice.

`modified` : The Unix timestamp of when the invoice was updated.

`checksum` : A checksum of the state register used for pruning.

`address` : The register address of the invoice

`name` : The name of the invoice

`data` : The JSON invoice data


***

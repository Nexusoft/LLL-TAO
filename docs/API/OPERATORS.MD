# Operators
-----------------------------------

The Nexus API, and according commands support varying types of operators. Operators allow computation to be performed on a filtered data-set, much like in an excel spreadsheet.

The minimum required components of the URI for an operator is:

```
commands/verb/noun/filter/operator
```

# `Filter`

In order to execute an operator, one must apply a basic filter to the data-set because we need to isolate a component of the returned data to operate on and return.

Take the following contract list:

```
{
    "id": 33,
    "OP": "CREDIT",
    "for": "COINBASE",
    "txid": "01625b58e131e61813f77e8e97404cb587d780c7951b055b4f531f2a8ea5a63fc3c95f179ec26fa138e639143615a10961719ffb1b33dd98e64d0bf4d9fe50ae",
    "contract": 2,
    "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
    "amount": 208.833665,
    "token": "0",
    "ticker": "NXS"
},
{
    "id": 34,
    "OP": "CREDIT",
    "for": "COINBASE",
    "txid": "01b7291259fda66e8dbde6de502bd811894e42b6f2dd9fc684bdf87e9a3f0f224e16df72c491355547191af28f1cdd84bed0b522686de837f5b346593646f40f",
    "contract": 2,
    "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
    "amount": 205.705007,
    "token": "0",
    "ticker": "NXS"
},
```

To operate on our `amount` field, we would first need to apply a filter:

```
commands/verb/noun/amount
```

This command will now prepare for an operator, that can be invoked by finishing the URI:

```
commands/verb/noun/amount/sum
```

This above command will return a JSON object with our filtered result:

```
{
    "amount": 414.53867,
}
```

# `Operators`

The following operators are supported for this API command-set:

[`array`](#array) - Generate a list of values given from a set of filtered results as a JSON array.

[`count`](#count) - Find the total number of entries returned in a given JSON list.

[`floor`](#floor) - Convert all values in a result to integers by stripping off decimals.

[`max`](#max)     - Find the maximum value in the given set, and return this as result.

[`mean`](#mean)   - Calculate the mean or average value across a set of filtered results.  

[`min`](#min)     - Find the minimum value in the given set, and return this as result.

[`mode`](#mode)    - Find the value with the most occurrences across the data-set.

[`sum`](#sum)     - Compute a sum of a set of values derived from filtered results.


**Example:**

```
finance/list/accounts/balance/sum
```

**Result:**

This command will return a sum of the balances for all accounts:

```
{
    "balance": 333.376
}
```

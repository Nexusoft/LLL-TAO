# Filtering Documentation

This document describes the filtering capabilities of the LLL-TAO API.

## Overview

All API endpoints support filtering results to retrieve specific parts of the dataset. This document explains how to filter JSON output effectively.

## Basic Filtering

### URI Structure

The minimum required components for filtering are:

```
commands/verb/noun/<filter>
```

Or when using with operators:

```
commands/verb/noun/<filter>/operator
```

### Simple Filtering

To filter an object, you need to know the JSON key you want to filter for. For example, given this JSON:

```json
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

You can filter specific fields:

```
commands/verb/noun/OP,to,amount,ticker
```

Resulting in:

```json
{
    "OP": "CREDIT",
    "to": "8BAv1H2YKfpaYC1RJMq9qjmMTiEV8DJiLpiKGqTY54wf2VPjznD",
    "amount": 231.861868,
    "ticker": "NXS"
}
```

## Recursive Filtering

### Nested JSON Filtering

The API supports filtering nested JSON structures using the `.` operator. For example, given this complex JSON:

```json
{
    "txid": "019427d69187a02a23305a152c167c0c3fb547426afbdbee47ae674c7ea5a3d8f70e9a29c818f213edf1ce2721ae8bb81652e8cc77aac842df3c371027fa4e60",
    "type": "tritium user",
    "version": 2,
    "sequence": 1731,
    "timestamp": 1610469941,
    "blockhash": "c469d4373da8a53f7ea73657e9498c1a4c967cb1f5a95cec7586263b564d7f1bdf21a0b595ab95102450282179e7622e18739ceecadac078e3a92ecf5969466e6731753312efe5f0b83833d3bbcff70d5d89576f88d750216944b4e8f8cffbb8c623fb6e677092c05347f4e22f1bfcd49f74e757dd32c5cfd50cefd2530ee9fb",
    "confirmations": 832340,
    "genesis": "a1bc1623d785130bd7bd725d523bd2bb755518646c3831ea7cd5f86883845901",
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
}
```

You can filter nested fields:

```
commands/verb/noun/type,confirmations,genesis,contracts.OP,contracts.to,contracts.amount,contracts.ticker
```

Resulting in:

```json
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
}
```

## Best Practices

1. Use comma-separated values for multiple filters
2. Maintain JSON hierarchy with dot notation
3. Avoid spaces in filter strings
4. Filter only needed fields
5. Use recursive filtering for nested data
6. Test filter combinations
7. Document complex filters
8. Monitor filter performance
9. Cache common filters
10. Validate filter syntax

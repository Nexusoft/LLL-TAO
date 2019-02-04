The LISP API Interface Definition
-----------------------------------

### MyEIDs

This will return the Lisp Endpoint Indentifers (EID's) currently configured for this node.

```
<method>:
/lisp/myeids

```

Return value JSON object:
```
{
    "eids": [
        "240.232.220.134/32",
        "fe::232:220:134/128"
    ]
}
```
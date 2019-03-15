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
    "result":
    {
        "eids": 
        [
            "240.231.221.134/32",
            "fe00::231:221:134/128"
        ]
    }
}
```

### MyRLOCs

This will return the Lisp Routing Locators (RLOC's) currently configured for this node.

```
<method>:
/lisp/myrlocs

```

Return value JSON object:
```
{
    "result":
    {
        "rlocs": 
        [
            "192.168.0.52"
        ]
    }
}
```

### DatabaseMapping

This will return the Lisp EID and RLOC mappings for this node.

```
<method>:
/lisp/databasemapping

```

Return value JSON object:
```
{
    "result":
    {
        "database-mapping":
        [
            {
                "prefix":
                {
                    "eid-prefix":"240.139.76.244/32",
                    "instance-id":"200"
                }
            },
            {
                "prefix":
                {
                    "eid-prefix":"fe00::139:76:244/128",
                    "instance-id":"200"
                }
            },
            {
                "rloc":
                {
                    "interface":"wlp59s0"
                }
            },
            {
                "rloc":
                {
                    "address":"192.168.0.52"
                }
            }
        ]
    }
}
```
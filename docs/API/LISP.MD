The LISP API Interface Definition
-----------------------------------

### EIDs

This will return the Lisp Endpoint Indentifers (EID's) currently configured for this node.

```
<method>:
/lisp/eids

```

Return value JSON object:
```
{
    "result":
    {
        "eids":
        [
            {
                "instance-id":"200",
                "eid":"240.139.76.244",
                "rlocs":
                [
                    {
                        "interface":"eth0",
                        "rloc-name":"mypc",
                        "rloc":"101.123.45.67"
                    }
                ]
            },
            {
                "instance-id":"200",
                "eid":"fe::139:76:244",
                "rlocs":
                [
                    {
                        "interface":"eth0",
                        "rloc-name":"mypc",
                        "rloc":"101.123.45.67"
                    }
                ]
            }
        ]
    }
}
```


# `Formats`

When creating or modifying a register for any command-set, a format is required to encode the operations to the given register. The following sections outline the formats and their corresponding syntax. This syntax is accessed when one supplies `format=basic` as a paramter to a command that creates a new register, or updates its current state.

## `readonly` or `raw`

Takes an additional parameter `data=<string>` and dumps this into the state of a `raw` or `readonly` register. The `readonly` format can only be used when creating a *readonly* register. The following shows how to create a register with this format:

```
assets/create/raw name=test format=raw data=this is my raw data string
```

Would result in an object with the following structure:

```
{
    "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
    "version": 1,
    "created": 1655279431,
    "modified": 1655279431,
    "type": "RAW",
    "data" : "this is my raw data string",
    "address": "87Wai2JoS4hNAEVXZVmejLS6pK21XQWKoLAkaep5aXFdrYnJJyk",
    "name": "test"
}
```


## `basic`

List extra parameters not required by command as a set of string-converted `<key>/<value>` pairs. The following demonstrates how to create an asset using the basic format:

```
assets/create/asset name=test format=basic field1=value1 field2=value2 field3=321
```

Would result in an object with the following structure:

```
{
    "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
    "version": 1,
    "created": 1655279431,
    "modified": 1655279431,
    "type": "OBJECT",
    "field1" : "value1",
    "field2" : "value2",
    "field3" : "321"
    "address": "87Wai2JoS4hNAEVXZVmejLS6pK21XQWKoLAkaep5aXFdrYnJJyk",
    "name": "test"
}
```

This format can be used for both create and update commands for any command-set. The following demonstrates how to update an asset using the basic format:

```
assets/update/asset name=test format=basic field1=100
```

Would result in an object with the following structure:

```
{
    "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
    "version": 1,
    "created": 1655279431,
    "modified": 1655279431,
    "type": "OBJECT",
    "field1" : "100",
    "field2" : "value2",
    "field3" : "321"
    "address": "87Wai2JoS4hNAEVXZVmejLS6pK21XQWKoLAkaep5aXFdrYnJJyk",
    "name": "test"
}
```

**NOTE:** The basic format only works for an object register of supported type `asset` or `schema`.

## `JSON`

If **format** is `JSON`, then this field will hold the json definition of the asset as a JSON array of objects representing each field in the object. It uses the following format:

- `name` : The name of the data field.

- `type` : The data type to use for this field. Values can be uint8, uint16, uint32, uint64, uint256, uint512, uint1024, string, or bytes.

- `value` : The default value of the field.

- `mutable` : The boolean field to indicate whether the field is writable (true) or
read-only (false).

- `maxlength`: Only applicable to string or bytes fields where mutable=true, this is the maximum number of characters (bytes) that can be stored in the field. If no maxlength parameter is provided then we will default the field size to the length of the default value rounded up to the nearest 64 bytes.

# Sessions API 
-----------------------------------

The sessions API provides methods for creating and managing sessions. A session is synonymous with a user login into the signature chain. The full supported endpoint of the sessions URI is as follows:

```
sessions/verb/noun
```

The minimum required components of the URI are:

```
sessions/verb/noun
```


## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`create`](#create)       - Generate a new session type specified by the noun.   
[`unlock`](#unlock)       - Unlock the session to carry out specified operations.     
[`lock`](#lock)           - Lock the session to stop a specified operations.  
[`save`](#save)           - Save a specified session to the local database.     
[`load`](#load)           - Load a specified session from the local database.     
[`terminate`](#terminate) - Terminates a session specified by the noun.          
[`status`](#status)       - Returns status information for the session specified by the noun.  



## `Supported Nouns`

The following nouns are supported for this API command-set:

[`local`] - The location of the session.

-----------------------------------
## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above verb and noun structure available above.

[`validate/pin`](#validate/pin)

-----------------------------------

# <a name="create"></a> `create`

Create a new session specified by given noun.

```
sessions/create/noun
```

This command only supports the `local` noun.


### Parameters:

`username` : Required to **identify** the profile to create the session for.

`password` : Required to **authenticate** the password for creating the session.

`pin` : Required to **authenticate** the `PIN` for creating the session.


### Results:

#### Return value JSON object:

```
{
    "genesis": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
    "session": "0aad63e028dd9e0f31f0b566831fea9dfc7db68fc2ba482a8ce975656971a67e"
}
[Completed in 1659.509829 ms]
```

#### Return values:

`genesis` : The profile genesis hash. This is a hash of the profile username.

`session` : When using multi-user API mode, an additional session value is returned and must be supplied in subsequent API calls, to allow the managing of multiple login sessions.

-----------------------------------

# <a name="unlock"></a> `unlock`

This will unlock the session specified by given noun and cache the PIN in encrypted memory to be used for all subsequent API calls for the specified operations.

```
sessions/unlock/noun
```

These commands only supports the `local` noun.

### Parameters:

`pin` : Required to **authenticate**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`mining` : Required for **unlocking** mining transactions. This boolean value unlocks the session for mining.

`notifications` : Required for **unlocking** notifications. This boolean value unlocks the session to automatically process incoming notifications.

`staking` : Required for **unlocking** staking transactions. This boolean value unlocks the session for staking.

`transactions` : Required for **unlocking** transactions. This boolean value unlocks the session to allow creating or claiming transactions.

### Returns:

#### Return value JSON object:

```
{
    "unlocked": {
        "mining": false,
        "notifications": true,
        "staking": true,
        "transactions": false
    }
}
[Completed in 1664.238652 ms]
```

#### Return values:

`unlocked` : This will contain child elements describing which functions the session is currently unlocked for.

`mining` : Boolean flag indicating whether the users sig chain is unlocked for mining.

`notifications` : Boolean flag indicating whether the users sig chain is unlocked for processing notifications.

`staking` : Boolean flag indicating whether the users sig chain is unlocked for staking.

`transactions` : Boolean flag indicating whether the users sig chain is unlocked for creating any transactions (except those automatically created through mining/processing notifications if those are unlocked).

-----------------------------------

# <a name="lock"></a> `lock`

This will lock the specified session by purging the PIN stored in encrypted memory, making it unavailable for use unless it is either unlocked or the PIN is passed in to all API requests.

```
sessions/lock/noun
```

These commands only supports the `local` noun.


### Parameters:

`pin` : Required to **authenticate**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.  

`mining` : Required for **locking** mining transactions. This boolean value locks the session for mining.

`notifications` : Required for **locking** notifications. This boolean value locks the session from processing incoming notifications.

`staking` : Required for **locking** staking transactions. This boolean value locks the session for staking.

`transactions : Required for **locking** transactions. This boolean value locks the session for creating or claiming transactions.


### Returns:

#### Return value JSON object:

```
{
    "unlocked": {
        "mining": false,
        "notifications": true,
        "staking": true,
        "transactions": false
    }
}
[Completed in 1664.238652 ms]
```

#### Return values:

`unlocked` : This will contain child elements describing which functions the session is currently unlocked for.

`mining` : Boolean flag indicating whether the users sig chain is unlocked for mining.

`notifications` : Boolean flag indicating whether the users sig chain is unlocked for processing notifications.

`staking` : Boolean flag indicating whether the users sig chain is unlocked for staking.

`transactions` : Boolean flag indicating whether the users sig chain is unlocked for creating any transactions (except those automatically created through mining/processing notifications if those are unlocked).

-----------------------------------

# <a name="save"></a> `save`

This will save the users session to the local database, allowing the session to be resumed at a later time without the need to login or unlock. The users PIN is required as this is used (in conjunction with the genesis) to encrypt the session data before persisting it to the local database.

```
sessions/save/noun
```

These commands only supports the `local` noun.

**NOTE:**
- Save session, saves the specified active session to the local disk.
- Saved sessions are saved permanent until deleted manually.
- Saved sessions will be resumed in the same condition (locked or unlocked) as it was saved.  


### Parameters:

`pin` : Required to **authenticate**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 


### Results:

#### Return value JSON object:

```
{
    "genesis": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
    "success": true
}
[Completed in 2.962459 ms]
```

#### Return values:

`genesis` : The profile genesis hash. This is a hash of the profile username.

`success` : Boolean flag indicating that the session was saved successfully.

-----------------------------------

# <a name="load"></a> `load`

This will resume a saved session from the local database, allowing a saved session to be resumed without the need to login or unlock. The profile username or genesis and PIN is required to decrypt and resume the saved session.

```
sessions/load/noun
```

These commands only supports the `local` noun.

**NOTE:**
- If there is an existing active session, it will not load a saved session. 
- Only a previously saved session can be loaded.  
- When a saved session is resumed, it creates a new session ID.
- Saved sessions can be resumed even after a core or node restart. 


### Parameters:

`pin` : Required if **authenticate**. The `PIN` for this profile.

`username`: Required to **identify** the profile to load the session for. This is optional if the `genesis` is provided.   

`genesis` :  Required to **identify** the profile to load the session for. This is optional if the `username` is provided.    


### Results:

#### Return value JSON object:

```
{
    "genesis": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
    "session": "2ef9de11b19af82984ddf93275e7ba22c11fe9659d0667f79311c46732bbb7a4"
}
[Completed in 0.072625 ms]
```

#### Return values:
`genesis` : The profile genesis hash. This is a hash of the profile username.

`session` : When using multi-user API mode, an additional session value is returned to identify the session.

-----------------------------------

# <a name="terminate"></a> `terminate`

This will terminate an active session specified by given noun.

```
sessions/terminate/noun
```

This command only supports the `local` noun.

**Note:** This does not delete a saved session  

### Parameters:

`pin` : Required to **authenticate**. The `PIN `for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 


### Results:

#### Return value JSON object:

```
{
    "success": true
}
[Completed in 1656.089478 ms]
```

#### Return values:

`success` : Boolean flag indicating that the active session was terminated successfully.

-----------------------------------

# <a name="status"></a> `status`

Retrieve the session status specified by given noun.

```
sessions/status/noun
```

This command only supports the `local` noun.


### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

### Results:

#### Return value JSON object:

```
{
    "genesis": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
    "accessed": 1653680627,
    "location": "local",
    "unlocked": {
        "mining": false,
        "notifications": true,
        "staking": false,
        "transactions": false
    }
}
[Completed in 0.080333 ms]
```

#### Return values:

`genesis` : The profile genesis hash for the currently logged in user.

`accessed` : The UNIX timestamp at which the profile was last accessed.

`location` : The location of session, local or remote.

`unlocked` : This will contain child elements describing which functions the session is currently unlocked for.

`mining` : Boolean flag indicating whether the users sig chain is unlocked for mining.

`notifications` : Boolean flag indicating whether the users profile is unlocked for processing notifications.

`staking` : Boolean flag indicating whether the profile is unlocked for staking.

`transactions` : Boolean flag indicating whether the profile is unlocked for creating any transactions (except those automatically created through mining/processing notifications if those are unlocked).

-----------------------------------

# <a name="validate/pin"></a> `validate/pin`

Validates a pin/string against the current logged in session. Uses the internal authentication logic to attempt to authenticate with the supplied string. This allows for UI layers to gatekeep information without executing a command.  

```
sessions/validate/pin
```

### Parameters:

`pin` : A string to use to attempt to authenticate a command. 

### Results:

#### Return value JSON object:

```
{
   true
}
```
   
#### Return value:

Boolean flag indicating whether the supplied pin would authenticate a command. 

-----------------------------------
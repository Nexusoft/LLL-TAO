# `Commands`

The Nexus Framework contains a powerful **RESTful Application Programming Interface** or **API** that allows developers to easily develop **decentralized applications** or **Dapps** for short. This `API` is broken into a set of commands that are a logically grouped collection of functions. The current command-sets available are as follows:

| Commands   | Description |
| :-----:| :-------- |
| [``ASSETS``](COMMANDS/ASSETS.MD) | Set of commands available for creating and tokenizing `NFT's` |
| [``FINANCE``](COMMANDS/FINANCE.MD) | Set of commands for creating and managing tokens and `NXS` |
| [``INVOICES``](COMMANDS/INVOICES.MD) | Set of commands for issuing and paying invoices on-chain |
| [``LEDGER``](COMMANDS/LEDGER.MD) | Set of low level commands for interacting directly with ledger layer |
| [``MARKET``](COMMANDS/MARKET.MD) | Set of commands for creating and managing `DEFI` markets and `DEX` |
| [``NAMES``](COMMANDS/NAMES.MD) | Set of commands for creating and managing name records for `TNS` |
| [``PROFILES``](COMMANDS/PROFILES.MD) | Set of commands for creating and managing public profiles |
| [``REGISTER``](COMMANDS/REGISTER.MD) | Set of low level commands for interacting directly with register layer |
| [``SESSIONS``](COMMANDS/SESSIONS.MD) | Set of commands for creating and managing login sessions on node |
| [``SUPPLY``](COMMANDS/SUPPLY.MD) | Set of commands for creating and managing supply chains |
| [``SYSTEM``](COMMANDS/SYSTEM.MD) | Set of commands for getting system status and managing core system. |

Click any of the above links to be taken to the core documentation for that command-set, which includes supported verbs, nouns, filters, and operators.

## `Getting Started`

The Nexus API contains a lot of features that will allow you to store, manage, update, or access registers in the register based virtual machine. Some command-sets are global and do not require a login such as `LEDGER`, `REGISTER`, `NETWORK`, and `SYSTEM`. In a general sense, you will find similar syntax for values you wish to override, one such is either `session`, `genesis` or `username`. Let's look at an example of this:

```
ledger/list/transactions genesis=<genesis-id>
```

**or**

```
ledger/list/transactions username=user1
```

The above commands will list the transactions for a particular sigchain without requiring you to be logged in. If you have the login credentials for the sigchain you are getting transactions for, make sure to use a session based mode for efficiency; you will see the equivalent command:

```
profiles/transactions/master session=<session-id>
```

Executes much faster than the ledger version. This is due to **dynamic indexing services** that builds a local index of logged-in sigchains, to access the data without needing to iterate the main chain-state databases. This is important for Reverse-PTR records *(getting the name record from an address)*, register accounting, and managing P2P Marketplace orders. If you don't have a session available when executing the above command, you need to create one:

```
sessions/create/local username=user password=pass1 pin=1234
```

The response will include your session-id if you are in `-multiuser=1` mode, if you are not then you will only receive the genesis-id as a response:

```
{
    "genesis": "b7918d3942a14bd98ceb0f1be4c8675cc833eda354665167140050eee935b1a2",
    "session": "e25c6e8e37e57c573b1c3162fd1acee1d9b4f238fed1b8b921fdcd12196b712b"
}
```

The completion time will generally be around 900-1500ms to respond. This operation is very computationally intensive and requires at least 64MB of memory to generate the required keys to check against the profile's crypto object register as a measure of brute-force resistance. If you were unable to create a session because it does not exist, you need to generate a master profile first:

```
profiles/create/master username=user password=pass1 pin=1234
```

After which you will be able to execute `sessions/create/local` to create a session to invoke any of the *session-required* command-sets.

## `Making a Call`

In order to invoke this URI endpoint, you need to ensure you are connecting to the correct host. You can find all the details including how to setup SSL, on the  [``PROTOCOL``](PROTOCOL.MD) page. But to make your first call, it's easiest to use the *command-line interface* (CLI):

```
./nexus profiles/create/master username=user password=pass1 pin=1234
```

The above shows how to use the CLI to make a call to your local daemon. In order for this to work, you need to make sure your daemon is configured with `apiauth=0` or `-noapiauth` if you have not defined credentials with `apiuser=<user>` and `apipassword=<password>`. Without either of these options, it will assume this is a relay node and not turn on the API server and you will see this warning message in daemon logs on startup:

```
NOTICE: API SERVER DISABLED
NOTICE: You must set apiuser=<user> and apipassword=<password> configuration.
NOTICE: If you intend to run the API server without authenticating, set apiauth=0
```

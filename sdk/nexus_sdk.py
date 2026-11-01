#!/usr/bin/env python3
#
# nexus_sdk.py
#
# Nexus Python SDK for interfacing with the Nexus Tritium++ APIs.
# Updated for Python 3 and current API structure (v6.0+).
#
# This file produces sdk/nexus_sdk.txt using the pydoc tool. To run pydoc:
#
#    python3 -m pydoc nexus_sdk > ./nexus_sdk.txt
#
# -----------------------------------------------------------------------------

"""
The Nexus Python SDK supports the following Nexus API calls.

System APIs:

System API:               Ledger API:                Network API:
-----------               -----------                ------------
system/get/info           ledger/get/blockhash       network/list/node
system/get/metrics        ledger/get/block           network/list/peer
system/stop               ledger/get/info
system/list/peers         ledger/get/metrics
system/list/lisp-eids     ledger/get/transaction
system/validate/address   ledger/list/blocks
                          ledger/list/transactions
                          ledger/recent/blocks
                          ledger/submit/transaction
                          ledger/sync/status

Session/Profile APIs:

Sessions API:             Profiles API:
-------------             -------------
sessions/create           profiles/create
sessions/load             profiles/notifications
sessions/save             profiles/list
sessions/lock             profiles/recover
sessions/unlock           profiles/status
sessions/status           profiles/transactions
sessions/terminate        profiles/update
sessions/validate/pin

Finance APIs:

Finance API:              Tokens API (deprecated):
------------              -----------------------
finance/create            Use finance/* instead
finance/burn
finance/credit
finance/debit
finance/get
finance/list
finance/history
finance/transactions
finance/get/balances
finance/get/stakeinfo
finance/set/stake
finance/void/transaction

Object APIs:

Assets API:               Supply API:               Names API:
-----------               -----------               ----------
assets/claim              supply/claim              names/claim
assets/create             supply/create             names/create
assets/get                supply/get                names/get
assets/history            supply/history            names/history
assets/list               supply/list               names/list
assets/list/partial       supply/transactions       names/rename
assets/verify/partial     supply/transfer           names/reverse/lookup
assets/tokenize           supply/update             names/transactions
assets/transactions                                 names/transfer
assets/transfer                                     names/update
assets/update

Register API:             Market API:               Invoices API:
-------------             -----------               -------------
register/get              market/create/bid         invoices/create
register/list             market/create/ask         invoices/get
register/history          market/list/bid           invoices/pay
register/transactions     market/list/ask           invoices/cancel
                          market/list/order         invoices/reject
                          market/list/executed      invoices/list
                          market/execute/bid        invoices/history
                          market/execute/ask        invoices/transactions
                          market/execute/order
                          market/cancel/bid
                          market/cancel/ask
                          market/cancel/order
                          market/user/order
                          market/user/executed

Local API:
----------
local/erase/record
local/has/record
local/list/record
local/push/record

"""

# -----------------------------------------------------------------------------

import urllib.request
import urllib.parse
import urllib.error
import json
import ssl
from typing import Optional, Dict, Any, Union

# Default configuration
SDK_DEFAULT_URL = "http://localhost:8080"
sdk_url = SDK_DEFAULT_URL

# API endpoint templates
SYSTEM_URL = "{}/system/{}"
LEDGER_URL = "{}/ledger/{}"
NETWORK_URL = "{}/network/{}"
SESSIONS_URL = "{}/sessions/{}"
PROFILES_URL = "{}/profiles/{}"
FINANCE_URL = "{}/finance/{}"
TOKENS_URL = "{}/tokens/{}"
ASSETS_URL = "{}/assets/{}"
SUPPLY_URL = "{}/supply/{}"
NAMES_URL = "{}/names/{}"
REGISTER_URL = "{}/register/{}"
MARKET_URL = "{}/market/{}"
INVOICES_URL = "{}/invoices/{}"
LOCAL_URL = "{}/local/{}"

# -----------------------------------------------------------------------------


def sdk_change_url(url: str = SDK_DEFAULT_URL) -> bool:
    """
    Change the URL that the SDK uses.
    
    Args:
        url: The URL formatted with http/https, e.g., "http://localhost:8080"
    
    Returns:
        True if URL was valid and set, False otherwise.
    """
    global sdk_url

    if not isinstance(url, str):
        return False

    if not url.startswith(("http://", "https://")):
        return False
    
    # Validate port exists
    parts = url.split(":")
    if len(parts) < 3:
        return False
    
    port = parts[-1].rstrip("/")
    if not port.isdigit():
        return False

    sdk_url = url.rstrip("/")
    return True


# Initialize with default URL
sdk_change_url()


class NexusSDK:
    """
    Nexus SDK class for interacting with the Nexus Tritium++ API.
    
    This class provides methods for all Nexus API endpoints including
    sessions, profiles, finance, assets, supply, market, and more.
    """
    
    def __init__(self, username: str = "", password: str = "", pin: str = "",
                 timeout: int = 30, verify_ssl: bool = True):
        """
        Initialize the SDK with optional user credentials.
        
        Args:
            username: The username for the Nexus account
            password: The password for the Nexus account  
            pin: The PIN for the Nexus account
            timeout: Request timeout in seconds (default 30)
            verify_ssl: Whether to verify SSL certificates (default True)
        """
        self.username = username
        self.password = password
        self.pin = pin
        self.session_id: Optional[str] = None
        self.genesis_id: Optional[str] = None
        self.timeout = timeout
        self.verify_ssl = verify_ssl
    
    # =========================================================================
    # SYSTEM API
    # =========================================================================
    
    def system_get_info(self) -> Dict[str, Any]:
        """
        Get system information about the Nexus node. Login not required.
        
        Returns:
            JSON response with node information including version, 
            protocol version, timestamp, hostname, etc.
        """
        url = SYSTEM_URL.format(sdk_url, "get/info")
        return self._get(url)
    
    def system_get_metrics(self) -> Dict[str, Any]:
        """
        Get system metrics for the Nexus node. Login not required.
        
        Returns:
            JSON response with system metrics.
        """
        url = SYSTEM_URL.format(sdk_url, "get/metrics")
        return self._get(url)
    
    def system_stop(self, password: Optional[str] = None) -> Dict[str, Any]:
        """
        Stop the Nexus server.
        
        Args:
            password: Optional password if -system/stop config is set
            
        Returns:
            JSON response confirming shutdown.
        """
        params = {}
        if password:
            params["password"] = password
        url = SYSTEM_URL.format(sdk_url, "stop")
        return self._get(url, params)
    
    def system_list_peers(self) -> Dict[str, Any]:
        """
        List established Nexus peer connections. Login not required.
        
        Returns:
            JSON response with list of connected peers.
        """
        url = SYSTEM_URL.format(sdk_url, "list/peers")
        return self._get(url)
    
    def system_list_lisp_eids(self) -> Dict[str, Any]:
        """
        List configured LISP EIDs for this node. Login not required.
        
        Returns:
            JSON response with LISP EID information.
        """
        url = SYSTEM_URL.format(sdk_url, "list/lisp-eids")
        return self._get(url)
    
    def system_validate_address(self, address: str) -> Dict[str, Any]:
        """
        Validate a Nexus register address.
        
        Args:
            address: The register address to validate
            
        Returns:
            JSON response with validation result.
        """
        url = SYSTEM_URL.format(sdk_url, "validate/address")
        return self._get(url, {"address": address})
    
    # =========================================================================
    # LEDGER API
    # =========================================================================
    
    def ledger_get_blockhash(self, height: int) -> Dict[str, Any]:
        """
        Retrieve the block hash for a block at the specified height.
        
        Args:
            height: The block height
            
        Returns:
            JSON response with block hash.
        """
        url = LEDGER_URL.format(sdk_url, "get/blockhash")
        return self._get(url, {"height": height})
    
    def ledger_get_block(self, block_hash: Optional[str] = None,
                         height: Optional[int] = None,
                         verbose: str = "default") -> Dict[str, Any]:
        """
        Retrieve a block by hash or height.
        
        Args:
            block_hash: The block hash (optional if height provided)
            height: The block height (optional if hash provided)
            verbose: Verbosity level - "default", "summary", "detail"
            
        Returns:
            JSON response with block data.
        """
        params = {"verbose": verbose}
        if block_hash:
            params["hash"] = block_hash
        elif height is not None:
            params["height"] = height
        
        url = LEDGER_URL.format(sdk_url, "get/block")
        return self._get(url, params)
    
    def ledger_get_info(self) -> Dict[str, Any]:
        """
        Get ledger/blockchain information.
        
        Returns:
            JSON response with ledger info (replaces get/mininginfo).
        """
        url = LEDGER_URL.format(sdk_url, "get/info")
        return self._get(url)
    
    def ledger_get_metrics(self) -> Dict[str, Any]:
        """
        Get ledger metrics.
        
        Returns:
            JSON response with ledger metrics.
        """
        url = LEDGER_URL.format(sdk_url, "get/metrics")
        return self._get(url)
    
    def ledger_get_transaction(self, txid: str, 
                                verbose: str = "default") -> Dict[str, Any]:
        """
        Retrieve a transaction by its hash.
        
        Args:
            txid: The transaction hash
            verbose: Verbosity level
            
        Returns:
            JSON response with transaction data.
        """
        url = LEDGER_URL.format(sdk_url, "get/transaction")
        return self._get(url, {"hash": txid, "verbose": verbose})
    
    def ledger_list_blocks(self, block_hash: Optional[str] = None,
                           height: Optional[int] = None,
                           limit: int = 10,
                           verbose: str = "default") -> Dict[str, Any]:
        """
        List blocks starting from a hash or height.
        
        Args:
            block_hash: Starting block hash
            height: Starting block height
            limit: Number of blocks to return
            verbose: Verbosity level
            
        Returns:
            JSON response with list of blocks.
        """
        params = {"limit": limit, "verbose": verbose}
        if block_hash:
            params["hash"] = block_hash
        elif height is not None:
            params["height"] = height
            
        url = LEDGER_URL.format(sdk_url, "list/blocks")
        return self._get(url, params)
    
    def ledger_list_transactions(self, page: int = 0, 
                                  limit: int = 100) -> Dict[str, Any]:
        """
        List transactions from the mempool.
        
        Args:
            page: Page offset
            limit: Number of transactions to return
            
        Returns:
            JSON response with transaction list.
        """
        url = LEDGER_URL.format(sdk_url, "list/transactions")
        return self._get(url, {"page": page, "limit": limit})
    
    def ledger_recent_blocks(self, limit: int = 10) -> Dict[str, Any]:
        """
        Get the most recent blocks.
        
        Args:
            limit: Number of recent blocks to return
            
        Returns:
            JSON response with recent blocks.
        """
        url = LEDGER_URL.format(sdk_url, "recent/blocks")
        return self._get(url, {"limit": limit})
    
    def ledger_submit_transaction(self, tx_data: str) -> Dict[str, Any]:
        """
        Submit a transaction to the network.
        
        Args:
            tx_data: The serialized transaction data
            
        Returns:
            JSON response with submission result.
        """
        url = LEDGER_URL.format(sdk_url, "submit/transaction")
        return self._get(url, {"data": tx_data})
    
    def ledger_sync_status(self) -> Dict[str, Any]:
        """
        Get the synchronization status of the node.
        
        Returns:
            JSON response with sync status.
        """
        url = LEDGER_URL.format(sdk_url, "sync/status")
        return self._get(url)
    
    # =========================================================================
    # NETWORK API
    # =========================================================================
    
    def network_list_nodes(self, page: int = 0, 
                           limit: int = 100) -> Dict[str, Any]:
        """
        List network nodes.
        
        Args:
            page: Page offset
            limit: Number of nodes to return
            
        Returns:
            JSON response with node list.
        """
        url = NETWORK_URL.format(sdk_url, "list/node")
        return self._get(url, {"page": page, "limit": limit})
    
    def network_list_peers(self, page: int = 0,
                           limit: int = 100) -> Dict[str, Any]:
        """
        List connected peers.
        
        Args:
            page: Page offset
            limit: Number of peers to return
            
        Returns:
            JSON response with peer list.
        """
        url = NETWORK_URL.format(sdk_url, "list/peer")
        return self._get(url, {"page": page, "limit": limit})
    
    # =========================================================================
    # SESSIONS API (replaces users login/logout)
    # =========================================================================
    
    def sessions_create(self, username: Optional[str] = None,
                        password: Optional[str] = None,
                        pin: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a new session (login) for a user.
        
        Args:
            username: Username (uses instance username if not provided)
            password: Password (uses instance password if not provided)
            pin: PIN (uses instance pin if not provided)
            
        Returns:
            JSON response with session information including session ID.
        """
        username = username or self.username
        password = password or self.password
        pin = pin or self.pin
        
        params = {
            "username": username,
            "password": password,
            "pin": pin
        }
        
        url = SESSIONS_URL.format(sdk_url, "create/local")
        result = self._post(url, params)
        
        if "result" in result:
            if "genesis" in result["result"]:
                self.genesis_id = result["result"]["genesis"]
            if "session" in result["result"]:
                self.session_id = result["result"]["session"]
        
        return result
    
    def sessions_load(self, username: Optional[str] = None,
                      password: Optional[str] = None,
                      pin: Optional[str] = None) -> Dict[str, Any]:
        """
        Load a previously saved session.
        
        Args:
            username: Username
            password: Password
            pin: PIN
            
        Returns:
            JSON response with session information.
        """
        username = username or self.username
        password = password or self.password
        pin = pin or self.pin
        
        params = {
            "username": username,
            "password": password,
            "pin": pin
        }
        
        url = SESSIONS_URL.format(sdk_url, "load/local")
        result = self._post(url, params)
        
        if "result" in result:
            if "genesis" in result["result"]:
                self.genesis_id = result["result"]["genesis"]
            if "session" in result["result"]:
                self.session_id = result["result"]["session"]
        
        return result
    
    def sessions_save(self) -> Dict[str, Any]:
        """
        Save the current session to disk.
        
        Returns:
            JSON response confirming save.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        url = SESSIONS_URL.format(sdk_url, "save/local")
        return self._get(url, {"session": self.session_id})
    
    def sessions_terminate(self) -> Dict[str, Any]:
        """
        Terminate the current session (logout).
        
        Returns:
            JSON response confirming logout.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {"session": self.session_id}
        url = SESSIONS_URL.format(sdk_url, "terminate/local")
        result = self._get(url, params)
        
        if "error" not in result:
            self.session_id = None
        
        return result
    
    def sessions_lock(self, minting: int = 0, 
                      transactions: int = 0) -> Dict[str, Any]:
        """
        Lock the current session.
        
        Args:
            minting: 1 to keep minting enabled while locked
            transactions: 1 to keep transactions enabled while locked
            
        Returns:
            JSON response confirming lock.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "minting": minting,
            "transactions": transactions
        }
        
        url = SESSIONS_URL.format(sdk_url, "lock/local")
        return self._get(url, params)
    
    def sessions_unlock(self, pin: Optional[str] = None,
                        minting: int = 0,
                        transactions: int = 0) -> Dict[str, Any]:
        """
        Unlock the current session.
        
        Args:
            pin: PIN to unlock (uses instance pin if not provided)
            minting: 1 to enable minting
            transactions: 1 to enable transactions
            
        Returns:
            JSON response confirming unlock.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": pin or self.pin,
            "minting": minting,
            "transactions": transactions
        }
        
        url = SESSIONS_URL.format(sdk_url, "unlock/local")
        return self._get(url, params)
    
    def sessions_status(self) -> Dict[str, Any]:
        """
        Get the status of the current session.
        
        Returns:
            JSON response with session status.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        url = SESSIONS_URL.format(sdk_url, "status/local")
        return self._get(url, {"session": self.session_id})
    
    def sessions_validate_pin(self, pin: Optional[str] = None) -> Dict[str, Any]:
        """
        Validate the PIN for the current session.
        
        Args:
            pin: PIN to validate
            
        Returns:
            JSON response with validation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": pin or self.pin
        }
        
        url = SESSIONS_URL.format(sdk_url, "validate/pin")
        return self._get(url, params)
    
    # =========================================================================
    # PROFILES API (replaces users create/account management)
    # =========================================================================
    
    def profiles_create(self, username: Optional[str] = None,
                        password: Optional[str] = None,
                        pin: Optional[str] = None,
                        recovery: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a new user profile (signature chain).
        
        Args:
            username: Username for the new profile
            password: Password for the new profile
            pin: PIN for the new profile
            recovery: Optional recovery phrase
            
        Returns:
            JSON response with new profile information.
        """
        params = {
            "username": username or self.username,
            "password": password or self.password,
            "pin": pin or self.pin
        }
        if recovery:
            params["recovery"] = recovery
        
        url = PROFILES_URL.format(sdk_url, "create/master")
        return self._post(url, params)
    
    def profiles_notifications(self, page: int = 0,
                               limit: int = 100) -> Dict[str, Any]:
        """
        List notifications for the current profile.
        
        Args:
            page: Page offset
            limit: Number of notifications to return
            
        Returns:
            JSON response with notifications list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = PROFILES_URL.format(sdk_url, "notifications")
        return self._get(url, params)
    
    def profiles_list(self, noun: str = "register",
                      page: int = 0,
                      limit: int = 100) -> Dict[str, Any]:
        """
        List registers owned by the current profile.
        
        Args:
            noun: Type of register to list
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with register list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = PROFILES_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def profiles_recover(self, username: str, recovery: str,
                         password: str, pin: str) -> Dict[str, Any]:
        """
        Recover a profile using the recovery phrase.
        
        Args:
            username: Username to recover
            recovery: Recovery phrase
            password: New password
            pin: New PIN
            
        Returns:
            JSON response with recovery result.
        """
        params = {
            "username": username,
            "recovery": recovery,
            "password": password,
            "pin": pin
        }
        
        url = PROFILES_URL.format(sdk_url, "recover/master")
        return self._post(url, params)
    
    def profiles_status(self) -> Dict[str, Any]:
        """
        Get the status of the current profile.
        
        Returns:
            JSON response with profile status.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        url = PROFILES_URL.format(sdk_url, "status/master")
        return self._get(url, {"session": self.session_id})
    
    def profiles_transactions(self, page: int = 0,
                              limit: int = 100,
                              verbose: str = "default") -> Dict[str, Any]:
        """
        List transactions for the current profile.
        
        Args:
            page: Page offset
            limit: Number of transactions to return
            verbose: Verbosity level
            
        Returns:
            JSON response with transaction list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit,
            "verbose": verbose
        }
        
        url = PROFILES_URL.format(sdk_url, "transactions")
        return self._get(url, params)
    
    def profiles_update(self, update_type: str = "credentials",
                        password: Optional[str] = None,
                        pin: Optional[str] = None,
                        new_password: Optional[str] = None,
                        new_pin: Optional[str] = None,
                        recovery: Optional[str] = None,
                        new_recovery: Optional[str] = None) -> Dict[str, Any]:
        """
        Update profile credentials or recovery phrase.
        
        Args:
            update_type: "credentials" or "recovery"
            password: Current password
            pin: Current PIN
            new_password: New password (for credentials update)
            new_pin: New PIN (for credentials update)
            recovery: Current recovery phrase (for recovery update)
            new_recovery: New recovery phrase
            
        Returns:
            JSON response with update result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {"session": self.session_id}
        
        if update_type == "credentials":
            params["password"] = password or self.password
            params["pin"] = pin or self.pin
            if new_password:
                params["new_password"] = new_password
            if new_pin:
                params["new_pin"] = new_pin
        elif update_type == "recovery":
            params["recovery"] = recovery
            if new_recovery:
                params["new_recovery"] = new_recovery
        
        url = PROFILES_URL.format(sdk_url, f"update/{update_type}")
        return self._post(url, params)
    
    # =========================================================================
    # FINANCE API
    # =========================================================================
    
    def finance_create(self, noun: str, name: str,
                       token: Optional[str] = None,
                       supply: Optional[int] = None,
                       decimals: int = 2,
                       **kwargs) -> Dict[str, Any]:
        """
        Create a finance object (account, token, or trust).
        
        Args:
            noun: Type to create - "account", "token", or "trust"
            name: Name for the new object
            token: Token identifier (for accounts)
            supply: Initial supply (for tokens)
            decimals: Decimal places (for tokens)
            **kwargs: Additional parameters
            
        Returns:
            JSON response with creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "name": name
        }
        
        if token:
            params["token"] = token
        if supply is not None:
            params["supply"] = supply
            params["decimals"] = decimals
        
        params.update(kwargs)
        
        url = FINANCE_URL.format(sdk_url, f"create/{noun}")
        return self._post(url, params)
    
    def finance_burn(self, name: Optional[str] = None,
                     address: Optional[str] = None,
                     amount: float = 0) -> Dict[str, Any]:
        """
        Burn tokens from a token register.
        
        Args:
            name: Name of the token
            address: Address of the token
            amount: Amount to burn
            
        Returns:
            JSON response with burn result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "amount": amount
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = FINANCE_URL.format(sdk_url, "burn/token")
        return self._post(url, params)
    
    def finance_credit(self, txid: str, 
                       name: Optional[str] = None,
                       address: Optional[str] = None) -> Dict[str, Any]:
        """
        Credit (claim) a pending debit transaction.
        
        Args:
            txid: Transaction ID of the debit to credit
            name: Name of the account to credit
            address: Address of the account to credit
            
        Returns:
            JSON response with credit result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = FINANCE_URL.format(sdk_url, "credit")
        return self._post(url, params)
    
    def finance_debit(self, amount: float,
                      name_from: Optional[str] = None,
                      address_from: Optional[str] = None,
                      name_to: Optional[str] = None,
                      address_to: Optional[str] = None,
                      reference: Optional[str] = None) -> Dict[str, Any]:
        """
        Debit (send) from an account.
        
        Args:
            amount: Amount to send
            name_from: Name of source account
            address_from: Address of source account
            name_to: Name of destination account
            address_to: Address of destination
            reference: Optional reference number
            
        Returns:
            JSON response with debit result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "amount": amount
        }
        
        if name_from:
            params["name"] = name_from
        elif address_from:
            params["address"] = address_from
        
        if name_to:
            params["name_to"] = name_to
        elif address_to:
            params["address_to"] = address_to
        
        if reference:
            params["reference"] = reference
        
        url = FINANCE_URL.format(sdk_url, "debit")
        return self._post(url, params)
    
    def finance_get(self, noun: str,
                    name: Optional[str] = None,
                    address: Optional[str] = None) -> Dict[str, Any]:
        """
        Get a finance object.
        
        Args:
            noun: Type to get - "account", "token", "trust", "any"
            name: Name of the object
            address: Address of the object
            
        Returns:
            JSON response with object data.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = FINANCE_URL.format(sdk_url, f"get/{noun}")
        return self._get(url, params)
    
    def finance_list(self, noun: str = "any",
                     page: int = 0,
                     limit: int = 100) -> Dict[str, Any]:
        """
        List finance objects.
        
        Args:
            noun: Type to list - "any", "account", "token", "trust"
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with object list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = FINANCE_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def finance_history(self, noun: str,
                        name: Optional[str] = None,
                        address: Optional[str] = None,
                        page: int = 0,
                        limit: int = 100) -> Dict[str, Any]:
        """
        Get history for a finance object.
        
        Args:
            noun: Type - "account", "token", "trust"
            name: Name of the object
            address: Address of the object
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = FINANCE_URL.format(sdk_url, f"history/{noun}")
        return self._get(url, params)
    
    def finance_transactions(self, noun: str,
                             name: Optional[str] = None,
                             address: Optional[str] = None,
                             page: int = 0,
                             limit: int = 100) -> Dict[str, Any]:
        """
        Get transactions for a finance object.
        
        Args:
            noun: Type - "account", "token", "trust"
            name: Name of the object
            address: Address of the object
            page: Page offset
            limit: Number of transactions to return
            
        Returns:
            JSON response with transactions.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = FINANCE_URL.format(sdk_url, f"transactions/{noun}")
        return self._get(url, params)
    
    def finance_get_balances(self) -> Dict[str, Any]:
        """
        Get balances for all accounts.
        
        Returns:
            JSON response with account balances.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        url = FINANCE_URL.format(sdk_url, "get/balances")
        return self._get(url, {"session": self.session_id})
    
    def finance_get_stakeinfo(self) -> Dict[str, Any]:
        """
        Get staking information for the current user.
        
        Returns:
            JSON response with staking information.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        url = FINANCE_URL.format(sdk_url, "get/stakeinfo")
        return self._get(url, {"session": self.session_id})
    
    def finance_set_stake(self, amount: float) -> Dict[str, Any]:
        """
        Set the stake amount.
        
        Args:
            amount: Amount to stake
            
        Returns:
            JSON response with stake result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "amount": amount
        }
        
        url = FINANCE_URL.format(sdk_url, "set/stake")
        return self._post(url, params)
    
    def finance_void_transaction(self, txid: str) -> Dict[str, Any]:
        """
        Void a pending transaction.
        
        Args:
            txid: Transaction ID to void
            
        Returns:
            JSON response with void result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        url = FINANCE_URL.format(sdk_url, "void/transaction")
        return self._post(url, params)
    
    # =========================================================================
    # ASSETS API
    # =========================================================================
    
    def assets_create(self, name: str, data: Union[str, dict],
                      format: str = "raw") -> Dict[str, Any]:
        """
        Create a new asset.
        
        Args:
            name: Name for the asset
            data: Asset data (string or dict for JSON format)
            format: Format - "raw", "readonly", "basic", "json"
            
        Returns:
            JSON response with creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "name": name,
            "format": format
        }
        
        if isinstance(data, dict):
            params["json"] = json.dumps(data)
        else:
            params["data"] = data
        
        url = ASSETS_URL.format(sdk_url, "create/asset")
        return self._post(url, params)
    
    def assets_get(self, name: Optional[str] = None,
                   address: Optional[str] = None) -> Dict[str, Any]:
        """
        Get an asset.
        
        Args:
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with asset data.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "get/asset")
        return self._get(url, params)
    
    def assets_update(self, data: Union[str, dict],
                      name: Optional[str] = None,
                      address: Optional[str] = None) -> Dict[str, Any]:
        """
        Update an asset.
        
        Args:
            data: New data for the asset
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with update result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        if isinstance(data, dict):
            params["json"] = json.dumps(data)
        else:
            params["data"] = data
        
        url = ASSETS_URL.format(sdk_url, "update/asset")
        return self._post(url, params)
    
    def assets_transfer(self, destination: str,
                        name: Optional[str] = None,
                        address: Optional[str] = None) -> Dict[str, Any]:
        """
        Transfer asset ownership.
        
        Args:
            destination: Destination username or address
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with transfer result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "destination": destination
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "transfer/asset")
        return self._post(url, params)
    
    def assets_claim(self, txid: str) -> Dict[str, Any]:
        """
        Claim a transferred asset.
        
        Args:
            txid: Transfer transaction ID
            
        Returns:
            JSON response with claim result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        url = ASSETS_URL.format(sdk_url, "claim/asset")
        return self._post(url, params)
    
    def assets_tokenize(self, token: str,
                        name: Optional[str] = None,
                        address: Optional[str] = None) -> Dict[str, Any]:
        """
        Tokenize an asset.
        
        Args:
            token: Token name or address to tokenize with
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with tokenization result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "token": token
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "tokenize/asset")
        return self._post(url, params)
    
    def assets_list(self, noun: str = "asset",
                    page: int = 0,
                    limit: int = 100) -> Dict[str, Any]:
        """
        List assets.
        
        Args:
            noun: Type - "asset", "raw", "readonly", "any"
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with asset list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = ASSETS_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def assets_history(self, name: Optional[str] = None,
                       address: Optional[str] = None,
                       page: int = 0,
                       limit: int = 100) -> Dict[str, Any]:
        """
        Get asset history.
        
        Args:
            name: Asset name
            address: Asset address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "history/asset")
        return self._get(url, params)
    
    def assets_transactions(self, name: Optional[str] = None,
                            address: Optional[str] = None,
                            page: int = 0,
                            limit: int = 100) -> Dict[str, Any]:
        """
        Get asset transactions.
        
        Args:
            name: Asset name
            address: Asset address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with transactions.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "transactions/asset")
        return self._get(url, params)
    
    def assets_list_partial(self, name: Optional[str] = None,
                            address: Optional[str] = None) -> Dict[str, Any]:
        """
        List partial ownership of a tokenized asset.
        
        Args:
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with partial ownership info.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "list/partial")
        return self._get(url, params)
    
    def assets_verify_partial(self, name: Optional[str] = None,
                              address: Optional[str] = None) -> Dict[str, Any]:
        """
        Verify partial ownership of a tokenized asset.
        
        Args:
            name: Asset name
            address: Asset address
            
        Returns:
            JSON response with verification result.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = ASSETS_URL.format(sdk_url, "verify/partial")
        return self._get(url, params)
    
    # =========================================================================
    # SUPPLY API
    # =========================================================================
    
    def supply_create(self, name: str, data: Union[str, dict],
                      format: str = "raw") -> Dict[str, Any]:
        """
        Create a new supply chain item.
        
        Args:
            name: Name for the item
            data: Item data
            format: Format - "raw", "readonly", "basic", "json"
            
        Returns:
            JSON response with creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "name": name,
            "format": format
        }
        
        if isinstance(data, dict):
            params["json"] = json.dumps(data)
        else:
            params["data"] = data
        
        url = SUPPLY_URL.format(sdk_url, "create/item")
        return self._post(url, params)
    
    def supply_get(self, name: Optional[str] = None,
                   address: Optional[str] = None) -> Dict[str, Any]:
        """
        Get a supply chain item.
        
        Args:
            name: Item name
            address: Item address
            
        Returns:
            JSON response with item data.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = SUPPLY_URL.format(sdk_url, "get/item")
        return self._get(url, params)
    
    def supply_update(self, data: Union[str, dict],
                      name: Optional[str] = None,
                      address: Optional[str] = None) -> Dict[str, Any]:
        """
        Update a supply chain item.
        
        Args:
            data: New data for the item
            name: Item name
            address: Item address
            
        Returns:
            JSON response with update result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        if isinstance(data, dict):
            params["json"] = json.dumps(data)
        else:
            params["data"] = data
        
        url = SUPPLY_URL.format(sdk_url, "update/item")
        return self._post(url, params)
    
    def supply_transfer(self, destination: str,
                        name: Optional[str] = None,
                        address: Optional[str] = None) -> Dict[str, Any]:
        """
        Transfer a supply chain item.
        
        Args:
            destination: Destination username or address
            name: Item name
            address: Item address
            
        Returns:
            JSON response with transfer result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "destination": destination
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = SUPPLY_URL.format(sdk_url, "transfer/item")
        return self._post(url, params)
    
    def supply_claim(self, txid: str) -> Dict[str, Any]:
        """
        Claim a transferred supply chain item.
        
        Args:
            txid: Transfer transaction ID
            
        Returns:
            JSON response with claim result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        url = SUPPLY_URL.format(sdk_url, "claim/item")
        return self._post(url, params)
    
    def supply_list(self, noun: str = "item",
                    page: int = 0,
                    limit: int = 100) -> Dict[str, Any]:
        """
        List supply chain items.
        
        Args:
            noun: Type - "item", "raw", "readonly", "any"
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with item list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = SUPPLY_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def supply_history(self, name: Optional[str] = None,
                       address: Optional[str] = None,
                       page: int = 0,
                       limit: int = 100) -> Dict[str, Any]:
        """
        Get supply chain item history.
        
        Args:
            name: Item name
            address: Item address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = SUPPLY_URL.format(sdk_url, "history/item")
        return self._get(url, params)
    
    def supply_transactions(self, name: Optional[str] = None,
                            address: Optional[str] = None,
                            page: int = 0,
                            limit: int = 100) -> Dict[str, Any]:
        """
        Get supply chain item transactions.
        
        Args:
            name: Item name
            address: Item address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with transactions.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = SUPPLY_URL.format(sdk_url, "transactions/item")
        return self._get(url, params)
    
    # =========================================================================
    # NAMES API
    # =========================================================================
    
    def names_create(self, noun: str, name: str,
                     register_address: Optional[str] = None,
                     namespace: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a name or namespace.
        
        Args:
            noun: Type - "name", "namespace", "global"
            name: The name to create
            register_address: Address the name points to (for names)
            namespace: Namespace for the name
            
        Returns:
            JSON response with creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "name": name
        }
        
        if register_address:
            params["register_address"] = register_address
        if namespace:
            params["namespace"] = namespace
        
        url = NAMES_URL.format(sdk_url, f"create/{noun}")
        return self._post(url, params)
    
    def names_get(self, noun: str = "name",
                  name: Optional[str] = None,
                  address: Optional[str] = None) -> Dict[str, Any]:
        """
        Get a name or namespace.
        
        Args:
            noun: Type - "name", "namespace"
            name: The name
            address: The address
            
        Returns:
            JSON response with name data.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = NAMES_URL.format(sdk_url, f"get/{noun}")
        return self._get(url, params)
    
    def names_list(self, noun: str = "name",
                   page: int = 0,
                   limit: int = 100) -> Dict[str, Any]:
        """
        List names or namespaces.
        
        Args:
            noun: Type - "name", "namespace", "local", "global", "any"
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with name list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = NAMES_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def names_rename(self, old_name: str, new_name: str) -> Dict[str, Any]:
        """
        Rename a local name.
        
        Args:
            old_name: Current name
            new_name: New name
            
        Returns:
            JSON response with rename result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "name": old_name,
            "new_name": new_name
        }
        
        url = NAMES_URL.format(sdk_url, "rename/name")
        return self._post(url, params)
    
    def names_update(self, register_address: str,
                     name: Optional[str] = None,
                     address: Optional[str] = None) -> Dict[str, Any]:
        """
        Update a name to point to a different register.
        
        Args:
            register_address: New register address
            name: Name to update
            address: Address of the name register
            
        Returns:
            JSON response with update result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "register_address": register_address
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = NAMES_URL.format(sdk_url, "update/name")
        return self._post(url, params)
    
    def names_transfer(self, destination: str,
                       name: Optional[str] = None,
                       address: Optional[str] = None) -> Dict[str, Any]:
        """
        Transfer a name.
        
        Args:
            destination: Destination username or address
            name: Name to transfer
            address: Address of the name
            
        Returns:
            JSON response with transfer result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "destination": destination
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = NAMES_URL.format(sdk_url, "transfer/name")
        return self._post(url, params)
    
    def names_claim(self, txid: str) -> Dict[str, Any]:
        """
        Claim a transferred name.
        
        Args:
            txid: Transfer transaction ID
            
        Returns:
            JSON response with claim result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        url = NAMES_URL.format(sdk_url, "claim/name")
        return self._post(url, params)
    
    def names_reverse_lookup(self, address: str) -> Dict[str, Any]:
        """
        Reverse lookup an address to find its name.
        
        Args:
            address: The register address
            
        Returns:
            JSON response with name information.
        """
        url = NAMES_URL.format(sdk_url, "reverse/lookup")
        return self._get(url, {"address": address})
    
    def names_history(self, name: Optional[str] = None,
                      address: Optional[str] = None,
                      page: int = 0,
                      limit: int = 100) -> Dict[str, Any]:
        """
        Get name history.
        
        Args:
            name: The name
            address: The address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = NAMES_URL.format(sdk_url, "history/name")
        return self._get(url, params)
    
    def names_transactions(self, name: Optional[str] = None,
                           address: Optional[str] = None,
                           page: int = 0,
                           limit: int = 100) -> Dict[str, Any]:
        """
        Get name transactions.
        
        Args:
            name: The name
            address: The address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with transactions.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = NAMES_URL.format(sdk_url, "transactions/name")
        return self._get(url, params)
    
    # =========================================================================
    # REGISTER API
    # =========================================================================
    
    def register_get(self, address: str) -> Dict[str, Any]:
        """
        Get any register by address.
        
        Args:
            address: Register address
            
        Returns:
            JSON response with register data.
        """
        params = {"address": address}
        
        if self.session_id:
            params["session"] = self.session_id
        
        url = REGISTER_URL.format(sdk_url, "get/any")
        return self._get(url, params)
    
    def register_list(self, noun: str = "any",
                      page: int = 0,
                      limit: int = 100,
                      where: Optional[str] = None) -> Dict[str, Any]:
        """
        List registers.
        
        Args:
            noun: Type - "any", "object", "raw", "readonly", "crypto"
            page: Page offset
            limit: Number of items to return
            where: Optional WHERE clause for filtering
            
        Returns:
            JSON response with register list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        if where:
            params["where"] = where
        
        url = REGISTER_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def register_history(self, address: str,
                         page: int = 0,
                         limit: int = 100) -> Dict[str, Any]:
        """
        Get register history.
        
        Args:
            address: Register address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {
            "address": address,
            "page": page,
            "limit": limit
        }
        
        if self.session_id:
            params["session"] = self.session_id
        
        url = REGISTER_URL.format(sdk_url, "history/any")
        return self._get(url, params)
    
    def register_transactions(self, address: str,
                              page: int = 0,
                              limit: int = 100) -> Dict[str, Any]:
        """
        Get register transactions.
        
        Args:
            address: Register address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with transactions.
        """
        params = {
            "address": address,
            "page": page,
            "limit": limit
        }
        
        if self.session_id:
            params["session"] = self.session_id
        
        url = REGISTER_URL.format(sdk_url, "transactions/any")
        return self._get(url, params)
    
    # =========================================================================
    # MARKET API
    # =========================================================================
    
    def market_create(self, order_type: str,
                      market: str,
                      amount: float,
                      price: float,
                      from_name: Optional[str] = None,
                      from_address: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a market order (bid or ask).
        
        Args:
            order_type: "bid" or "ask"
            market: Market pair (e.g., "TOKEN/NXS")
            amount: Amount to buy/sell
            price: Price per unit
            from_name: Name of account to use
            from_address: Address of account to use
            
        Returns:
            JSON response with order creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "market": market,
            "amount": amount,
            "price": price
        }
        
        if from_name:
            params["name"] = from_name
        elif from_address:
            params["address"] = from_address
        
        url = MARKET_URL.format(sdk_url, f"create/{order_type}")
        return self._post(url, params)
    
    def market_list(self, noun: str,
                    market: str,
                    page: int = 0,
                    limit: int = 100) -> Dict[str, Any]:
        """
        List market orders.
        
        Args:
            noun: Type - "bid", "ask", "order", "executed"
            market: Market pair
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with order list.
        """
        params = {
            "market": market,
            "page": page,
            "limit": limit
        }
        
        if self.session_id:
            params["session"] = self.session_id
        
        url = MARKET_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def market_execute(self, order_type: str,
                       txid: str,
                       from_name: Optional[str] = None,
                       from_address: Optional[str] = None) -> Dict[str, Any]:
        """
        Execute a market order.
        
        Args:
            order_type: "bid", "ask", or "order"
            txid: Transaction ID of the order to execute
            from_name: Name of account to use
            from_address: Address of account to use
            
        Returns:
            JSON response with execution result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        if from_name:
            params["name"] = from_name
        elif from_address:
            params["address"] = from_address
        
        url = MARKET_URL.format(sdk_url, f"execute/{order_type}")
        return self._post(url, params)
    
    def market_cancel(self, order_type: str,
                      txid: str) -> Dict[str, Any]:
        """
        Cancel a market order.
        
        Args:
            order_type: "bid", "ask", or "order"
            txid: Transaction ID of the order to cancel
            
        Returns:
            JSON response with cancellation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "txid": txid
        }
        
        url = MARKET_URL.format(sdk_url, f"cancel/{order_type}")
        return self._post(url, params)
    
    def market_user(self, noun: str,
                    market: str,
                    page: int = 0,
                    limit: int = 100) -> Dict[str, Any]:
        """
        List user's market orders.
        
        Args:
            noun: Type - "order", "executed"
            market: Market pair
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with user's order list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "market": market,
            "page": page,
            "limit": limit
        }
        
        url = MARKET_URL.format(sdk_url, f"user/{noun}")
        return self._get(url, params)
    
    # =========================================================================
    # INVOICES API
    # =========================================================================
    
    def invoices_create(self, recipient: str,
                        items: list,
                        account: Optional[str] = None,
                        account_name: Optional[str] = None) -> Dict[str, Any]:
        """
        Create an invoice.
        
        Args:
            recipient: Recipient genesis or username
            items: List of invoice items with description, units, unit_amount
            account: Account address for payment
            account_name: Account name for payment
            
        Returns:
            JSON response with invoice creation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin,
            "recipient": recipient,
            "items": json.dumps(items) if isinstance(items, list) else items
        }
        
        if account:
            params["account"] = account
        elif account_name:
            params["account_name"] = account_name
        
        url = INVOICES_URL.format(sdk_url, "create/invoice")
        return self._post(url, params)
    
    def invoices_get(self, name: Optional[str] = None,
                     address: Optional[str] = None) -> Dict[str, Any]:
        """
        Get an invoice.
        
        Args:
            name: Invoice name
            address: Invoice address
            
        Returns:
            JSON response with invoice data.
        """
        params = {}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = INVOICES_URL.format(sdk_url, "get/invoice")
        return self._get(url, params)
    
    def invoices_pay(self, name: Optional[str] = None,
                     address: Optional[str] = None,
                     from_name: Optional[str] = None,
                     from_address: Optional[str] = None) -> Dict[str, Any]:
        """
        Pay an invoice.
        
        Args:
            name: Invoice name
            address: Invoice address
            from_name: Payment account name
            from_address: Payment account address
            
        Returns:
            JSON response with payment result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        if from_name:
            params["name_from"] = from_name
        elif from_address:
            params["address_from"] = from_address
        
        url = INVOICES_URL.format(sdk_url, "pay/invoice")
        return self._post(url, params)
    
    def invoices_cancel(self, name: Optional[str] = None,
                        address: Optional[str] = None) -> Dict[str, Any]:
        """
        Cancel an invoice (issuer only).
        
        Args:
            name: Invoice name
            address: Invoice address
            
        Returns:
            JSON response with cancellation result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = INVOICES_URL.format(sdk_url, "cancel/invoice")
        return self._post(url, params)
    
    def invoices_reject(self, name: Optional[str] = None,
                        address: Optional[str] = None) -> Dict[str, Any]:
        """
        Reject an invoice (recipient only).
        
        Args:
            name: Invoice name
            address: Invoice address
            
        Returns:
            JSON response with rejection result.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "pin": self.pin
        }
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = INVOICES_URL.format(sdk_url, "reject/invoice")
        return self._post(url, params)
    
    def invoices_list(self, noun: str = "invoice",
                      page: int = 0,
                      limit: int = 100) -> Dict[str, Any]:
        """
        List invoices.
        
        Args:
            noun: Type - "invoice", "outstanding", "paid", "cancelled"
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with invoice list.
        """
        if not self.session_id:
            return self._error("Not logged in")
        
        params = {
            "session": self.session_id,
            "page": page,
            "limit": limit
        }
        
        url = INVOICES_URL.format(sdk_url, f"list/{noun}")
        return self._get(url, params)
    
    def invoices_history(self, name: Optional[str] = None,
                         address: Optional[str] = None,
                         page: int = 0,
                         limit: int = 100) -> Dict[str, Any]:
        """
        Get invoice history.
        
        Args:
            name: Invoice name
            address: Invoice address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with history.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = INVOICES_URL.format(sdk_url, "history/invoice")
        return self._get(url, params)
    
    def invoices_transactions(self, name: Optional[str] = None,
                              address: Optional[str] = None,
                              page: int = 0,
                              limit: int = 100) -> Dict[str, Any]:
        """
        Get invoice transactions.
        
        Args:
            name: Invoice name
            address: Invoice address
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with transactions.
        """
        params = {"page": page, "limit": limit}
        
        if self.session_id:
            params["session"] = self.session_id
        
        if name:
            params["name"] = name
        elif address:
            params["address"] = address
        
        url = INVOICES_URL.format(sdk_url, "transactions/invoice")
        return self._get(url, params)
    
    # =========================================================================
    # LOCAL API
    # =========================================================================
    
    def local_push(self, key: str, value: Union[str, dict]) -> Dict[str, Any]:
        """
        Push a record to local storage.
        
        Args:
            key: Record key
            value: Record value
            
        Returns:
            JSON response with push result.
        """
        params = {"key": key}
        
        if isinstance(value, dict):
            params["value"] = json.dumps(value)
        else:
            params["value"] = value
        
        url = LOCAL_URL.format(sdk_url, "push/record")
        return self._post(url, params)
    
    def local_has(self, key: str) -> Dict[str, Any]:
        """
        Check if a record exists in local storage.
        
        Args:
            key: Record key
            
        Returns:
            JSON response with existence check result.
        """
        url = LOCAL_URL.format(sdk_url, "has/record")
        return self._get(url, {"key": key})
    
    def local_list(self, page: int = 0,
                   limit: int = 100) -> Dict[str, Any]:
        """
        List records in local storage.
        
        Args:
            page: Page offset
            limit: Number of items to return
            
        Returns:
            JSON response with record list.
        """
        url = LOCAL_URL.format(sdk_url, "list/record")
        return self._get(url, {"page": page, "limit": limit})
    
    def local_erase(self, key: str) -> Dict[str, Any]:
        """
        Erase a record from local storage.
        
        Args:
            key: Record key
            
        Returns:
            JSON response with erase result.
        """
        url = LOCAL_URL.format(sdk_url, "erase/record")
        return self._post(url, {"key": key})
    
    # =========================================================================
    # INTERNAL METHODS
    # =========================================================================
    
    def _get(self, url: str, params: Optional[Dict] = None) -> Dict[str, Any]:
        """
        Make a GET request to the API.
        
        Args:
            url: The API endpoint URL
            params: Optional query parameters
            
        Returns:
            JSON response as dictionary.
        """
        if params:
            query_string = urllib.parse.urlencode(params)
            url = f"{url}?{query_string}"
        
        return self._request(url)
    
    def _post(self, url: str, params: Dict) -> Dict[str, Any]:
        """
        Make a POST request to the API.
        
        Args:
            url: The API endpoint URL
            params: POST parameters
            
        Returns:
            JSON response as dictionary.
        """
        data = urllib.parse.urlencode(params).encode('utf-8')
        return self._request(url, data)
    
    def _request(self, url: str, 
                 data: Optional[bytes] = None) -> Dict[str, Any]:
        """
        Make an HTTP request.
        
        Args:
            url: The URL to request
            data: Optional POST data
            
        Returns:
            JSON response as dictionary.
        """
        try:
            # Create SSL context
            ssl_context = None
            if not self.verify_ssl:
                ssl_context = ssl.create_default_context()
                ssl_context.check_hostname = False
                ssl_context.verify_mode = ssl.CERT_NONE
            
            # Create request
            request = urllib.request.Request(url)
            if data:
                request.add_header('Content-Type', 
                                   'application/x-www-form-urlencoded')
            
            # Make request
            if ssl_context:
                response = urllib.request.urlopen(request, data, 
                                                  timeout=self.timeout,
                                                  context=ssl_context)
            else:
                response = urllib.request.urlopen(request, data,
                                                  timeout=self.timeout)
            
            text = response.read().decode('utf-8')
            
            if not text:
                api = url.split("?")[0].split("/")
                api_name = "/".join(api[-2:]) if len(api) >= 2 else url
                return self._error(f"{api_name} returned no JSON")
            
            return json.loads(text)
            
        except urllib.error.HTTPError as e:
            try:
                error_body = e.read().decode('utf-8')
                return json.loads(error_body)
            except:
                return self._error(f"HTTP Error {e.code}: {e.reason}")
        except urllib.error.URLError as e:
            return self._error(f"Connection error: {e.reason}")
        except json.JSONDecodeError as e:
            return self._error(f"Invalid JSON response: {e}")
        except Exception as e:
            return self._error(str(e))
    
    def _error(self, message: str) -> Dict[str, Any]:
        """
        Create an error response dictionary.
        
        Args:
            message: Error message
            
        Returns:
            Error dictionary in API format.
        """
        return {
            "error": {
                "code": -1,
                "sdk-error": True,
                "message": message
            },
            "result": None
        }


# =============================================================================
# BACKWARD COMPATIBILITY - Legacy class alias
# =============================================================================

class sdk_init(NexusSDK):
    """
    Legacy compatibility class. Use NexusSDK instead.
    
    This class provides backward compatibility with the old SDK API.
    New code should use the NexusSDK class directly.
    """
    
    def __init__(self, user: str, pw: str, pin: str):
        """
        Initialize with legacy parameters.
        
        Args:
            user: Username
            pw: Password
            pin: PIN
        """
        super().__init__(username=user, password=pw, pin=pin)
    
    # Legacy method aliases for backward compatibility
    def nexus_system_get_info(self):
        return self.system_get_info()
    
    def nexus_system_list_peers(self):
        return self.system_list_peers()
    
    def nexus_system_list_lisp_eids(self):
        return self.system_list_lisp_eids()
    
    def nexus_ledger_get_blockhash(self, height):
        return self.ledger_get_blockhash(height)
    
    def nexus_ledger_get_block_by_hash(self, block_hash, verbosity):
        return self.ledger_get_block(block_hash=block_hash, verbose=verbosity)
    
    def nexus_ledger_get_block_by_height(self, block_height, verbosity):
        return self.ledger_get_block(height=block_height, verbose=verbosity)
    
    def nexus_ledger_list_blocks_by_hash(self, block_hash, limit, verbosity):
        return self.ledger_list_blocks(block_hash=block_hash, limit=limit, 
                                       verbose=verbosity)
    
    def nexus_ledger_list_blocks_by_height(self, block_height, limit, verbosity):
        return self.ledger_list_blocks(height=block_height, limit=limit,
                                       verbose=verbosity)
    
    def nexus_ledger_get_transaction(self, tx_hash, verbosity):
        return self.ledger_get_transaction(tx_hash, verbosity)
    
    def nexus_ledger_submit_transaction(self, tx_data):
        return self.ledger_submit_transaction(tx_data)
    
    def nexus_ledger_get_mininginfo(self):
        # Deprecated - use ledger/get/info instead
        return self.ledger_get_info()
    
    def nexus_users_create_user(self):
        return self.profiles_create()
    
    def nexus_users_login_user(self):
        return self.sessions_create()
    
    def nexus_users_logout_user(self):
        return self.sessions_terminate()
    
    def nexus_users_lock_user(self, minting=0, transactions=0):
        return self.sessions_lock(minting, transactions)
    
    def nexus_users_unlock_user(self, minting=0, transactions=0):
        return self.sessions_unlock(minting=minting, transactions=transactions)
    
    def nexus_supply_create_item(self, name, data):
        return self.supply_create(name, data)
    
    def nexus_supply_get_item_by_name(self, name):
        return self.supply_get(name=name)
    
    def nexus_supply_get_item_by_address(self, address):
        return self.supply_get(address=address)
    
    def nexus_supply_transfer_item_by_name(self, name, new_owner):
        return self.supply_transfer(destination=new_owner, name=name)
    
    def nexus_supply_transfer_item_by_address(self, address, new_owner):
        return self.supply_transfer(destination=new_owner, address=address)
    
    def nexus_supply_claim_item(self, txid):
        return self.supply_claim(txid)
    
    def nexus_supply_update_item_by_name(self, name, data):
        return self.supply_update(data, name=name)
    
    def nexus_supply_update_item_by_address(self, address, data):
        return self.supply_update(data, address=address)
    
    def nexus_supply_list_item_history_by_name(self, name):
        return self.supply_history(name=name)
    
    def nexus_supply_list_item_history_by_address(self, address):
        return self.supply_history(address=address)
    
    def nexus_assets_create_asset(self, asset_name, data):
        return self.assets_create(asset_name, data)
    
    def nexus_assets_get_asset_by_name(self, asset_name):
        return self.assets_get(name=asset_name)
    
    def nexus_assets_get_asset_by_address(self, asset_address):
        return self.assets_get(address=asset_address)
    
    def nexus_assets_update_asset_by_name(self, asset_name, data):
        return self.assets_update(data, name=asset_name)
    
    def nexus_assets_update_asset_by_address(self, asset_address, data):
        return self.assets_update(data, address=asset_address)
    
    def nexus_assets_transfer_asset_by_name(self, asset_name, dest_username):
        return self.assets_transfer(dest_username, name=asset_name)
    
    def nexus_assets_transfer_asset_by_address(self, asset_address, dest_address):
        return self.assets_transfer(dest_address, address=asset_address)
    
    def nexus_assets_claim_asset(self, txid):
        return self.assets_claim(txid)
    
    def nexus_assets_tokenize_asset_by_name(self, asset_name, token_name):
        return self.assets_tokenize(token_name, name=asset_name)
    
    def nexus_assets_tokenize_asset_by_address(self, asset_address, token_address):
        return self.assets_tokenize(token_address, address=asset_address)
    
    def nexus_assets_list_asset_history_by_name(self, asset_name):
        return self.assets_history(name=asset_name)
    
    def nexus_assets_list_asset_history_by_address(self, asset_address):
        return self.assets_history(address=asset_address)
    
    def nexus_tokens_create_token(self, token_name, supply, decimals=2):
        return self.finance_create("token", token_name, 
                                   supply=supply, decimals=decimals)
    
    def nexus_tokens_create_account(self, account_name, token_name):
        return self.finance_create("account", account_name, token=token_name)
    
    def nexus_tokens_get_token_by_name(self, token_name):
        return self.finance_get("token", name=token_name)
    
    def nexus_tokens_get_token_by_address(self, token_address):
        return self.finance_get("token", address=token_address)
    
    def nexus_tokens_get_account_by_name(self, account_name):
        return self.finance_get("account", name=account_name)
    
    def nexus_tokens_get_account_by_address(self, account_address):
        return self.finance_get("account", address=account_address)
    
    def nexus_tokens_debit_account_by_name(self, from_name, to_name, amount):
        return self.finance_debit(amount, name_from=from_name, name_to=to_name)
    
    def nexus_tokens_debit_account_by_address(self, from_address, to_address, 
                                               amount):
        return self.finance_debit(amount, address_from=from_address, 
                                  address_to=to_address)
    
    def nexus_tokens_debit_token_by_name(self, from_name, to_name, amount):
        return self.finance_debit(amount, name_from=from_name, name_to=to_name)
    
    def nexus_tokens_debit_token_by_address(self, from_address, to_address, 
                                             amount):
        return self.finance_debit(amount, address_from=from_address,
                                  address_to=to_address)
    
    def nexus_tokens_credit_token_by_name(self, to_name, amount, txid):
        return self.finance_credit(txid, name=to_name)
    
    def nexus_tokens_credit_token_by_address(self, to_address, amount, txid):
        return self.finance_credit(txid, address=to_address)
    
    def nexus_tokens_credit_account_by_name(self, to_name, amount, txid, 
                                             name_proof=None):
        return self.finance_credit(txid, name=to_name)
    
    def nexus_tokens_credit_account_by_address(self, to_address, amount, txid,
                                                address_proof=None):
        return self.finance_credit(txid, address=to_address)
    
    def nexus_finance_create_account(self, name):
        return self.finance_create("account", name)
    
    def nexus_finance_get_account_by_name(self, name):
        return self.finance_get("account", name=name)
    
    def nexus_finance_get_account_by_address(self, address):
        return self.finance_get("account", address=address)
    
    def nexus_finance_debit_account_by_name(self, from_name, to_name, amount):
        return self.finance_debit(amount, name_from=from_name, name_to=to_name)
    
    def nexus_finance_debit_account_by_address(self, from_address, to_address,
                                                amount):
        return self.finance_debit(amount, address_from=from_address,
                                  address_to=to_address)
    
    def nexus_finance_credit_account_by_name(self, to_name, amount, txid):
        return self.finance_credit(txid, name=to_name)
    
    def nexus_finance_credit_account_by_address(self, to_address, amount, txid):
        return self.finance_credit(txid, address=to_address)
    
    def nexus_finance_list_accounts(self):
        return self.finance_list("account")
    
    def nexus_finance_get_stakeinfo(self):
        return self.finance_get_stakeinfo()
    
    def nexus_finance_set_stake(self, amount):
        return self.finance_set_stake(amount)


# =============================================================================
# MODULE TEST
# =============================================================================

if __name__ == "__main__":
    print("Nexus SDK v2.0 - Python 3")
    print("=" * 40)
    
    # Test without login
    sdk = NexusSDK()
    
    print("\nTesting system/get/info...")
    result = sdk.system_get_info()
    if "result" in result:
        print(f"  Node version: {result['result'].get('version', 'N/A')}")
    elif "error" in result:
        print(f"  Error: {result['error'].get('message', 'Unknown')}")
    
    print("\nSDK initialized successfully!")
    print("Use NexusSDK class for new code, or sdk_init for legacy compatibility.")



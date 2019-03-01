/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_INCLUDE_AMBASSADOR_H
#define NEXUS_LEGACY_INCLUDE_AMBASSADOR_H

#include <string>
#include <vector>
#include <inttypes.h>

namespace Legacy
{

    /** Dummy address for running on the Testnet. **/
    const std::string TESTNET_DUMMY_ADDRESS             = "4jzgcyvCM6Yv8uoAPwCwe5eSikccs7ofJBnxsRWtmePGuJYnV8E";


    /** New testnet dummy addresses. */
    const std::string TESTNET_DUMMY_AMBASSADOR_RECYCLED = "4kRwiTAu6h3ZPTABfZ7wYjVfovHWWHJxShATUYCYYsSWVdiuCYa";
    const std::string TESTNET_DUMMY_DEVELOPER_RECYCLED  = "4kUF9T3tCMFtRPyoFX5Kyhn6BVwxi5dgnVyDwPxUr8kZpwdr6Zy";


    /** Signature to Check Testnet Blocks are Produced Correctly. **/
    extern std::vector<uint8_t> TESTNET_DUMMY_SIGNATURE;


    /** New testnet dummy signatures. **/
    extern std::vector<uint8_t> TESTNET_DUMMY_SIGNATURE_AMBASSADOR_RECYCLED;
    extern std::vector<uint8_t> TESTNET_DUMMY_SIGNATURE_DEVELOPER_RECYCLED;


    /** Addresses of the Exchange Channels. **/
    const std::string AMBASSADOR_ADDRESSES[] =
    {
        "2Qn8MsUCkv9S8X8sMvUPkr9Ekqqg5VKzeKJk2ftcFMUiJ5QnWRc",
        "2S4WLATVCdJXTpcfdkDNDVKK5czDi4rjR5HrCRjayktJZ4rN8oA",
        "2RE29WahXWawQ9huhyaGhfvEMmUWHH9Hfo1anbNk8eW3nTU7H2g",
        "2QpSfg6MBZYCjQKXjTgo9eHoKMCJsYjLQsNT3xeeAYhrQmNBEUd",
        "2RHjigCh1qt1j3WKz4mShFBiVE5g6z9vrFpGMT6EDQsFJbtx4hr",
        "2SZ87FB1zukH5h7BLDT4yUyMTQnEJEt2KzpGYFxuUzMqAxEFN7Y",
        "2STyHuCeBgE81ZNjhH5QB8UXViXW7WPYM1YQgmXfLvMJXaKAFCs",
        "2SLq49uDrhLyP1N7Xnkj86WCHQUKxn6zx38LBNoTgwsAjfV1seq",
        "2RwtQdi3VPPQqht15QmXnS4KELgxrfaH2hXSywtJrfDdCJMnwPQ",
        "2SWscUR6rEezZKgFh5XkEyhmRmja2qrHMRsfmtxdapwMymmM96Q",
        "2SJzPMXNPEgW2zJW4489qeiCjdUanDTqCuSNAMmZXm1KX269jAt",
        "2Rk2mBEYWkGDMzQhEqdpSGZ77ZGvp9HWAbcsY6mDtbWKJy4DQuq",
        "2Rnh3qFvzeRQmSJEHtz6dNphq3r7uDSGQdjucnVFtpcuzBbeiLx"
    };


    /** Addresses for the Developer Accounts. **/
    const std::string DEVELOPER_ADDRESSES[] =
    {
        "2Qp1rHzLCCsL4RcmLRcNhhAjnFShDXQaAjyBB9YpDSomCJsfGfS",
        "2SFx2tc8tLHBtkLkfK7nNjkU9DwvZZMNKKLaeX4xcG8ev4HQqVP",
        "2SawW67sUcVtLNarcAkVaFR2L1R8AWujkoryJHi8L47bdDP8hwC",
        "2QvzSNP3jy4MjqmB7jRy6jRGrDz6s6ALzTwu8htdohraU6Fdgrc",
        "2RxmzQ1XomQrbzQimajfnC2FubYnSzbwz5EkU2du7bDxuJW7i2L",
        "2S2JaEbGoBFx7N2YGEUJbWRjLf35tY7kezV8R9vzq9Wu1f5cwVz",
        "2S9bR5wB6RcBm1weFPBXCZai5pRFisa9zQSEedrdi9QLmd5Am8y",
        "2S6NjGDuTozyCWjMziwEBYKnnaC6fy5zRjDmd2NQhHBjuzKw4bg",
        "2RURDVPFD14eYCC7brgio2HF77LP22SdN5CeAvwQAwdSPdS95dT",
        "2SNAEJ6mbmpPXbP6ZkmH7FgtWTWcNnw2Pcs3Stb4PDaq3vH1GgE",
        "2SDnQuMgW9UdESUUekrrWegxSHLQWnFWJ2BNWAUQVecKNwBeNh5",
        "2SCLU4SKxh2P27foN9NRoAdtUZMdELMvBpfmVER98HayRRqGKFx",
        "2SLN8urU2mERZRQajqYe9VgQQgK7tPWWQ1679c5B3scZKP2vDxi"
    };


    /** Binary Data of Each Developer Address for Signature Verification of Exchange Transaction in Accept Block. **/
    extern std::vector<uint8_t> AMBASSADOR_SCRIPT_SIGNATURES[13];
    extern std::vector<uint8_t> DEVELOPER_SCRIPT_SIGNATURES[13];


    /** New ambassador addresses recycled. **/
    const std::string AMBASSADOR_ADDRESSES_RECYCLED[] =
    {
        "2RSWG4zGzJZdkem23CeuqSEVjjbwUbVe2oZRpcA5ZpSqTojzQYy",
        "2RmF9e5k2W4RvKsZsKXK8y6Md1Hd5joNQFrrEKeLMpv3CfFMjQZ",
        "2QyxbcfCckkr5HQzxGqrKdChnVwXzwuAjt8ADMBYw5i3jxYu96H",
        "2Ruf4e6FWEkJKoPewJ9DPi5gjgLCR5a8NGvFd99ycsPjxmrzbo2",
        "2S7diRGZQF9nwmJ3J1hH1Zapgvo551eBtPT69arsSLrhuyqfpwR",
        "2RdnyGpVhQarMswVj9We7eNs6TTazWEcnFCwVyK7zReg5hKq8kc",
        "2RHsaGpRjSheYDGpBsvDPyW5tSuQjHXkBdbZZ6QLpLBhcWHJyUZ",
        "2RhMNr6qaEnQndMUiuSwBpNToQovcDbVK6FDGkxrMALdcPY1zWv",
        "2SaPcUuSMj7J7szWeuH5ZHhPg9oQGtfJFrU7Prezkw6aqENXGvc",
        "2Rjsdzb6HPPNtjCmDbJ1fNHFKNmQLfxVGYqE1gCdfPnJh78bKwz",
        "2RsG4LMrMCCuPnsy21FRSXrbXRVLJWguQbzL4aZFJEWSkvu75Qb",
        "2QmAPn1ymoJj2UVYGedetEkN3WkPXPDL1Tn13W6vAoDS71543UJ",
        "2RbJ6uqpnVmNvzzHz73v6m3J4M3ks3Jv6E7bDZhTgtXkSTEsiob"
    };


    /** New developer addresses recycled. **/
    const std::string DEVELOPER_ADDRESSES_RECYCLED[] =
    {
        "2SPinzyuXJdf9iFchK4fvH7Fcu6SLqTasugDaPpUTrV4WDo27Vx",
        "2RS4jz5TdHNvhnQPGQCfhsddyT6PXc4tip2uRQ61hMK9dkfFZE4",
        "2Qip9FFJH3CjhHLv7ZfxpjenKohmbEz2zVe27TfE5gqT1rg8Y8P",
        "2R7gedizZWpe9RySUXmxVznoJJie3c7SypBVHkRZ8Dbo6mBj8zw",
        "2Rz6Z6XMPH8A2iyeKnYS14cV9rHDwGWPCs2z4EZ6Qc4FX8DBdE3",
        "2SRto8HnG6GfXY6m37zxSbKD9shAXYtdWn62umezHhC85i61LKB",
        "2Rdfzmijbn8m7yWRkhWdsfJJ78XSAp7VPks2Ci18x98hFfru4bN",
        "2S7JbY11Kfss3A2bhchzzxeervQZ6JpGDSC7FBJ3oXq1xoCbHse",
        "2RVKmatLPGP6iWGvMdXzQrkrkNcMkgeoHXdJafrg7UTHhjxFLcR",
        "2S81rREFrxgGoBhp6g7yi14sKpnDNEnwp28q9Eqg1KG5JMvRjXF",
        "2ReZqUDgSMBFJ6W4qGrDDxecmgChYt2RZjonAT7eNzUdCZQyHA9",
        "2QkNsRC3jsqCSeeUpJgCusMX11QTcUHYNrh798HsSdWTyFQ2du3",
        "2Qv9haWgvomJkawpy2EDDmtCaE3rXpnvtH4pRRuM8JgJQTxoCt8"
    };

    /** Binary Data of Each Developer Address for Signature Verification of Exchange Transaction in Accept Block. **/
    extern std::vector<uint8_t> AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED[13];
    extern std::vector<uint8_t> DEVELOPER_SCRIPT_SIGNATURES_RECYCLED[13];


    /** Initialize the scripts. **/
    void InitializeScripts();

}

#endif

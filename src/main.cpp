/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>
#include <LLP/include/port.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/include/lisp.h>
#include <LLP/include/port.h>
#include <LLP/include/channel_state_manager.h>

#include <LLD/include/global.h>
#include <LLD/types/ledger.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/cmd.h>
#include <TAO/API/types/authentication.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/dispatch.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/hex.h>
#include <Util/include/signals.h>
#include <Util/include/daemon.h>

#include <Legacy/include/ambassador.h>
#include <Legacy/include/global.h>
#include <Legacy/wallet/wallet.h>

#ifndef WIN32
#include <sys/resource.h>
#endif

#include <algorithm>
#include <filesystem>
#include <limits>

namespace
{
    static constexpr uint32_t AUDITBLOCK_MAX_SECTOR_FILE = 99999;

    bool ParseAuditHashArg(const std::string& strValue, uint1024_t& hashOut, std::string& strError,
                          const bool fAllowZero = false)
    {
        if(strValue.empty())
        {
            strError = "missing required value";
            return false;
        }

        if(strValue.size() != 256 || !IsHex(strValue))
        {
            strError = "must be full 256-hex-character uint1024 hash";
            return false;
        }

        hashOut.SetHex(strValue);
        if(!fAllowZero && hashOut == 0)
        {
            strError = "hash cannot be zero";
            return false;
        }

        return true;
    }


    bool ParseAuditUint32Arg(const std::string& strValue, const char* pszName, uint32_t& nOut, std::string& strError)
    {
        if(strValue.empty())
        {
            strError = debug::safe_printstr(pszName, " is empty");
            return false;
        }

        uint64_t nValue = 0;
        for(const char ch : strValue)
        {
            if(ch < '0' || ch > '9')
            {
                strError = debug::safe_printstr(pszName, " must be an unsigned integer");
                return false;
            }

            nValue = (nValue * 10) + static_cast<uint64_t>(ch - '0');
            if(nValue > std::numeric_limits<uint32_t>::max())
            {
                strError = debug::safe_printstr(pszName, " exceeds uint32 range");
                return false;
            }
        }

        nOut = static_cast<uint32_t>(nValue);
        return true;
    }


    void BuildAuditScanOptions(const bool fMaxFilesProvided, const uint32_t nMaxFiles,
                               const bool fStartFileProvided, const uint32_t nStartFile,
                               const bool fEndFileProvided, const uint32_t nEndFile,
                               LLD::BlockAuditScanOptions& optionsOut)
    {
        optionsOut = LLD::BlockAuditScanOptions();
        optionsOut.nMaxFiles = std::max<uint32_t>(1, nMaxFiles);

        if(!(fStartFileProvided || fEndFileProvided))
            return;

        optionsOut.fHasStartFile = true;
        optionsOut.fHasEndFile = true;

        if(fStartFileProvided && fEndFileProvided)
        {
            optionsOut.nStartFile = nStartFile;
            optionsOut.nEndFile = nEndFile;
            return;
        }

        if(fStartFileProvided)
        {
            optionsOut.nStartFile = nStartFile;
            optionsOut.nEndFile = fMaxFilesProvided
                ? static_cast<uint32_t>(std::min<uint64_t>(
                    static_cast<uint64_t>(AUDITBLOCK_MAX_SECTOR_FILE),
                    static_cast<uint64_t>(nStartFile) + static_cast<uint64_t>(optionsOut.nMaxFiles - 1)))
                : nStartFile;

            return;
        }

        optionsOut.nEndFile = nEndFile;
        optionsOut.nStartFile = (nEndFile >= (optionsOut.nMaxFiles - 1))
            ? (nEndFile - (optionsOut.nMaxFiles - 1))
            : 0;
    }


    int RunOfflineAuditBlock()
    {
        if(config::fClient.load())
        {
            debug::error(FUNCTION, "-auditblock is NODE-only and does not support -client mode");
            return 2;
        }

        const std::string strHashArg = config::GetArg("-auditblock", "");
        uint1024_t hashTarget = 0;
        std::string strError;
        if(!ParseAuditHashArg(strHashArg, hashTarget, strError))
        {
            debug::error(FUNCTION, "-auditblock ", strError);
            return 2;
        }

        bool fExpectedHeightProvided = false;
        uint32_t nExpectedHeight = 0;
        if(config::HasArg("-auditblockheight"))
        {
            fExpectedHeightProvided = true;
            if(!ParseAuditUint32Arg(config::GetArg("-auditblockheight", ""), "-auditblockheight",
                                    nExpectedHeight, strError))
            {
                debug::error(FUNCTION, strError);
                return 2;
            }
        }

        bool fChildProvided = false;
        uint1024_t hashChild = 0;
        if(config::HasArg("-auditblockchild"))
        {
            fChildProvided = true;
            if(!ParseAuditHashArg(config::GetArg("-auditblockchild", ""), hashChild, strError, true))
            {
                debug::error(FUNCTION, "-auditblockchild ", strError);
                return 2;
            }
        }

        bool fMaxFilesProvided = false;
        uint32_t nMaxFiles = 8;
        if(config::HasArg("-auditblockfiles"))
        {
            fMaxFilesProvided = true;
            if(!ParseAuditUint32Arg(config::GetArg("-auditblockfiles", ""), "-auditblockfiles",
                                    nMaxFiles, strError))
            {
                debug::error(FUNCTION, strError);
                return 2;
            }

            if(nMaxFiles == 0)
            {
                debug::error(FUNCTION, "-auditblockfiles must be greater than zero");
                return 2;
            }
        }

        bool fStartFileProvided = false;
        uint32_t nStartFile = 0;
        if(config::HasArg("-auditblockstartfile"))
        {
            fStartFileProvided = true;
            if(!ParseAuditUint32Arg(config::GetArg("-auditblockstartfile", ""), "-auditblockstartfile",
                                    nStartFile, strError))
            {
                debug::error(FUNCTION, strError);
                return 2;
            }
        }

        bool fEndFileProvided = false;
        uint32_t nEndFile = 0;
        if(config::HasArg("-auditblockendfile"))
        {
            fEndFileProvided = true;
            if(!ParseAuditUint32Arg(config::GetArg("-auditblockendfile", ""), "-auditblockendfile",
                                    nEndFile, strError))
            {
                debug::error(FUNCTION, strError);
                return 2;
            }
        }

        if(fStartFileProvided && fEndFileProvided && nStartFile > nEndFile)
        {
            debug::error(FUNCTION, "invalid range: -auditblockstartfile cannot exceed -auditblockendfile");
            return 2;
        }

        if((fStartFileProvided && nStartFile > AUDITBLOCK_MAX_SECTOR_FILE)
        || (fEndFileProvided && nEndFile > AUDITBLOCK_MAX_SECTOR_FILE))
        {
            debug::error(FUNCTION, "auditblock file range exceeds supported sector file max ",
                AUDITBLOCK_MAX_SECTOR_FILE);
            return 2;
        }

        const std::string strLedgerBase = debug::safe_printstr(config::GetDataDir(), "_LEDGER/");
        const std::string strLedgerDatachain = debug::safe_printstr(strLedgerBase, "datachain/");
        if(!std::filesystem::is_directory(strLedgerDatachain))
        {
            debug::error(FUNCTION, "ledger datachain not present for read-only audit at ", strLedgerBase);
            return 3;
        }

        LLD::LedgerDB ledgerReadOnly(0, 256 * 256 * 64);
        LLD::LedgerDB* const pLedger = &ledgerReadOnly;

        LLD::BlockAuditScanOptions options;
        BuildAuditScanOptions(fMaxFilesProvided, nMaxFiles, fStartFileProvided, nStartFile,
                              fEndFileProvided, nEndFile, options);

        const bool fHashKeyExists = pLedger->Exists(hashTarget);
        TAO::Ledger::BlockState stateByHash;
        LLD::BlockAuditAliasResult hashAlias;
        const bool fHashKeyReadable = pLedger->AuditReadBlockRecord(hashTarget, stateByHash, hashAlias);
        const bool fHashKeyMatches = fHashKeyReadable && (stateByHash.GetHash() == hashTarget);

        LLD::BlockAuditScanResult rawScan;
        if(!pLedger->AuditScanBlockRecords(hashTarget, options, rawScan))
        {
            debug::error(FUNCTION, "raw scan infrastructure failed");
            return 3;
        }

        bool fHeightChecked = false;
        bool fHeightExists = false;
        bool fHeightReadable = false;
        bool fHeightMatches = false;
        uint32_t nHeightChecked = 0;
        TAO::Ledger::BlockState stateByHeight;
        TAO::Ledger::BlockState stateByChild;
        bool fExpectedHeightCheckPerformed = false;
        bool fExpectedHeightExists = false;
        bool fExpectedHeightReadable = false;
        bool fExpectedHeightMatches = false;
        TAO::Ledger::BlockState stateByExpectedHeight;
        LLD::BlockAuditAliasResult heightAlias;
        LLD::BlockAuditAliasResult expectedHeightAlias;
        LLD::BlockAuditAliasResult childAlias;

        if(fHashKeyReadable)
        {
            fHeightChecked = true;
            nHeightChecked = stateByHash.nHeight;
        }
        else if(fExpectedHeightProvided)
        {
            fHeightChecked = true;
            nHeightChecked = nExpectedHeight;
        }
        else
        {
            for(const auto& candidate : rawScan.vMatches)
            {
                if(candidate.fSerializedComplete)
                {
                    fHeightChecked = true;
                    nHeightChecked = candidate.nHeight;
                    break;
                }
            }
        }

        if(fHeightChecked)
        {
            fHeightExists = pLedger->Exists(std::make_pair(std::string("height"), nHeightChecked));
            fHeightReadable = pLedger->AuditReadBlockRecord(
                std::make_pair(std::string("height"), nHeightChecked), stateByHeight, heightAlias);
            fHeightMatches = fHeightReadable && stateByHeight.GetHash() == hashTarget
                && stateByHeight.nHeight == nHeightChecked;

            if(fExpectedHeightProvided && nExpectedHeight != nHeightChecked)
            {
                fExpectedHeightCheckPerformed = true;
                fExpectedHeightExists = pLedger->Exists(std::make_pair(std::string("height"), nExpectedHeight));
                fExpectedHeightReadable = pLedger->AuditReadBlockRecord(
                    std::make_pair(std::string("height"), nExpectedHeight), stateByExpectedHeight, expectedHeightAlias);
                fExpectedHeightMatches = fExpectedHeightReadable && (stateByExpectedHeight.GetHash() == hashTarget)
                    && stateByExpectedHeight.nHeight == nExpectedHeight;
            }
        }

        const size_t nRawMatches = rawScan.vMatches.size();
        const bool fMultipleRawMatches = nRawMatches > 1;
        const bool fRawFound = rawScan.fFound;
        const bool fHeightComparable = fHeightExists && heightAlias.fExists && hashAlias.fExists
            && !heightAlias.fKeychainOnly && !hashAlias.fKeychainOnly;
        const bool fAliasSameSector = fHeightComparable
            && hashAlias.nSectorFile == heightAlias.nSectorFile
            && hashAlias.nSectorStart == heightAlias.nSectorStart
            && hashAlias.nSectorSize == heightAlias.nSectorSize;

        bool fChildExists = false;
        bool fChildReadable = false;
        bool fChildPrevMatchesTarget = false;
        bool fHashNextMatchesChild = false;
        bool fHeightNextMatchesChild = false;
        if(fChildProvided)
        {
            fChildExists = pLedger->Exists(hashChild);
            fChildReadable = pLedger->AuditReadBlockRecord(hashChild, stateByChild, childAlias);
            fChildPrevMatchesTarget = fChildReadable && stateByChild.GetHash() == hashChild
                && (stateByChild.hashPrevBlock == hashTarget);
            fHashNextMatchesChild = fHashKeyMatches && (stateByHash.hashNextBlock == hashChild);
            fHeightNextMatchesChild = fHeightReadable && fHeightMatches
                && (stateByHeight.hashNextBlock == hashChild);
        }

        std::string strClassification = "BLOCK_NOT_FOUND_IN_BOUNDED_SCAN";
        if(rawScan.fTruncatedRecord || rawScan.fMalformedRecord)
            strClassification = "RAW_RECORD_TRUNCATED_OR_MALFORMED";
        else if((fHeightChecked && fHeightReadable && !fHeightMatches)
             || (fExpectedHeightCheckPerformed && fExpectedHeightReadable && !fExpectedHeightMatches))
            strClassification = "HEIGHT_INDEX_POINTS_TO_DIFFERENT_BLOCK";
        else if(fHeightComparable && fHashKeyMatches && fHeightMatches && !fAliasSameSector)
            strClassification = "HASH_HEIGHT_ALIAS_DIFFERENT_SECTORS";
        else if(fHashKeyExists && fHashKeyReadable && fHashKeyMatches)
            strClassification = "HASH_KEY_READABLE";
        else if(fHashKeyExists && fHashKeyReadable && !fHashKeyMatches)
            strClassification = "HASH_KEY_READABLE_MISMATCH";
        else if(fHeightChecked && fHeightMatches && fHashKeyExists && !fHashKeyReadable)
            strClassification = "HASH_KEY_PRESENT_UNREADABLE_HEIGHT_INDEX_PRESENT";
        else if(!fHashKeyExists && fHeightMatches)
            strClassification = "HASH_ALIAS_MISSING_HEIGHT_INDEX_PRESENT";
        else if(fHashKeyExists && !fHashKeyReadable && fRawFound)
            strClassification = "HASH_KEY_PRESENT_UNREADABLE_RAW_RECORD_PRESENT";
        else if(fHashKeyExists && !fHashKeyReadable && !fRawFound)
            strClassification = "HASH_KEY_PRESENT_UNREADABLE_RAW_RECORD_MISSING";
        else if(!fHashKeyExists && fRawFound)
            strClassification = "HASH_ALIAS_MISSING_RAW_RECORD_PRESENT";

        if(strClassification == "BLOCK_NOT_FOUND_IN_BOUNDED_SCAN" && rawScan.fOversizedFile)
            strClassification = "RAW_SECTOR_FILE_OVERSIZED";

        if(fMultipleRawMatches && strClassification != "RAW_RECORD_TRUNCATED_OR_MALFORMED")
            strClassification = "MULTIPLE_RAW_MATCHES";

        const char* const strStatus =
            (fHashKeyMatches || fHeightMatches || fExpectedHeightMatches || fRawFound) ? "FOUND" : "NOT_FOUND";
        debug::log(0, "AUDITBLOCK hash=", hashTarget.ToString(), " status=", strStatus,
            " hash_key_exists=", fHashKeyExists ? "true" : "false",
            " raw_found=", fRawFound ? "true" : "false");

        if(fHeightChecked)
        {
            debug::log(0, "AUDITBLOCK height_index checked=true height=", nHeightChecked,
                " exists=", fHeightExists ? "true" : "false",
                " readable=", fHeightReadable ? "true" : "false",
                " matches=", fHeightMatches ? "true" : "false");
            debug::log(0, "AUDITBLOCK alias_sector hash_exists=", hashAlias.fExists ? "true" : "false",
                " height_exists=", fHeightExists ? "true" : "false",
                " same_sector=", fHeightComparable ? (fAliasSameSector ? "true" : "false") : "unknown");

            if(fExpectedHeightCheckPerformed)
            {
                debug::log(0, "AUDITBLOCK expected_height_index checked=true height=", nExpectedHeight,
                    " exists=", fExpectedHeightExists ? "true" : "false",
                    " readable=", fExpectedHeightReadable ? "true" : "false",
                    " matches=", fExpectedHeightMatches ? "true" : "false");
            }
        }
        else
            debug::log(0, "AUDITBLOCK height_index checked=false");

        if(fChildProvided)
        {
            debug::log(0, "AUDITBLOCK child_link checked=true child=", hashChild.ToString(),
                " child_exists=", fChildExists ? "true" : "false",
                " child_readable=", fChildReadable ? "true" : "false",
                " child_prev_matches_target=", fChildPrevMatchesTarget ? "true" : "false",
                " hash_next_matches_child=", fHashNextMatchesChild ? "true" : "false",
                " height_next_matches_child=", fHeightNextMatchesChild ? "true" : "false");
        }

        for(size_t i = 0; i < nRawMatches; ++i)
        {
            const auto& match = rawScan.vMatches[i];
            const bool fHeightValid = !fExpectedHeightProvided || (match.nHeight == nExpectedHeight);
            const bool fChildValid = !fChildProvided || (match.hashNextBlock == hashChild);

            debug::log(0, "AUDITBLOCK candidate[", i, "] hash=", match.hashBlock.ToString(),
                " height=", match.nHeight,
                " prev=", match.hashPrevBlock.ToString(),
                " next=", match.hashNextBlock.ToString(),
                " sector_file=", match.nSectorFile,
                " sector_offset=", match.nSectorStart,
                " sector_size=", match.nSectorSize,
                " valid_height=", fHeightValid ? "true" : "false",
                " valid_child_link=", fChildValid ? "true" : "false",
                " serialized_complete=", match.fSerializedComplete ? "true" : "false");
        }

        encoding::json jSummary;
        jSummary["status"] = strStatus;
        jSummary["hash"] = hashTarget.ToString();
        jSummary["expected_height"] = fExpectedHeightProvided ? encoding::json(nExpectedHeight) : encoding::json(nullptr);
        jSummary["hash_key"] = {
            {"exists", fHashKeyExists},
            {"readable", fHashKeyReadable},
            {"matches", fHashKeyMatches},
            {"oversized", hashAlias.fOversized},
            {"keychain_only", hashAlias.fKeychainOnly},
            {"sector_file", hashAlias.fExists ? encoding::json(hashAlias.nSectorFile) : encoding::json(nullptr)},
            {"sector_offset", hashAlias.fExists ? encoding::json(hashAlias.nSectorStart) : encoding::json(nullptr)},
            {"sector_size", hashAlias.fExists ? encoding::json(hashAlias.nSectorSize) : encoding::json(nullptr)}
        };
        jSummary["height_index"] = {
            {"checked", fHeightChecked},
            {"height", fHeightChecked ? encoding::json(nHeightChecked) : encoding::json(nullptr)},
            {"exists", fHeightExists},
            {"readable", fHeightReadable},
            {"matches", fHeightMatches},
            {"oversized", heightAlias.fOversized},
            {"keychain_only", heightAlias.fKeychainOnly},
            {"sector_file", heightAlias.fExists ? encoding::json(heightAlias.nSectorFile) : encoding::json(nullptr)},
            {"sector_offset", heightAlias.fExists ? encoding::json(heightAlias.nSectorStart) : encoding::json(nullptr)},
            {"sector_size", heightAlias.fExists ? encoding::json(heightAlias.nSectorSize) : encoding::json(nullptr)}
        };
        if(fExpectedHeightCheckPerformed)
        {
            jSummary["height_index"]["expected_height_check"] = {
                {"checked", true},
                {"height", nExpectedHeight},
                {"exists", fExpectedHeightExists},
                {"readable", fExpectedHeightReadable},
                {"matches", fExpectedHeightMatches},
                {"oversized", expectedHeightAlias.fOversized},
                {"keychain_only", expectedHeightAlias.fKeychainOnly},
                {"sector_file", expectedHeightAlias.fExists ? encoding::json(expectedHeightAlias.nSectorFile) : encoding::json(nullptr)},
                {"sector_offset", expectedHeightAlias.fExists ? encoding::json(expectedHeightAlias.nSectorStart) : encoding::json(nullptr)},
                {"sector_size", expectedHeightAlias.fExists ? encoding::json(expectedHeightAlias.nSectorSize) : encoding::json(nullptr)}
            };
        }
        jSummary["alias_relation"] = {
            {"comparable", fHeightComparable},
            {"same_sector", fHeightComparable ? encoding::json(fAliasSameSector) : encoding::json(nullptr)}
        };
        if(fChildProvided)
        {
            jSummary["child_link"] = {
                {"checked", true},
                {"hash", hashChild.ToString()},
                {"exists", fChildExists},
                {"readable", fChildReadable},
                {"child_prev_matches_target", fChildPrevMatchesTarget},
                {"hash_next_matches_child", fHashNextMatchesChild},
                {"height_next_matches_child", fHeightNextMatchesChild},
                {"sector_file", childAlias.fExists ? encoding::json(childAlias.nSectorFile) : encoding::json(nullptr)},
                {"sector_offset", childAlias.fExists ? encoding::json(childAlias.nSectorStart) : encoding::json(nullptr)},
                {"sector_size", childAlias.fExists ? encoding::json(childAlias.nSectorSize) : encoding::json(nullptr)}
            };
        }

        jSummary["raw_scan"] = {
            {"scan_start_file", rawScan.nScanStartFile},
            {"scan_end_file", rawScan.nScanEndFile},
            {"files_scanned", rawScan.nFilesScanned},
            {"files_skipped", rawScan.nFilesSkipped},
            {"records_scanned", rawScan.nRecordsScanned},
            {"found", fRawFound},
            {"matches", nRawMatches},
            {"multiple_matches", fMultipleRawMatches},
            {"malformed_record", rawScan.fMalformedRecord},
            {"truncated_record", rawScan.fTruncatedRecord},
            {"oversized_file", rawScan.fOversizedFile}
        };

        jSummary["candidates"] = encoding::json::array();
        for(const auto& candidate : rawScan.vMatches)
        {
            jSummary["candidates"].push_back({
                {"height", candidate.nHeight},
                {"hash", candidate.hashBlock.ToString()},
                {"hash_prev", candidate.hashPrevBlock.ToString()},
                {"hash_next", candidate.hashNextBlock.ToString()},
                {"valid_hash", candidate.hashBlock == hashTarget},
                {"valid_height", !fExpectedHeightProvided || candidate.nHeight == nExpectedHeight},
                {"valid_child_link", !fChildProvided || candidate.hashNextBlock == hashChild},
                {"serialized_complete", candidate.fSerializedComplete},
                {"sector_file", candidate.nSectorFile},
                {"sector_offset", candidate.nSectorStart},
                {"sector_size", candidate.nSectorSize}
            });
        }

        if(!rawScan.vMatches.empty())
        {
            const auto& candidate = rawScan.vMatches.front();
            jSummary["candidate"] = {
                {"height", candidate.nHeight},
                {"hash", candidate.hashBlock.ToString()},
                {"hash_prev", candidate.hashPrevBlock.ToString()},
                {"hash_next", candidate.hashNextBlock.ToString()},
                {"valid_hash", candidate.hashBlock == hashTarget},
                {"valid_height", !fExpectedHeightProvided || candidate.nHeight == nExpectedHeight},
                {"valid_child_link", !fChildProvided || candidate.hashNextBlock == hashChild},
                {"serialized_complete", candidate.fSerializedComplete},
                {"sector_file", candidate.nSectorFile},
                {"sector_offset", candidate.nSectorStart},
                {"sector_size", candidate.nSectorSize}
            };
        }

        jSummary["classification"] = strClassification;
        debug::log(0, jSummary.dump());

        return 0;
    }
}


/** RunAutoLogin
 *
 *  Parse credentials and create/unlock SESSION::DEFAULT for unattended mining.
 *  Handles both credential formats:
 *    Compact  : -autologin=username:password[:pin]
 *    Legacy   : -autologin=1 -username=user -password -pin=1234
 *
 *  Must only be called after the node is fully synced and when not in
 *  multiuser mode.  Does not start mining template creation; only creates
 *  and unlocks the default session.
 *
 **/
static void RunAutoLogin()
{
    /* Parse autologin credentials -- two formats are supported:
     *
     *   Compact (single-arg): autologin=username:password[:pin]
     *     The value of -autologin itself carries the credentials.
     *     PIN may be omitted and supplied via the separate -pin arg.
     *
     *   Legacy (multi-arg): autologin=1  username=user  ******  pin=1234
     *     Credentials are read from individual -username/-password/-pin args.
     *
     * Both formats may coexist; the compact format takes precedence for
     * username/password when present.
     */
    std::string strLoginUser = config::GetArg("-username", "");
    std::string strLoginPass = config::GetArg("-password", "");
    std::string strLoginPIN  = config::GetArg("-pin", "");

    /* Detect compact format: value contains ':' (e.g. "alice:s3cr3t" or "alice:s3cr3t:5678")
     * Split on the FIRST colon (username) and SECOND colon (password/pin boundary) only,
     * so passwords that contain colons are preserved intact. */
    {
        const std::string strAutoLoginVal = config::GetArg("-autologin", "");
        const size_t nFirstColon = strAutoLoginVal.find(':');
        if(nFirstColon != std::string::npos)
        {
            std::string strUser = strAutoLoginVal.substr(0, nFirstColon);
            std::string strRest = strAutoLoginVal.substr(nFirstColon + 1);

            /* Find optional second colon that separates password from PIN.
             * Everything between the first and second colon is the password
             * (so passwords may contain colons). */
            const size_t nSecondColon = strRest.find(':');
            std::string strPass = (nSecondColon != std::string::npos)
                                  ? strRest.substr(0, nSecondColon)
                                  : strRest;
            std::string strPIN2 = (nSecondColon != std::string::npos)
                                  ? strRest.substr(nSecondColon + 1)
                                  : "";

            /* Only apply if username and password are both non-empty. */
            if(!strUser.empty() && !strPass.empty())
            {
                strLoginUser = strUser;
                strLoginPass = strPass;
                if(!strPIN2.empty())
                    strLoginPIN = strPIN2;
            }
        }
    }

    /* Create a JSON encoding and call the main API endpoint. */
    encoding::json jParams =
    {
        { "username", strLoginUser },
        { "password", strLoginPass },
        { "pin",      strLoginPIN  }
    };

    /* Handle for -autocreate if specified. */
    std::string strCreate = "create/master";
    if(config::GetBoolArg("-autocreate", false))
    {
        try { TAO::API::Commands::Invoke("profiles", strCreate, jParams); }
        catch(const TAO::API::Exception& e){ debug::notice(FUNCTION, "::autocreate:", e.what()); }
    }

    /* Handle for -autologin if specified. */
    if(config::GetBoolArg("-autologin", false))
    {
        try
        {
            /* Create our local session first. */
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== AUTOLOGIN: Starting session creation ===", ANSI_COLOR_RESET);
            std::string strLogin = "create/local";
            TAO::API::Commands::Invoke("sessions", strLogin, jParams);
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "    Session created successfully", ANSI_COLOR_RESET);

            /* Wait for dynamic indexing services. */
            debug::log(0, "    Waiting for sigchain indexing...");
            std::string strStatus = "status/local";
            uint32_t nWaitCount = 0;
            while(!config::fShutdown.load())
            {
                /* Check our current status against indexing services. */
                const encoding::json jStatus =
                    TAO::API::Commands::Invoke("sessions", strStatus, jParams);

                /* Break once we have indexed sigchain. */
                if(!jStatus["indexing"].get<bool>()) //basic spin-lock
                {
                    debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "    Indexing complete (waited ", nWaitCount, " seconds)", ANSI_COLOR_RESET);
                    break;
                }

                /* Log progress every 5 seconds */
                if(nWaitCount % 5 == 0)
                    debug::log(0, "    Still indexing... (", nWaitCount, " seconds)");

                ++nWaitCount;

                /* Make sure we don't spin at 100% of a CPU core. */
                runtime::sleep(1);
            }

            /* Create a JSON encoding and call the main API endpoint. */
            encoding::json jUnlock =
            {
                { "pin",  strLoginPIN },
                { "notifications",              "1" },
                { "mining",                     "1" },
                { "staking",                    "1" }
            };

            /* Unlock our local session now. */
            debug::log(0, "    Unlocking session for mining/staking/notifications...");
            std::string strUnlock = "unlock/local";
            TAO::API::Commands::Invoke("sessions", strUnlock, jUnlock);
            debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "    Session unlocked successfully", ANSI_COLOR_RESET);

            /* Verify Session::DEFAULT is available */
            if(TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
            {
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "    Session::DEFAULT verified (mining ready)", ANSI_COLOR_RESET);
            }
            else
            {
                debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "    Warning: Session::DEFAULT not unlocked for mining", ANSI_COLOR_RESET);
            }

            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== AUTOLOGIN: Complete ===", ANSI_COLOR_RESET);
        }
        catch(const TAO::API::Exception& e){ debug::notice(FUNCTION, "::autologin: ", e.what()); }
    }
}


/** Startup
 *
 *  Wrap all our initialization logic here that needs to run in the background after startup.
 *
 **/
void Startup()
{
    /* Add our connections from commandline. */
    LLP::MakeConnections<LLP::TimeNode>   (LLP::TIME_SERVER);
    LLP::MakeConnections<LLP::TritiumNode>(LLP::TRITIUM_SERVER);

    /* Run our autologin scripts now. */
    if(config::GetBoolArg("-autocreate", false) || config::GetBoolArg("-autologin", false))
    {
        /* Check that we are in single-user mode. */
        if(config::fMultiuser.load())
        {
            /* Output our new warning message if the API was disabled. */
            debug::log(0, ANSI_COLOR_BRIGHT_RED, "-autocreate and -autologin DISABLED", ANSI_COLOR_RESET);
            debug::log(0, ANSI_COLOR_BRIGHT_YELLOW, "You cannot use -multiuser=1 with -auto(type) arguments.", ANSI_COLOR_RESET);

            return;
        }

        /* Defer autologin until the node is fully synchronized.
         *
         * Unattended mining nodes often restart after downtime and begin a
         * resync before they are ready to sign block templates.  Creating
         * SESSION::DEFAULT while the chain is still catching up wastes index
         * work and may produce stale templates.  We wait here -- checking
         * every few seconds -- and only run autologin once sync completes.
         *
         * If the node is already synced at startup (common for nodes that
         * were only briefly offline) the check is false immediately and we
         * fall through without any sleep.
         */
        if(TAO::Ledger::ChainState::Synchronizing())
        {
            debug::log(0, "AUTOLOGIN: waiting for sync to complete before creating SESSION::DEFAULT");

            /* Poll at this interval (ms); respect shutdown signal. */
            static const uint32_t AUTOLOGIN_SYNC_POLL_INTERVAL_MS = 3000;
            while(!config::fShutdown.load() && TAO::Ledger::ChainState::Synchronizing())
                runtime::sleep(AUTOLOGIN_SYNC_POLL_INTERVAL_MS);

            /* If we woke up due to shutdown, bail out cleanly. */
            if(config::fShutdown.load())
                return;

            debug::log(0, "AUTOLOGIN: sync complete, creating SESSION::DEFAULT");
        }

        /* Run the autologin flow exactly once. */
        RunAutoLogin();
    }
}

int main(int argc, char** argv)
{
    /* Setup the timer timer. */
    runtime::timer timer;
    timer.Start();


    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Read the configuration file. Pass argc and argv for possible -datadir setting */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs, argc, argv);


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();


    /* Initalize the debug logger. */
    debug::Initialize();


    /* Handle Commandline switch */
    for(int i = 1; i < argc; ++i)
    {
        /* Handle for commandline API/RPC */
        if(!convert::IsSwitchChar(argv[i][0]))
        {
            /* As a helpful shortcut, if the method name includes a "/" then we will assume it is meant for the API. */
            const std::string strEndpoint = std::string(argv[i]);

            /* Handle for API if symbol detected. */
            if(strEndpoint.find('/') != strEndpoint.npos || config::GetBoolArg(std::string("-api")))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    if(config::HasArg("-auditblock"))
    {
        int nAuditResult = 3;
        try
        {
            nAuditResult = RunOfflineAuditBlock();
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "auditblock infrastructure failed: ", e.what());
        }
        debug::Shutdown();
        return nAuditResult;
    }


    /* Handle forensic fork analysis commands */
    if(config::GetBoolArg("-forensicforks", false) || config::GetBoolArg("-analyzeforks", false))
    {
        /* Initialize minimal subsystems for forensic analysis */
        debug::log(0, FUNCTION, "Starting forensic fork analysis...");
        
        /* Initialize LLD to access blockchain data */
        if(!LLD::Initialize())
        {
            LLD::Shutdown();
            debug::Shutdown();
            return 1;
        }
        
        /* Initialize ChainState to access block data */
        if(!TAO::Ledger::ChainState::Initialize())
        {
            LLD::Shutdown();
            debug::Shutdown();
            return 1;
        }
        
        /* Run comprehensive forensic analysis */
        LLP::ForensicForkInfo info = LLP::ChannelStateManager::AnalyzeChannelHeightDiscrepancy();
        
        /* Shutdown and exit */
        LLD::Shutdown();
        debug::Shutdown();
        
        return 0;
    }
    
    /* Handle channel statistics output */
    if(config::GetBoolArg("-channelstats", false))
    {
        /* Initialize minimal subsystems for stats */
        debug::log(0, FUNCTION, "Retrieving channel height statistics...");
        
        /* Initialize LLD to access blockchain data */
        if(!LLD::Initialize())
        {
            LLD::Shutdown();
            debug::Shutdown();
            return 1;
        }
        
        /* Initialize ChainState to access block data */
        if(!TAO::Ledger::ChainState::Initialize())
        {
            LLD::Shutdown();
            debug::Shutdown();
            return 1;
        }
        
        /* Get and output statistics */
        std::string strStats = LLP::ChannelStateManager::GetChannelHeightStatistics();
        debug::log(0, "\n", strStats);
        
        /* Shutdown and exit */
        LLD::Shutdown();
        debug::Shutdown();
        
        return 0;
    }


    /* Log the startup information now. */
    debug::LogStartup(argc, argv);


    /* Run the process as Daemon RPC/LLP Server if Flagged. */
    if(config::fDaemon)
    {
        debug::log(0, FUNCTION, "-daemon flag enabled. Running in background");
        Daemonize();
    }


    /* Create directories if they don't exist yet. */
    if(!filesystem::exists(config::GetDataDir()) &&
        filesystem::create_directory(config::GetDataDir()))
    {
        debug::log(0, FUNCTION, "Generated Path ", config::GetDataDir());
    }


    /* Check for failures or shutdown. */
    bool fFailed = config::fShutdown.load();
    if(!fFailed)
    {
        /* Initialize LLD. */
        if(!LLD::Initialize())
        {
            config::fShutdown.store(true);
            fFailed = true;
        }

        if(!fFailed)
        {
            /* Initialize dispatch relays. */
            TAO::Ledger::Dispatch::Initialize();


            /* Initialize ChainState. */
            if(!TAO::Ledger::ChainState::Initialize())
            {
                TAO::Ledger::Dispatch::Shutdown();
                LLD::Shutdown();
                debug::Shutdown();
                return 1;
            }


            /* Run our LLD indexing operations. */
            LLD::Indexing();


            /* Initialize Legacy Environment. */
            if(!Legacy::Initialize())
            {
                config::fShutdown.store(true);
                fFailed = true;
            }


            /* Initialize the Lower Level Protocol. */
            LLP::Initialize();


            /* Startup performance metric. */
            debug::log(0, FUNCTION, "Started up in ", timer.ElapsedMilliseconds(), "ms");


            /* Set the initialized flags. */
            config::fInitialized.store(true);


            /* Kick off our startup thread for post-startup processing. */
            std::thread tStartup = std::thread(Startup);


            /* Initialize generator thread. */
            std::thread thread;
            if(config::fHybrid.load())
                thread = std::thread(TAO::Ledger::ThreadGenerator);


            /* Wait for shutdown. */
            if(!config::GetBoolArg(std::string("-gdb")))
            {
                std::mutex SHUTDOWN_MUTEX;
                std::unique_lock<std::mutex> SHUTDOWN_LOCK(SHUTDOWN_MUTEX);
                SHUTDOWN.wait(SHUTDOWN_LOCK, []{ return config::fShutdown.load(); });
            }


            /* GDB mode waits for keyboard input to initiate clean shutdown. */
            else
            {
                getchar();
                config::fShutdown = true;
            }


            /* Wait for our startup thread to finish. */
            tStartup.join();


            /* Stop stake minter if running. Minter ignores request if not running, so safe to just call both */
            TAO::Ledger::StakeMinter::GetInstance().Stop();


            /* Wait for the private condition. */
            if(config::fHybrid.load())
            {
                TAO::Ledger::PRIVATE_CONDITION.notify_all();
                thread.join();
            }


            /* Shutdown dispatch. */
            TAO::Ledger::Dispatch::Shutdown();
        }
    }


    /* Shutdown metrics. */
    timer.Reset();


    /* Release network triggers. */
    LLP::Release();


    /* Shutdown the API subsystems. */
    TAO::API::Shutdown();


    /* Shutdown network subsystem. */
    LLP::Shutdown();


    /* Shutdown LLL sub-systems. */
    LLD::Shutdown();


    /* We check failed here as wallet could have cause failed startup via the fShutdown flag. */
    if(!fFailed)
        Legacy::Shutdown();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", timer.ElapsedMilliseconds(), "ms");


    /* Close the debug log file once and for all. */
    debug::Shutdown();


    return fFailed ? 1 : 0;
}

/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

 [Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#include "../main.h"
#include "../wallet/db.h"
#include "../core/unifiedtime.h"
#include "../wallet/walletdb.h"
#include "../util/ui_interface.h"

#include "rpcserver.h"
#include "net.h"

#include "../LLD/index.h"
#include "../LLP/miningserver.h"

#undef printf
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem/fstream.hpp>


#define printf OutputDebugStringF
// MinGW 3.4.5 gets "fatal error: had to relocate PCH" if the json headers are
// precompiled in headers.h.  The problem might be when the pch file goes over
// a certain size around 145MB.  If we need access to json_spirit outside this
// file, we could use the compiled json_spirit option.

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

namespace Net
{
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;

    void ThreadRPCServer2(void* parg);

    // Key used by getwork/getblocktemplate miners.
    // Allocated in StartRPCThreads, free'd in StopRPCThreads


    static std::string strRPCUserColonPass;

    static int64_t nWalletUnlockTime;
    static CCriticalSection cs_nWalletUnlockTime;

    extern Value dumpprivkey(const Array& params, bool fHelp);
    extern Value importprivkey(const Array& params, bool fHelp);

    extern Value exportkeys(const Array& params, bool fHelp);
    extern Value importkeys(const Array& params, bool fHelp);
    extern Value rescan(const Array& params, bool fHelp);


    Object JSONRPCError(int code, const string& message)
    {
        Object error;
        error.push_back(Pair("code", code));
        error.push_back(Pair("message", message));
        return error;
    }


    int64_t AmountFromValue(const Value& value)
    {
        double dAmount = value.get_real();
        if (dAmount <= 0.0 || dAmount > Core::MAX_TXOUT_AMOUNT)
            throw JSONRPCError(-3, "Invalid amount");
        int64_t nAmount = roundint64(dAmount * COIN);
        if (!Core::MoneyRange(nAmount))
            throw JSONRPCError(-3, "Invalid amount");
        return nAmount;
    }

    Value ValueFromAmount(int64_t amount)
    {
        return (double)amount / (double)COIN;
    }

    std::string
    HexBits(unsigned int nBits)
    {
        union {
            int32_t nBits;
            char cBits[4];
        } uBits;
        uBits.nBits = htonl((int32_t)nBits);
        return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
    }

    void WalletTxToJSON(const Wallet::CWalletTx& wtx, Object& entry)
    {
        int confirms = wtx.GetDepthInMainChain();
        entry.push_back(Pair("confirmations", confirms));
        if (confirms)
        {
            entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
            entry.push_back(Pair("blockindex", wtx.nIndex));
        }
        entry.push_back(Pair("txid", wtx.GetHash().GetHex()));
        entry.push_back(Pair("time", (boost::int64_t)wtx.GetTxTime()));
        BOOST_FOREACH(const PAIRTYPE(string,string)& item, wtx.mapValue)
            entry.push_back(Pair(item.first, item.second));
    }

    string AccountFromValue(const Value& value)
    {
        string strAccount = value.get_str();
        if (strAccount == "*")
            throw JSONRPCError(-11, "Invalid account name");
        return strAccount;
    }

    Object blockToJSON(const Core::CBlock& block, const Core::CBlockIndex* blockindex, bool fPrintTransactionDetail)
    {
        Object result;
        result.push_back(Pair("hash", block.GetHash().GetHex()));
        result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
        result.push_back(Pair("height", (int)blockindex->nHeight));
        result.push_back(Pair("channel", (int)block.nChannel));
        result.push_back(Pair("version", (int)block.nVersion));
        result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
        result.push_back(Pair("time", DateTimeStrFormat(block.GetBlockTime())));
        result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
        result.push_back(Pair("bits", HexBits(block.nBits)));
        result.push_back(Pair("difficulty", Core::GetDifficulty(blockindex->nBits, blockindex->GetChannel())));
        result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
        if (blockindex->pprev)
            result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
        if (blockindex->pnext)
            result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

        Array txinfo;
        BOOST_FOREACH (const Core::CTransaction& tx, block.vtx)
        {
            if (fPrintTransactionDetail)
            {
                txinfo.push_back(tx.ToStringShort());
                txinfo.push_back(DateTimeStrFormat(tx.nTime));
                BOOST_FOREACH(const Core::CTxIn& txin, tx.vin)
                    txinfo.push_back(txin.ToStringShort());
                BOOST_FOREACH(const Core::CTxOut& txout, tx.vout)
                    txinfo.push_back(txout.ToStringShort());
            }
            else
                txinfo.push_back(tx.GetHash().GetHex());
        }
        result.push_back(Pair("tx", txinfo));
        return result;
    }



    ///
    /// Note: This interface may still be subject to change.
    ///

    string CRPCTable::help(string strCommand) const
    {
        string strRet;
        set<rpcfn_type> setDone;
        for (map<string, const CRPCCommand*>::const_iterator mi = mapCommands.begin(); mi != mapCommands.end(); ++mi)
        {
            const CRPCCommand *pcmd = mi->second;
            string strMethod = mi->first;
            // We already filter duplicates, but these deprecated screw up the sort order
            if (strMethod == "getamountreceived" ||
                strMethod == "getallreceived" ||
                strMethod == "getblocknumber" || // deprecated
                (strMethod.find("label") != string::npos))
                continue;
            if (strCommand != "" && strMethod != strCommand)
                continue;
            try
            {
                Array params;
                rpcfn_type pfn = pcmd->actor;
                if (setDone.insert(pfn).second)
                    (*pfn)(params, true);
            }
            catch (std::exception& e)
            {
                // Help text is returned in an exception
                string strHelp = string(e.what());
                if (strCommand == "")
                    if (strHelp.find('\n') != string::npos)
                        strHelp = strHelp.substr(0, strHelp.find('\n'));
                strRet += strHelp + "\n";
            }
        }
        if (strRet == "")
            strRet = strprintf("help: unknown command: %s\n", strCommand.c_str());
        strRet = strRet.substr(0,strRet.size()-1);
        return strRet;
    }

    Value help(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 1)
            throw runtime_error(
                "help [command]\n"
                "List commands, or get help for a command.");

        string strCommand;
        if (params.size() > 0)
            strCommand = params[0].get_str();

        return tableRPC.help(strCommand);
    }

    // Adding the getRPCMapComands function.
    // string CRPCTable::help(string strCommand) const
    std::vector<std::string> CRPCTable::getMapCommandsKeyVector() const
    {
        std::vector<std::string> rpcTableKeys; // a standard vector of strings...
        for(map<string, const CRPCCommand*>::const_iterator it = this->mapCommands.begin(); it!= this->mapCommands.end(); ++it) {
            rpcTableKeys.push_back(it->first);
        }
        return rpcTableKeys;
    }

    Value getMapCommandsKeyVector(const Array& params, bool fHelp) {
        // if (fHelp || params.size() != 0)
        //     throw runtime_error(
        //         "getMapCommandsKeyVector\n"
        //         "returns a Vector of the keys for the RPC");
        // std::stringstream result;
        // std::copy(tableRPC.getMapCommandsKeyVector().begin(), tableRPC.getMapCommandsKeyVector().end(), std::ostream_iterator<string>(result, ", "));
        Array ret;
        BOOST_FOREACH(const std::string it, tableRPC.getMapCommandsKeyVector())
        {
            ret.push_back(it);
        }

        return ret;
    }


    Value stop(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "stop\n"
                "Stop Nexus server.");
        // Shutdown will take long enough that the response should get back
        StartShutdown();
        return "Nexus server stopping";
    }

    Value getnetworkhashps(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getnetworkhashps\n"
                "Get network hashrate for the hashing channel.");

        /* This constant was determined by finding the time it takes to find hash of given difficulty at a given hash rate.
         * It is the total hashes per second required to find a hash of difficulty 1.0 every second.
         * This can then be used in calculing the network hash rate by block times over average of 1440 blocks.
         * */
        uint64_t nTimeConstant = 276758250000;

        const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 2);
        unsigned int nAverageTime = 0, nTotal = 0;
        double nAverageDifficulty = 0.0;

        for( ; nTotal < 1440 && pindex->pprev; nTotal ++) {

            nAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 2)->GetBlockTime());
            nAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 2));

            pindex = Core::GetLastChannelIndex(pindex->pprev, 2);
        }

        if(nTotal == 0)
        throw runtime_error("getnetworkhashps\n"
                            "No Blocks produced on Hashing Channel.");

        nAverageTime       /= nTotal;
        nAverageDifficulty /= nTotal;
        /* TODO END **/

        Object obj;
        obj.push_back(Pair("average time", (int) nAverageTime));
        obj.push_back(Pair("average difficulty", nAverageDifficulty));

        /* Calculate the hashrate based on nTimeConstant. */
        uint64_t nHashRate = (nTimeConstant / nAverageTime) * nAverageDifficulty;

        obj.push_back(Pair("hashrate", (boost::uint64_t)nHashRate));

        return obj;
    }


    Value getnetworkpps(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getnetworkpps\n"
                "Get network prime searched per second.");

        /* This constant was determined by finding the time it takes to find primes at given difficulty and Primes Per Second'
         * This constant was found that 2,450 Prime Per Second is required to find 1 3ch Cluster per Second.
         * The difficulty changes are exponential or in other words require 50x more work per difficulty increase
         */
        unsigned int nTimeConstant = 2480;

        const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 1);
        unsigned int nAverageTime = 0, nTotal = 0;
        double nAverageDifficulty = 0.0;

        for( ; nTotal < 1440 && pindex->pprev; nTotal ++) {

            nAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 1)->GetBlockTime());
            nAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 1));

            pindex = Core::GetLastChannelIndex(pindex->pprev, 1);
        }

        nAverageTime       /= nTotal;
        nAverageDifficulty /= nTotal;
        /* TODO END **/

        Object obj;
        obj.push_back(Pair("average time", (int) nAverageTime));
        obj.push_back(Pair("average difficulty", nAverageDifficulty));

        /* Calculate the hashrate based on nTimeConstant. Set the reference from 3ch and
         * Above since 1x and 2x were never useful starting difficulties. */
        uint64_t nHashRate = (nTimeConstant / nAverageTime) * std::pow(50.0, (nAverageDifficulty - 3.0));

        obj.push_back(Pair("primes per second", (boost::uint64_t)nHashRate));

        return obj;
    }

    Value getnetworktrustkeys(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getnetworktrustkeys\n"
                "List all the Trust Keys on the Network");


        unsigned int nTotalActive = 0;
        Array trustkeys;
        Object ret;
        for(std::map<uint576, Core::CTrustKey>::iterator it = Core::cTrustPool.mapTrustKeys.begin(); it != Core::cTrustPool.mapTrustKeys.end(); ++it)
        {
            Object obj;

            if(it->second.Expired(0, Core::pindexBest->pprev->GetBlockHash()))
                continue;

            /** Check the Wallet and Trust Keys in Trust Pool to see if we own any keys. **/
            Wallet::NexusAddress address;
            address.SetPubKey(it->second.vchPubKey);

            obj.push_back(Pair("address", address.ToString()));
            obj.push_back(Pair("interest rate", 100.0 * Core::cTrustPool.InterestRate(it->first, GetUnifiedTimestamp())));
            obj.push_back(Pair("trust key", it->second.ToString()));

            nTotalActive ++;

            trustkeys.push_back(obj);
        }

        ret.push_back(Pair("keys", trustkeys));
        ret.push_back(Pair("total active", (int) nTotalActive));
        ret.push_back(Pair("total expired", (int) (Core::cTrustPool.mapTrustKeys.size() - nTotalActive)));

        return ret;
    }


    Value getblockcount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getblockcount\n"
                "Returns the number of blocks in the longest block chain.");

        return (int)Core::nBestHeight;
    }


    // deprecated
    Value getblocknumber(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getblocknumber\n"
                "Deprecated.  Use getblockcount.");

        return (int)Core::nBestHeight;
    }


    Value getconnectioncount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getconnectioncount\n"
                "Returns the number of connections to other nodes.");

        LOCK(cs_vNodes);
        return (int)vNodes.size();
    }

    static void CopyNodeStats(std::vector<CNodeStats>& vstats)
    {
        vstats.clear();

        LOCK(cs_vNodes);
        vstats.reserve(vNodes.size());
        BOOST_FOREACH(CNode* pnode, vNodes) {
            CNodeStats stats;
            pnode->copyStats(stats);
            vstats.push_back(stats);
        }
    }

    Value getpeerinfo(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getpeerinfo\n"
                "Returns data about each connected network node.");

        vector<CNodeStats> vstats;
        CopyNodeStats(vstats);

        Array ret;

        BOOST_FOREACH(const CNodeStats& stats, vstats) {
            Object obj;

            obj.push_back(Pair("addr", stats.addrName));
            obj.push_back(Pair("services", strprintf("%08" PRI64x, stats.nServices)));
            obj.push_back(Pair("lastsend", (boost::int64_t)stats.nLastSend));
            obj.push_back(Pair("lastrecv", (boost::int64_t)stats.nLastRecv));
            obj.push_back(Pair("conntime", (boost::int64_t)stats.nTimeConnected));
            obj.push_back(Pair("version", stats.nVersion));
            obj.push_back(Pair("subver", stats.strSubVer));
            obj.push_back(Pair("inbound", stats.fInbound));
            obj.push_back(Pair("releasetime", (boost::int64_t)stats.nReleaseTime));
            obj.push_back(Pair("height", stats.nStartingHeight));
            obj.push_back(Pair("banscore", stats.nMisbehavior));

            ret.push_back(obj);
        }

        return ret;
    }


    Value getdifficulty(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getdifficulty\n"
                "Returns difficulty as a multiple of the minimum difficulty.");

        const Core::CBlockIndex* pindexPOS = Core::GetLastChannelIndex(Core::pindexBest, 0);
        const Core::CBlockIndex* pindexCPU = Core::GetLastChannelIndex(Core::pindexBest, 1);
        const Core::CBlockIndex* pindexGPU = Core::GetLastChannelIndex(Core::pindexBest, 2);


        Object obj;
        obj.push_back(Pair("prime - channel",        Core::GetDifficulty(pindexCPU->nBits, 1)));
        obj.push_back(Pair("hash  - channel",        Core::GetDifficulty(pindexGPU->nBits, 2)));

        obj.push_back(Pair("proof-of-stake",       Core::GetDifficulty(pindexPOS->nBits, 0)));
        return obj;
    }

    Value getsupplyrates(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getsupplyrates\n"
                "Returns an object containing current Nexus production rates in set time intervals.\n"
                "Time Frequency is in base 13 month, 28 day totalling 364 days.\n"
                "This is to prevent error from Gregorian Figures.");

        Object obj;
        unsigned int nMinutes = Core::GetChainAge(Core::pindexBest->GetBlockTime());

        obj.push_back(Pair("chainAge",       (int)nMinutes));

        int64_t nSupply = Core::pindexBest->nMoneySupply;
        int64_t nTarget = Core::CompoundSubsidy(nMinutes);

        obj.push_back(Pair("moneysupply",   ValueFromAmount(nSupply)));
        obj.push_back(Pair("targetsupply",   ValueFromAmount(nTarget)));
        obj.push_back(Pair("inflationrate",   ((nSupply * 100.0) / nTarget) - 100.0));

        obj.push_back(Pair("minuteSupply",  ValueFromAmount(Core::SubsidyInterval(nMinutes, 1)))); //1
        obj.push_back(Pair("hourSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 60)))); //60
        obj.push_back(Pair("daySupply",     ValueFromAmount(Core::SubsidyInterval(nMinutes, 1440)))); //1440
        obj.push_back(Pair("weekSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 10080)))); //10080
        obj.push_back(Pair("monthSupply",   ValueFromAmount(Core::SubsidyInterval(nMinutes, 40320)))); //40320
        obj.push_back(Pair("yearSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 524160)))); //524160

        return obj;
    }

    Value getmoneysupply(const Array& params, bool fHelp)
    {
        if(fHelp || params.size() != 0)
            throw runtime_error(
                "getmoneysupply <timestamp>\n\n"
                "Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors.\n"
                "Default timestamp is the current Unified Timestamp. The timestamp is recorded as a UNIX timestamp");

        Object obj;
        unsigned int nMinutes = Core::GetChainAge(Core::pindexBest->GetBlockTime());

        obj.push_back(Pair("chainAge",       (int)nMinutes));
        obj.push_back(Pair("miners", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 0))));
        obj.push_back(Pair("ambassadors", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 1))));
        obj.push_back(Pair("developers", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 2))));

        return obj;
    }

    Value getinfo(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getinfo\n"
                "Returns an object containing various state info.");

        Object obj;
        obj.push_back(Pair("version",       FormatFullVersion()));
        obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
        obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
        obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
        obj.push_back(Pair("newmint",       ValueFromAmount(pwalletMain->GetNewMint())));
        obj.push_back(Pair("stake",         ValueFromAmount(pwalletMain->GetStake())));

        double dPercent = ((double)Core::dTrustWeight + (double)Core::dBlockWeight) / 37.5;
        obj.push_back(Pair("interestweight", (double)Core::dInterestRate * 100.0));
        obj.push_back(Pair("stakeweight",    dPercent * 100.0));
        obj.push_back(Pair("trustweight",    (double)Core::dTrustWeight * 100.0 / 17.5));
        obj.push_back(Pair("blockweight",    (double)Core::dBlockWeight * 100.0  / 20.0));
        obj.push_back(Pair("txtotal",        (int)pwalletMain->mapWallet.size()));

        obj.push_back(Pair("blocks",        (int)Core::nBestHeight));
        obj.push_back(Pair("timestamp", (int)GetUnifiedTimestamp()));

        obj.push_back(Pair("connections",   (int)vNodes.size()));
        obj.push_back(Pair("proxy",         (fUseProxy ? addrProxy.ToStringIPPort() : string())));
        obj.push_back(Pair("ip",            addrSeenByPeer.ToStringIP()));

        obj.push_back(Pair("testnet",       fTestNet));
        obj.push_back(Pair("keypoololdest", (boost::int64_t)pwalletMain->GetOldestKeyPoolTime()));
        obj.push_back(Pair("keypoolsize",   pwalletMain->GetKeyPoolSize()));
        obj.push_back(Pair("paytxfee",      ValueFromAmount(Core::nTransactionFee)));
        if (pwalletMain->IsCrypted())
            obj.push_back(Pair("unlocked_until", (boost::int64_t)nWalletUnlockTime / 1000));
        obj.push_back(Pair("errors",        Core::GetWarnings("statusbar")));
        return obj;
    }


    Value getmininginfo(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getmininginfo\n"
                "Returns an object containing mining-related information.");

        uint64_t nPrimePS = 0;
        if(!Core::pindexBest || Core::pindexBest->GetBlockHash() != Core::hashGenesisBlock)
        {
            double nPrimeAverageDifficulty = 0.0;
            unsigned int nPrimeAverageTime = 0;
            unsigned int nPrimeTimeConstant = 2480;
            int nTotal = 0;
            const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 1);
            for(; (nTotal < 1440 && pindex->pprev); nTotal ++) {

                nPrimeAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 1)->GetBlockTime());
                nPrimeAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 1));

                pindex = Core::GetLastChannelIndex(pindex->pprev, 1);
            }
            nPrimeAverageDifficulty /= nTotal;
            nPrimeAverageTime /= nTotal;
            nPrimePS = (nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0));
        }

        // Hash
        int nHTotal = 0;
        unsigned int nHashAverageTime = 0;
        double nHashAverageDifficulty = 0.0;
        uint64_t nTimeConstant = 276758250000;
        const Core::CBlockIndex* hindex = Core::GetLastChannelIndex(Core::pindexBest, 2);
        for(;  (nHTotal < 1440 && hindex->pprev); nHTotal ++) {

            nHashAverageTime += (hindex->GetBlockTime() - Core::GetLastChannelIndex(hindex->pprev, 2)->GetBlockTime());
            nHashAverageDifficulty += (Core::GetDifficulty(hindex->nBits, 2));

            hindex = Core::GetLastChannelIndex(hindex->pprev, 2);
        }
        nHashAverageDifficulty /= nHTotal;
        nHashAverageTime /= nHTotal;

        uint64_t nHashRate = (nTimeConstant / nHashAverageTime) * nHashAverageDifficulty;


        Object obj;
        obj.push_back(Pair("blocks",        (int)Core::nBestHeight));
        obj.push_back(Pair("timestamp", (int)GetUnifiedTimestamp()));

        obj.push_back(Pair("currentblocksize",(uint64_t)Core::nLastBlockSize));
        obj.push_back(Pair("currentblocktx",(uint64_t)Core::nLastBlockTx));

        const Core::CBlockIndex* pindexCPU = Core::GetLastChannelIndex(Core::pindexBest, 1);
        const Core::CBlockIndex* pindexGPU = Core::GetLastChannelIndex(Core::pindexBest, 2);
        obj.push_back(Pair("primeDifficulty",       Core::GetDifficulty(Core::GetNextTargetRequired(Core::pindexBest, 1, false), 1)));
        obj.push_back(Pair("hashDifficulty",        Core::GetDifficulty(Core::GetNextTargetRequired(Core::pindexBest, 2, false), 2)));
        obj.push_back(Pair("primeReserve",           ValueFromAmount(pindexCPU->nReleasedReserve[0])));
        obj.push_back(Pair("hashReserve",            ValueFromAmount(pindexGPU->nReleasedReserve[0])));
        obj.push_back(Pair("primeValue",               ValueFromAmount(Core::GetCoinbaseReward(Core::pindexBest, 1, 0))));
        obj.push_back(Pair("hashValue",                ValueFromAmount(Core::GetCoinbaseReward(Core::pindexBest, 2, 0))));
        obj.push_back(Pair("pooledtx",              (boost::uint64_t)Core::mempool.size()));
        obj.push_back(Pair("primesPerSecond",         (boost::uint64_t)nPrimePS));
        obj.push_back(Pair("hashPerSecond",         (boost::uint64_t)nHashRate));

        if(GetBoolArg("-mining", false))
        {
            obj.push_back(Pair("totalConnections", LLP::MINING_LLP->TotalConnections()));
        }


        return obj;
    }


    Value getnewaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 1)
            throw runtime_error(
                "getnewaddress [account]\n"
                "Returns a new Nexus address for receiving payments.  "
                "If [account] is specified (recommended), it is added to the address book "
                "so payments received with the address will be credited to [account].");

        // Parse the account first so we don't generate a key if there's an error
        string strAccount;
        if (params.size() > 0)
            strAccount = AccountFromValue(params[0]);

        if (!pwalletMain->IsLocked())
            pwalletMain->TopUpKeyPool();

        // Generate a new key that is added to wallet
        std::vector<unsigned char> newKey;
        if (!pwalletMain->GetKeyFromPool(newKey, false))
            throw JSONRPCError(-12, "Error: Keypool ran out, please call keypoolrefill first");
        Wallet::NexusAddress address(newKey);

        pwalletMain->SetAddressBookName(address, strAccount);

        return address.ToString();
    }


    Wallet::NexusAddress GetAccountAddress(string strAccount, bool bForceNew=false)
    {
        Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);

        Wallet::CAccount account;
        walletdb.ReadAccount(strAccount, account);

        bool bKeyUsed = false;

        // Check if the current key has been used
        if (!account.vchPubKey.empty())
        {
            Wallet::CScript scriptPubKey;
            scriptPubKey.SetNexusAddress(account.vchPubKey);
            for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
                 it != pwalletMain->mapWallet.end() && !account.vchPubKey.empty();
                 ++it)
            {
                const Wallet::CWalletTx& wtx = (*it).second;
                BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
                    if (txout.scriptPubKey == scriptPubKey)
                        bKeyUsed = true;
            }
        }

        // Generate a new key
        if (account.vchPubKey.empty() || bForceNew || bKeyUsed)
        {
            if (!pwalletMain->GetKeyFromPool(account.vchPubKey, false))
                throw JSONRPCError(-12, "Error: Keypool ran out, please call keypoolrefill first");

            pwalletMain->SetAddressBookName(Wallet::NexusAddress(account.vchPubKey), strAccount);
            walletdb.WriteAccount(strAccount, account);
        }

        return Wallet::NexusAddress(account.vchPubKey);
    }

    Value getaccountaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getaccountaddress <account>\n"
                "Returns the current Nexus address for receiving payments to this account.");

        // Parse the account first so we don't generate a key if there's an error
        string strAccount = AccountFromValue(params[0]);

        Value ret;

        ret = GetAccountAddress(strAccount).ToString();

        return ret;
    }



    Value setaccount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "setaccount <Nexusaddress> <account>\n"
                "Sets the account associated with the given address.");

        Wallet::NexusAddress address(params[0].get_str());
        if (!address.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");


        string strAccount;
        if (params.size() > 1)
            strAccount = AccountFromValue(params[1]);

        // Detect when changing the account of an address that is the 'unused current key' of another account:
        if (pwalletMain->mapAddressBook.count(address))
        {
            string strOldAccount = pwalletMain->mapAddressBook[address];
            if (address == GetAccountAddress(strOldAccount))
                GetAccountAddress(strOldAccount, true);
        }

        pwalletMain->SetAddressBookName(address, strAccount);

        return Value::null;
    }


    Value getaccount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getaccount <Nexusaddress>\n"
                "Returns the account associated with the given address.");

        Wallet::NexusAddress address(params[0].get_str());
        if (!address.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");

        string strAccount;
        map<Wallet::NexusAddress, string>::iterator mi = pwalletMain->mapAddressBook.find(address);
        if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.empty())
            strAccount = (*mi).second;
        return strAccount;
    }


    Value getaddressesbyaccount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getaddressesbyaccount <account>\n"
                "Returns the list of addresses for the given account.");

        string strAccount = AccountFromValue(params[0]);

        // Find all addresses that have the given account
        Array ret;
        BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
        {
            const Wallet::NexusAddress& address = item.first;
            const string& strName = item.second;
            if (strName == strAccount)
                ret.push_back(address.ToString());
        }
        return ret;
    }

    Value settxfee(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < Core::MIN_TX_FEE)
            throw runtime_error(
                "settxfee <amount>\n"
                "<amount> is a real and is rounded to 0.01 (cent)\n"
                "Minimum and default transaction fee per KB is 1 cent");

        Core::nTransactionFee = AmountFromValue(params[0]);
        Core::nTransactionFee = (Core::nTransactionFee / CENT) * CENT;  // round to cent
        return true;
    }

    Value sendtoaddress(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
            throw runtime_error(
                "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]\n"
                "<amount> is a real and is rounded to the nearest 0.000001\n"
                "requires wallet passphrase to be set with walletpassphrase first");
        if (!pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
            throw runtime_error(
                "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]\n"
                "<amount> is a real and is rounded to the nearest 0.000001");

        Wallet::NexusAddress address(params[0].get_str());
        if (!address.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");

        // Amount
        int64_t nAmount = AmountFromValue(params[1]);
        if (nAmount < Core::MIN_TXOUT_AMOUNT)
            throw JSONRPCError(-101, "Send amount too small");

        // Wallet comments
        Wallet::CWalletTx wtx;
        if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
            wtx.mapValue["comment"] = params[2].get_str();
        if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
            wtx.mapValue["to"]      = params[3].get_str();

        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        string strError = pwalletMain->SendToNexusAddress(address, nAmount, wtx);
        if (strError != "")
            throw JSONRPCError(-4, strError);

        return wtx.GetHash().GetHex();
    }

    Value signmessage(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 2)
            throw runtime_error(
                "signmessage <Nexusaddress> <message>\n"
                "Sign a message with the private key of an address");

        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        string strAddress = params[0].get_str();
        string strMessage = params[1].get_str();

        Wallet::NexusAddress addr(strAddress);
        if (!addr.IsValid())
            throw JSONRPCError(-3, "Invalid address");

        Wallet::CKey key;
        if (!pwalletMain->GetKey(addr, key))
            throw JSONRPCError(-4, "Private key not available");

        CDataStream ss(SER_GETHASH, 0);
        ss << Core::strMessageMagic;
        ss << strMessage;

        vector<unsigned char> vchSig;
        if (!key.SignCompact(SK256(ss.begin(), ss.end()), vchSig))
            throw JSONRPCError(-5, "Sign failed");

        return EncodeBase64(&vchSig[0], vchSig.size());
    }

    Value verifymessage(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 3)
            throw runtime_error(
                "verifymessage <Nexusaddress> <signature> <message>\n"
                "Verify a signed message");

        string strAddress  = params[0].get_str();
        string strSign     = params[1].get_str();
        string strMessage  = params[2].get_str();

        Wallet::NexusAddress addr(strAddress);
        if (!addr.IsValid())
            throw JSONRPCError(-3, "Invalid address");

        bool fInvalid = false;
        vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

        if (fInvalid)
            throw JSONRPCError(-5, "Malformed base64 encoding");

        CDataStream ss(SER_GETHASH, 0);
        ss << Core::strMessageMagic;
        ss << strMessage;

        Wallet::CKey key;
        if (!key.SetCompactSignature(SK256(ss.begin(), ss.end()), vchSig))
            return false;

        return (Wallet::NexusAddress(key.GetPubKey()) == addr);
    }


    Value getreceivedbyaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "getreceivedbyaddress <Nexusaddress> [minconf=1]\n"
                "Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations.");

        // Nexus address
        Wallet::NexusAddress address = Wallet::NexusAddress(params[0].get_str());
        Wallet::CScript scriptPubKey;
        if (!address.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");
        scriptPubKey.SetNexusAddress(address);
        if (!IsMine(*pwalletMain,scriptPubKey))
            return (double)0.0;

        // Minimum confirmations
        int nMinDepth = 1;
        if (params.size() > 1)
            nMinDepth = params[1].get_int();

        // Tally
        int64_t nAmount = 0;
        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const Wallet::CWalletTx& wtx = (*it).second;
            if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                continue;

            BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    if (wtx.GetDepthInMainChain() >= nMinDepth)
                        nAmount += txout.nValue;
        }

        return  ValueFromAmount(nAmount);
    }


    void GetAccountAddresses(string strAccount, set<Wallet::NexusAddress>& setAddress)
    {
        BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
        {
            const Wallet::NexusAddress& address = item.first;
            const string& strName = item.second;
            if (strName == strAccount)
                setAddress.insert(address);
        }
    }


    Value getreceivedbyaccount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "getreceivedbyaccount <account> [minconf=1]\n"
                "Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.");

        // Minimum confirmations
        int nMinDepth = 1;
        if (params.size() > 1)
            nMinDepth = params[1].get_int();

        // Get the set of pub keys assigned to account
        string strAccount = AccountFromValue(params[0]);
        set<Wallet::NexusAddress> setAddress;
        GetAccountAddresses(strAccount, setAddress);

        // Tally
        int64_t nAmount = 0;
        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const Wallet::CWalletTx& wtx = (*it).second;
            if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                continue;

            BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
            {
                Wallet::NexusAddress address;
                if (ExtractAddress(txout.scriptPubKey, address) && pwalletMain->HaveKey(address) && setAddress.count(address))
                    if (wtx.GetDepthInMainChain() >= nMinDepth)
                        nAmount += txout.nValue;
            }
        }

        return (double)nAmount / (double)COIN;
    }

    /** Dump the top balances of the Rich List to RPC console. **/
    Value dumprichlist(const Array& params, bool fHelp)
    {
        if(!GetBoolArg("-richlist", false))
            throw runtime_error("please enable -richlist to use this command");

        if (fHelp || params.size() != 1)
            throw runtime_error(
                "dumprichlist <count>\n"
                "Get balances of top addresses in the Network.");

        int nCount = params[0].get_int();
        multimap<uint64_t, uint256> richList = flip_map(Core::mapAddressTransactions);

        /** Dump the Address and Values. **/
        Object entry;
        for(multimap<uint64_t, uint256>::const_reverse_iterator it = richList.rbegin(); it != richList.rend() && nCount > 0; ++it)
        {
            Wallet::NexusAddress cAddress(it->second);
            entry.push_back(Pair(cAddress.ToString(), (double)it->first / COIN));
            nCount--;
        }

        return entry;
    }


    /** Dump the top balances of the Rich List to RPC console. **/
    Value gettransactions(const Array& params, bool fHelp)
    {
        if(!GetBoolArg("-richlist", false))
            throw runtime_error("please enable -richlist to use this command");

        if (fHelp || params.size() == 0)
            throw runtime_error(
                "gettransactions [address] [block=0] [coinbase=true]\n"
                "Get transaction data of given address");

        /* Extract the address from input arguments. */
        string strAddress = params[0].get_str();
        Wallet::NexusAddress cAddress(strAddress);

        int nBlock = 0;
        if(params.size() > 1)
            nBlock = params[1].get_int();

        bool fCoinbase = true;
        if(params.size() > 2)
            fCoinbase = params[2].get_bool();

        /* List the transactions stored in the memory map. */
        std::vector<std::pair<bool, uint512> > vTransactions = Core::mapRichList[cAddress.GetHash256()];

        Array entry;
        for(auto tx : vTransactions)
        {
            if(fCoinbase && tx.first)
                continue;

            uint1024 hashBlock = 0;
            Core::CTransaction TX;
            if (!Core::GetTransaction(tx.second, TX, hashBlock))
                throw JSONRPCError(-5, "No information available about transaction");

            if(nBlock > 0 && Core::mapBlockIndex[hashBlock]->nHeight < nBlock)
                continue;

            Object obj;
            obj.push_back(Pair("type", tx.first ? "debit" : "credit"));
            obj.push_back(Pair("txid", tx.second.ToString()));
            obj.push_back(Pair("time", (int)TX.nTime));
            obj.push_back(Pair("amount", ValueFromAmount(TX.GetValueOut())));

            entry.push_back(obj);
        }

        return entry;
    }


    /** Dump the top balances of the Rich List to RPC console. **/
    Value totaltransactions(const Array& params, bool fHelp)
    {
        if(!GetBoolArg("-richlist", false))
            throw runtime_error("please enable -richlist to use this command");

        if (fHelp)
            throw runtime_error(
                "totaltransactions [coinbase=true] [address]\n"
                "Get the total global transactions since the genesis block. Optional to do by address");

        bool fCoinbase = true;
        if(params.size() > 0)
            fCoinbase = params[0].get_bool();

        /* Extract the address from input arguments. */
        unsigned int nTotalTransactions = 0, nTotalAddresses = 0;
        if(params.size() > 1)
        {
            string strAddress = params[1].get_str();
            Wallet::NexusAddress cAddress(strAddress);

            if(Core::mapRichList.count(cAddress.GetHash256()))
            {
                for(auto tx : Core::mapRichList[cAddress.GetHash256()])
                {
                    if(!fCoinbase && tx.first)
                        continue;

                    nTotalTransactions ++;
                }
            }
        }
        else
        {
            /* List the transactions stored in the memory map. */
            for (auto it : Core::mapRichList)
            {
                bool fHasCoinbase = false;
                for(auto tx : it.second)
                {
                    if(!fCoinbase && tx.first)
                    {
                        fHasCoinbase = true;
                        continue;
                    }

                    nTotalTransactions ++;
                }

                if(!fHasCoinbase)
                    nTotalAddresses++;
            }
        }

        Object entry;
        entry.push_back(Pair("transactions", (int)nTotalTransactions));
        entry.push_back(Pair("addresses", (int)nTotalAddresses));

        return entry;
    }

    /** Dump the top balances of the Rich List to RPC console. **/
    Value getaddressbalance(const Array& params, bool fHelp)
    {
        if(!GetBoolArg("-richlist", false))
            throw runtime_error("please enable -richlist to use this command");

        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getaddressbalance [address]\n"
                "Get balances of top addresses in the Network.");

        string strAddress = params[0].get_str();
        Wallet::NexusAddress cAddress(strAddress);

        if (!cAddress.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");

        /** Dump the Address and Values. **/
        Object entry;
        entry.push_back(Pair(strAddress, (double)Core::mapAddressTransactions[cAddress.GetHash256()] / COIN));


        return entry;
    }


    /** RPC Method to bridge limitation of Transaction Lookup from Wallet. Allows lookup from any wallet. **/
    Value getglobaltransaction(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getglobaltransaction [txid]\n"
                "Get detailed information about [txid]");

        uint512 hash;
        hash.SetHex(params[0].get_str());

        Core::CTxIndex txindex;
        Core::CTransaction cTransaction;

        LLD::CIndexDB indexdb("cr");
        if(!indexdb.ReadTxIndex(hash, txindex))
        {
            throw JSONRPCError(-5, "Invalid transaction id");
        }

        if(!cTransaction.ReadFromDisk(txindex.pos))
        {
            throw JSONRPCError(-5, "Failed to Read Transaction");
        }


        int confirms = txindex.GetDepthInMainChain();

        Object entry;
        entry.push_back(Pair("confirmations", confirms));
        if (confirms)
        {
            Core::CBlock pblock;
            pblock.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false);
            entry.push_back(Pair("blockhash", pblock.GetHash().GetHex()));
        }

        entry.push_back(Pair("txid", cTransaction.GetHash().GetHex()));
        entry.push_back(Pair("time", (int)cTransaction.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(cTransaction.GetValueOut())));

        Array txoutputs;
        BOOST_FOREACH(const Core::CTxOut& txout, cTransaction.vout)
        {
            Wallet::NexusAddress cAddress;
            if(!Wallet::ExtractAddress(txout.scriptPubKey, cAddress))
            {
                throw JSONRPCError(-5, "Unable to Extract Output Address");
            }

            txoutputs.push_back(strprintf("%s:%f", cAddress.ToString().c_str(), (double) txout.nValue / COIN));
        }

        if(!cTransaction.IsCoinBase())
        {
            Array txinputs;
            BOOST_FOREACH(const Core::CTxIn& txin, cTransaction.vin)
            {
                if(txin.IsStakeSig())
                    continue;

                Core::CTransaction tx;
                Core::CTxIndex txind;

                if(!indexdb.ReadTxIndex(txin.prevout.hash, txind))
                {
                    throw JSONRPCError(-5, "Invalid transaction id");
                }

                if(!tx.ReadFromDisk(txind.pos))
                {

                    throw JSONRPCError(-5, "Failed to Read Transaction");
                }

                Wallet::NexusAddress cAddress;
                if(!Wallet::ExtractAddress(tx.vout[txin.prevout.n].scriptPubKey, cAddress))
                {

                    throw JSONRPCError(-5, "Unable to Extract Input Address");
                }

                txinputs.push_back(strprintf("%s:%f", cAddress.ToString().c_str(), (double) tx.vout[txin.prevout.n].nValue / COIN));
            }

            entry.push_back(Pair("inputs", txinputs));
        }

        entry.push_back(Pair("outputs", txoutputs));

        return entry;
    }


    int64_t GetAccountBalance(Wallet::CWalletDB& walletdb, const string& strAccount, int nMinDepth)
    {
        int64_t nBalance = 0;

        // Tally wallet transactions
        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const Wallet::CWalletTx& wtx = (*it).second;
            if (!wtx.IsFinal())
                continue;

            int64_t nGenerated, nReceived, nSent, nFee;
            wtx.GetAccountAmounts(strAccount, nGenerated, nReceived, nSent, nFee);

            if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
                nBalance += nReceived;
            nBalance += nGenerated - nSent - nFee;
        }

        // Tally internal accounting entries
        nBalance += walletdb.GetAccountCreditDebit(strAccount);

        return nBalance;
    }


    int64_t GetAccountBalance(const string& strAccount, int nMinDepth)
    {
        Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);
        return GetAccountBalance(walletdb, strAccount, nMinDepth);
    }


    Value getbalance(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 2)
            throw runtime_error(
                "getbalance [account] [minconf=1]\n"
                "If [account] is not specified, returns the server's total available balance.\n"
                "If [account] is specified, returns the balance in the account.");

        if (params.size() == 0)
            return  ValueFromAmount(pwalletMain->GetBalance());

        int nMinDepth = 1;
        if (params.size() > 1)
            nMinDepth = params[1].get_int();

        if (params[0].get_str() == "*") {
            // Calculate total balance a different way from GetBalance()
            // (GetBalance() sums up all unspent TxOuts)
            // getbalance and getbalance '*' should always return the same number.
            int64_t nBalance = 0;
            for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
            {
                const Wallet::CWalletTx& wtx = (*it).second;
                if (!wtx.IsFinal())
                    continue;

                int64_t allGeneratedImmature, allGeneratedMature, allFee;
                allGeneratedImmature = allGeneratedMature = allFee = 0;
                string strSentAccount;
                list<pair<Wallet::NexusAddress, int64_t> > listReceived;
                list<pair<Wallet::NexusAddress, int64_t> > listSent;
                wtx.GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                {
                    BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress,int64_t)& r, listReceived)
                        nBalance += r.second;
                }
                BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress,int64_t)& r, listSent)
                    nBalance -= r.second;
                nBalance -= allFee;
                nBalance += allGeneratedMature;
            }
            return  ValueFromAmount(nBalance);
        }

        string strAccount = AccountFromValue(params[0]);

        int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);

        return ValueFromAmount(nBalance);
    }


    Value movecmd(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 3 || params.size() > 5)
            throw runtime_error(
                "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]\n"
                "Move from one account in your wallet to another.");

        string strFrom = AccountFromValue(params[0]);
        string strTo = AccountFromValue(params[1]);
        int64_t nAmount = AmountFromValue(params[2]);
        if (params.size() > 3)
            // unused parameter, used to be nMinDepth, keep type-checking it though
            (void)params[3].get_int();
        string strComment;
        if (params.size() > 4)
            strComment = params[4].get_str();

        Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);
        if (!walletdb.TxnBegin())
            throw JSONRPCError(-20, "database error");

        int64_t nNow = GetUnifiedTimestamp();

        // Debit
        Wallet::CAccountingEntry debit;
        debit.strAccount = strFrom;
        debit.nCreditDebit = -nAmount;
        debit.nTime = nNow;
        debit.strOtherAccount = strTo;
        debit.strComment = strComment;
        walletdb.WriteAccountingEntry(debit);

        // Credit
        Wallet::CAccountingEntry credit;
        credit.strAccount = strTo;
        credit.nCreditDebit = nAmount;
        credit.nTime = nNow;
        credit.strOtherAccount = strFrom;
        credit.strComment = strComment;
        walletdb.WriteAccountingEntry(credit);

        if (!walletdb.TxnCommit())
            throw JSONRPCError(-20, "database error");

        return true;
    }


    Value sendfrom(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
            throw runtime_error(
                "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]\n"
                "<amount> is a real and is rounded to the nearest 0.000001\n"
                "requires wallet passphrase to be set with walletpassphrase first");
        if (!pwalletMain->IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
            throw runtime_error(
                "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]\n"
                "<amount> is a real and is rounded to the nearest 0.000001");

        string strAccount = AccountFromValue(params[0]);
        Wallet::NexusAddress address(params[1].get_str());
        if (!address.IsValid())
            throw JSONRPCError(-5, "Invalid Nexus address");
        int64_t nAmount = AmountFromValue(params[2]);
        if (nAmount < Core::MIN_TXOUT_AMOUNT)
            throw JSONRPCError(-101, "Send amount too small");
        int nMinDepth = 1;
        if (params.size() > 3)
            nMinDepth = params[3].get_int();

        Wallet::CWalletTx wtx;
        wtx.strFromAccount = strAccount;
        if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
            wtx.mapValue["comment"] = params[4].get_str();
        if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
            wtx.mapValue["to"]      = params[5].get_str();

        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        // Check funds
        int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
        if (nAmount > nBalance)
            throw JSONRPCError(-6, "Account has insufficient funds");

        // Send
        string strError = pwalletMain->SendToNexusAddress(address, nAmount, wtx);
        if (strError != "")
            throw JSONRPCError(-4, strError);

        return wtx.GetHash().GetHex();
    }


    Value sendmany(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
            throw runtime_error(
                "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]\n"
                "amounts are double-precision floating point numbers\n"
                "requires wallet passphrase to be set with walletpassphrase first");
        if (!pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
            throw runtime_error(
                "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]\n"
                "amounts are double-precision floating point numbers");

        string strAccount = AccountFromValue(params[0]);
        Object sendTo = params[1].get_obj();
        int nMinDepth = 1;
        if (params.size() > 2)
            nMinDepth = params[2].get_int();

        Wallet::CWalletTx wtx;
        wtx.strFromAccount = strAccount;
        if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
            wtx.mapValue["comment"] = params[3].get_str();

        set<Wallet::NexusAddress> setAddress;
        vector<pair<Wallet::CScript, int64_t> > vecSend;

        int64_t totalAmount = 0;
        BOOST_FOREACH(const Pair& s, sendTo)
        {
            Wallet::NexusAddress address(s.name_);
            if (!address.IsValid())
                throw JSONRPCError(-5, string("Invalid Nexus address:")+s.name_);

            if (setAddress.count(address))
                throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+s.name_);
            setAddress.insert(address);

            Wallet::CScript scriptPubKey;
            scriptPubKey.SetNexusAddress(address);
            int64_t nAmount = AmountFromValue(s.value_);
            if (nAmount < Core::MIN_TXOUT_AMOUNT)
                throw JSONRPCError(-101, "Send amount too small");
            totalAmount += nAmount;

            vecSend.push_back(make_pair(scriptPubKey, nAmount));
        }

        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
        if (Wallet::fWalletUnlockMintOnly)
            throw JSONRPCError(-13, "Error: Wallet unlocked for block minting only.");

        // Check funds
        int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
        if (totalAmount > nBalance)
            throw JSONRPCError(-6, "Account has insufficient funds");

        // Send
        Wallet::CReserveKey keyChange(pwalletMain);
        int64_t nFeeRequired = 0;
        bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);
        if (!fCreated)
        {
            if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
                throw JSONRPCError(-6, "Insufficient funds");
            throw JSONRPCError(-4, "Transaction creation failed");
        }
        if (!pwalletMain->CommitTransaction(wtx, keyChange))
            throw JSONRPCError(-4, "Transaction commit failed");

        return wtx.GetHash().GetHex();
    }

    Value addmultisigaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 2 || params.size() > 3)
        {
            string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]\n"
                "Add a nrequired-to-sign multisignature address to the wallet\"\n"
                "each key is a nexus address or hex-encoded public key\n"
                "If [account] is specified, assign address to [account].";
            throw runtime_error(msg);
        }

        int nRequired = params[0].get_int();
        const Array& keys = params[1].get_array();
        string strAccount;
        if (params.size() > 2)
            strAccount = AccountFromValue(params[2]);

        // Gather public keys
        if (nRequired < 1)
            throw runtime_error("a multisignature address must require at least one key to redeem");
        if ((int)keys.size() < nRequired)
            throw runtime_error(
                strprintf("not enough keys supplied "
                          "(got %d keys, but need at least %d to redeem)", keys.size(), nRequired));
        std::vector<Wallet::CKey> pubkeys;
        pubkeys.resize(keys.size());
        for (unsigned int i = 0; i < keys.size(); i++)
        {
            const std::string& ks = keys[i].get_str();

            // Case 1: nexus address and we have full public key:
            Wallet::NexusAddress address(ks);
            if (address.IsValid())
            {
                if (address.IsScript())
                    throw runtime_error(
                        strprintf("%s is a pay-to-script address",ks.c_str()));
                std::vector<unsigned char> vchPubKey;
                if (!pwalletMain->GetPubKey(address, vchPubKey))
                    throw runtime_error(
                        strprintf("no full public key for address %s",ks.c_str()));
                if (vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
                    throw runtime_error(" Invalid public key: "+ks);
            }

            // Case 2: hex public key
            else if (IsHex(ks))
            {
                vector<unsigned char> vchPubKey = ParseHex(ks);
                if (vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
                    throw runtime_error(" Invalid public key: "+ks);
            }
            else
            {
                throw runtime_error(" Invalid public key: "+ks);
            }
        }

        // Construct using pay-to-script-hash:
        Wallet::CScript inner;
        inner.SetMultisig(nRequired, pubkeys);

        uint256 scriptHash = SK256(inner);
        Wallet::CScript scriptPubKey;
        scriptPubKey.SetPayToScriptHash(inner);
        pwalletMain->AddCScript(inner);
        Wallet::NexusAddress address;
        address.SetScriptHash256(scriptHash);

        pwalletMain->SetAddressBookName(address, strAccount);
        return address.ToString();
    }


    struct tallyitem
    {
        int64_t nAmount;
        int nConf;
        tallyitem()
        {
            nAmount = 0;
            nConf = std::numeric_limits<int>::max();
        }
    };

    Value ListReceived(const Array& params, bool fByAccounts)
    {
        // Minimum confirmations
        int nMinDepth = 1;
        if (params.size() > 0)
            nMinDepth = params[0].get_int();

        // Whether to include empty accounts
        bool fIncludeEmpty = false;
        if (params.size() > 1)
            fIncludeEmpty = params[1].get_bool();

        // Tally
        map<Wallet::NexusAddress, tallyitem> mapTally;
        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const Wallet::CWalletTx& wtx = (*it).second;

            if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                continue;

            int nDepth = wtx.GetDepthInMainChain();
            if (nDepth < nMinDepth)
                continue;

            BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
            {
                Wallet::NexusAddress address;
                if (!ExtractAddress(txout.scriptPubKey, address) || !pwalletMain->HaveKey(address) || !address.IsValid())
                    continue;

                tallyitem& item = mapTally[address];
                item.nAmount += txout.nValue;
                item.nConf = min(item.nConf, nDepth);
            }
        }

        // Reply
        Array ret;
        map<string, tallyitem> mapAccountTally;
        BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
        {
            const Wallet::NexusAddress& address = item.first;
            const string& strAccount = item.second;
            map<Wallet::NexusAddress, tallyitem>::iterator it = mapTally.find(address);
            if (it == mapTally.end() && !fIncludeEmpty)
                continue;

            int64_t nAmount = 0;
            int nConf = std::numeric_limits<int>::max();
            if (it != mapTally.end())
            {
                nAmount = (*it).second.nAmount;
                nConf = (*it).second.nConf;
            }

            if (fByAccounts)
            {
                tallyitem& item = mapAccountTally[strAccount];
                item.nAmount += nAmount;
                item.nConf = min(item.nConf, nConf);
            }
            else
            {
                Object obj;
                obj.push_back(Pair("address",       address.ToString()));
                obj.push_back(Pair("account",       strAccount));
                obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
                obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
                ret.push_back(obj);
            }
        }

        if (fByAccounts)
        {
            for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
            {
                int64_t nAmount = (*it).second.nAmount;
                int nConf = (*it).second.nConf;
                Object obj;
                obj.push_back(Pair("account",       (*it).first));
                obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
                obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
                ret.push_back(obj);
            }
        }

        return ret;
    }

    Value listreceivedbyaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 2)
            throw runtime_error(
                "listreceivedbyaddress [minconf=1] [includeempty=false]\n"
                "[minconf] is the minimum number of confirmations before payments are included.\n"
                "[includeempty] whether to include addresses that haven't received any payments.\n"
                "Returns an array of objects containing:\n"
                "  \"address\" : receiving address\n"
                "  \"account\" : the account of the receiving address\n"
                "  \"amount\" : total amount received by the address\n"
                "  \"confirmations\" : number of confirmations of the most recent transaction included");

        return ListReceived(params, false);
    }

    Value listreceivedbyaccount(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 2)
            throw runtime_error(
                "listreceivedbyaccount [minconf=1] [includeempty=false]\n"
                "[minconf] is the minimum number of confirmations before payments are included.\n"
                "[includeempty] whether to include accounts that haven't received any payments.\n"
                "Returns an array of objects containing:\n"
                "  \"account\" : the account of the receiving addresses\n"
                "  \"amount\" : total amount received by addresses with this account\n"
                "  \"confirmations\" : number of confirmations of the most recent transaction included");

        return ListReceived(params, true);
    }

    void ListTransactions(const Wallet::CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
    {
        int64_t nGeneratedImmature, nGeneratedMature, nFee;
        string strSentAccount;
        list<pair<Wallet::NexusAddress, int64_t> > listReceived;
        list<pair<Wallet::NexusAddress, int64_t> > listSent;

        wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);

        bool fAllAccounts = (strAccount == string("*"));

        // Generated blocks assigned to account ""
        if ((nGeneratedMature+nGeneratedImmature) != 0 && (fAllAccounts || strAccount == ""))
        {
            Object entry;
            entry.push_back(Pair("account", string("")));
            if (nGeneratedImmature)
            {
                entry.push_back(Pair("category", wtx.GetDepthInMainChain() ? "immature" : "orphan"));
                entry.push_back(Pair("amount", ValueFromAmount(nGeneratedImmature)));
            }
            else if (wtx.IsCoinStake())
            {
                entry.push_back(Pair("category", "stake"));
                entry.push_back(Pair("amount", ValueFromAmount(nGeneratedMature)));
            }
            else
            {
                entry.push_back(Pair("category", "generate"));
                entry.push_back(Pair("amount", ValueFromAmount(nGeneratedMature)));
            }
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }

        // Sent
        if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
        {
            BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64_t)& s, listSent)
            {
                Object entry;
                entry.push_back(Pair("account", strSentAccount));
                entry.push_back(Pair("address", s.first.ToString()));
                entry.push_back(Pair("category", "send"));
                entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
                entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
        }

        // Received
        if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
        {
            BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64_t)& r, listReceived)
            {
                string account;
                if (pwalletMain->mapAddressBook.count(r.first))
                    account = pwalletMain->mapAddressBook[r.first];
                if (fAllAccounts || (account == strAccount))
                {
                    Object entry;
                    entry.push_back(Pair("account", account));
                    entry.push_back(Pair("address", r.first.ToString()));
                    entry.push_back(Pair("category", "receive"));
                    entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                    if (fLong)
                        WalletTxToJSON(wtx, entry);
                    ret.push_back(entry);
                }
            }
        }
    }

    void AcentryToJSON(const Wallet::CAccountingEntry& acentry, const string& strAccount, Array& ret)
    {
        bool fAllAccounts = (strAccount == string("*"));

        if (fAllAccounts || acentry.strAccount == strAccount)
        {
            Object entry;
            entry.push_back(Pair("account", acentry.strAccount));
            entry.push_back(Pair("category", "move"));
            entry.push_back(Pair("time", (boost::int64_t)acentry.nTime));
            entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
            entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
            entry.push_back(Pair("comment", acentry.strComment));
            ret.push_back(entry);
        }
    }

    Value listtransactions(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactions [account] [count=10] [from=0]\n"
                "Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");

        string strAccount = "*";
        if (params.size() > 0)
            strAccount = params[0].get_str();
        int nCount = 10;
        if (params.size() > 1)
            nCount = params[1].get_int();
        int nFrom = 0;
        if (params.size() > 2)
            nFrom = params[2].get_int();

        if (nCount < 0)
            throw JSONRPCError(-8, "Negative count");
        if (nFrom < 0)
            throw JSONRPCError(-8, "Negative from");

        Array ret;
        Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);

        // First: get all Wallet::CWalletTx and Wallet::CAccountingEntry into a sorted-by-time multimap.
        typedef pair<Wallet::CWalletTx*, Wallet::CAccountingEntry*> TxPair;
        typedef multimap<int64_t, TxPair > TxItems;
        TxItems txByTime;

        // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
        // would make this much faster for applications that do this a lot.
        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            Wallet::CWalletTx* wtx = &((*it).second);
            txByTime.insert(make_pair(wtx->GetTxTime(), TxPair(wtx, (Wallet::CAccountingEntry*)0)));
        }
        list<Wallet::CAccountingEntry> acentries;
        walletdb.ListAccountCreditDebit(strAccount, acentries);
        BOOST_FOREACH(Wallet::CAccountingEntry& entry, acentries)
        {
            txByTime.insert(make_pair(entry.nTime, TxPair((Wallet::CWalletTx*)0, &entry)));
        }

        // iterate backwards until we have nCount items to return:
        for (TxItems::reverse_iterator it = txByTime.rbegin(); it != txByTime.rend(); ++it)
        {
            Wallet::CWalletTx *const pwtx = (*it).second.first;
            if (pwtx != 0)
                ListTransactions(*pwtx, strAccount, 0, true, ret);
            Wallet::CAccountingEntry *const pacentry = (*it).second.second;
            if (pacentry != 0)
                AcentryToJSON(*pacentry, strAccount, ret);

            if (ret.size() >= (nCount+nFrom)) break;
        }
        // ret is newest to oldest

        if (nFrom > (int)ret.size())
            nFrom = ret.size();
        if ((nFrom + nCount) > (int)ret.size())
            nCount = ret.size() - nFrom;
        Array::iterator first = ret.begin();
        std::advance(first, nFrom);
        Array::iterator last = ret.begin();
        std::advance(last, nFrom+nCount);

        if (last != ret.end()) ret.erase(last, ret.end());
        if (first != ret.begin()) ret.erase(ret.begin(), first);

        std::reverse(ret.begin(), ret.end()); // Return oldest to newest

        return ret;
    }


    Value listaccounts(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 1)
            throw runtime_error(
                "listaccounts [minconf=1]\n"
                "Returns Object that has account names as keys, account balances as values.");

        int nMinDepth = 1;
        if (params.size() > 0)
            nMinDepth = params[0].get_int();

        map<string, int64_t> mapAccountBalances;
        BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& entry, pwalletMain->mapAddressBook) {
            if (pwalletMain->HaveKey(entry.first)) // This address belongs to me
                mapAccountBalances[entry.second] = 0;
        }

        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const Wallet::CWalletTx& wtx = (*it).second;
            int64_t nGeneratedImmature, nGeneratedMature, nFee;
            string strSentAccount;
            list<pair<Wallet::NexusAddress, int64_t> > listReceived;
            list<pair<Wallet::NexusAddress, int64_t> > listSent;
            wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);
            mapAccountBalances[strSentAccount] -= nFee;
            BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64_t)& s, listSent)
                mapAccountBalances[strSentAccount] -= s.second;
            if (wtx.GetDepthInMainChain() >= nMinDepth)
            {
                mapAccountBalances[""] += nGeneratedMature;
                BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64_t)& r, listReceived)
                    if (pwalletMain->mapAddressBook.count(r.first))
                        mapAccountBalances[pwalletMain->mapAddressBook[r.first]] += r.second;
                    else
                        mapAccountBalances[""] += r.second;
            }
        }

        list<Wallet::CAccountingEntry> acentries;
        Wallet::CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
        BOOST_FOREACH(const Wallet::CAccountingEntry& entry, acentries)
            mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

        Object ret;
        BOOST_FOREACH(const PAIRTYPE(string, int64_t)& accountBalance, mapAccountBalances) {
            ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
        }
        return ret;
    }

    Value listsinceblock(const Array& params, bool fHelp)
    {
        if (fHelp)
            throw runtime_error(
                "listsinceblock [blockhash] [target-confirmations]\n"
                "Get all transactions in blocks since block [blockhash], or all transactions if omitted");

        Core::CBlockIndex *pindex = NULL;
        int target_confirms = 1;

        if (params.size() > 0)
        {
            uint1024 blockId = 0;

            blockId.SetHex(params[0].get_str());
            pindex = Core::CBlockLocator(blockId).GetBlockIndex();
        }

        if (params.size() > 1)
        {
            target_confirms = params[1].get_int();

            if (target_confirms < 1)
                throw JSONRPCError(-8, "Invalid parameter");
        }

        int depth = pindex ? (1 + Core::nBestHeight - pindex->nHeight) : -1;

        Array transactions;

        for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
        {
            Wallet::CWalletTx tx = (*it).second;

            if (depth == -1 || tx.GetDepthInMainChain() < depth)
                ListTransactions(tx, "*", 0, true, transactions);
        }


        //NOTE: Do we need this code for anything?
        uint1024 lastblock;

        if (target_confirms == 1)
        {
            lastblock = Core::hashBestChain;
        }
        else
        {
            int target_height = Core::pindexBest->nHeight + 1 - target_confirms;

            Core::CBlockIndex *block;
            for (block = Core::pindexBest;
                 block && block->nHeight > target_height;
                 block = block->pprev)  { }

            lastblock = block ? block->GetBlockHash() : 0;
        }

        Object ret;
        ret.push_back(Pair("transactions", transactions));
        ret.push_back(Pair("lastblock", lastblock.GetHex()));

        return ret;
    }

    Value gettransaction(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "gettransaction <txid>\n"
                "Get detailed information about <txid>");

        uint512 hash;
        hash.SetHex(params[0].get_str());

        Object entry;

        if (!pwalletMain->mapWallet.count(hash))
            throw JSONRPCError(-5, "Invalid or non-wallet transaction id");
        const Wallet::CWalletTx& wtx = pwalletMain->mapWallet[hash];

        int64_t nCredit = wtx.GetCredit();
        int64_t nDebit = wtx.GetDebit();
        int64_t nNet = nCredit - nDebit;
        int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

        entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
        if (wtx.IsFromMe())
            entry.push_back(Pair("fee", ValueFromAmount(nFee)));

        WalletTxToJSON(pwalletMain->mapWallet[hash], entry);

        Array details;
        ListTransactions(pwalletMain->mapWallet[hash], "*", 0, false, details);
        entry.push_back(Pair("details", details));

        return entry;
    }

    //
    // Raw transactions
    //
    Value getrawtransaction(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getrawtransaction <txid>\n"
                "Returns a string that is serialized,\n"
                "hex-encoded data for <txid>.");

        uint512 hash;
        hash.SetHex(params[0].get_str());

        Core::CTransaction tx;
        uint1024 hashBlock = 0;
        if (!Core::GetTransaction(hash, tx, hashBlock))
            throw JSONRPCError(-5, "No information available about transaction");

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;
        return HexStr(ssTx.begin(), ssTx.end());
    }

    Value sendrawtransaction(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "sendrawtransaction <hex string> [checkinputs=0]\n"
                "Submits raw transaction (serialized, hex-encoded) to local node and network.\n"
                "If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it.");

        // parse hex string from parameter
        vector<unsigned char> txData(ParseHex(params[0].get_str()));
        CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
        bool fCheckInputs = false;
        if (params.size() > 1)
            fCheckInputs = (params[1].get_int() != 0);
        Core::CTransaction tx;

        // deserialize binary data stream
        try {
            ssData >> tx;
        }
        catch (std::exception &e) {
            throw JSONRPCError(-22, "TX decode failed");
        }
        uint512 hashTx = tx.GetHash();

        // See if the transaction is already in a block
        // or in the memory pool:
        Core::CTransaction existingTx;
        uint1024 hashBlock = 0;
        if (Core::GetTransaction(hashTx, existingTx, hashBlock))
        {
            if (hashBlock != 0)
                throw JSONRPCError(-5, string("transaction already in block ")+hashBlock.GetHex());
            // Not in block, but already in the memory pool; will drop
            // through to re-relay it.
        }
        else
        {
            // push to local node
            LLD::CIndexDB txdb("r");
            if (!tx.AcceptToMemoryPool(txdb, fCheckInputs))
                throw JSONRPCError(-22, "TX rejected");

            SyncWithWallets(tx, NULL, true);
        }
        RelayMessage(CInv(MSG_TX, hashTx), tx);

        return hashTx.GetHex();
    }


    Value backupwallet(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "backupwallet <destination>\n"
                "Safely copies wallet.dat to destination, which can be a directory or a path with filename.");

        string strDest = params[0].get_str();
        BackupWallet(*pwalletMain, strDest);

        return Value::null;
    }


    Value keypoolrefill(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() > 0))
            throw runtime_error(
                "keypoolrefill\n"
                "Fills the keypool, requires wallet passphrase to be set.");
        if (!pwalletMain->IsCrypted() && (fHelp || params.size() > 0))
            throw runtime_error(
                "keypoolrefill\n"
                "Fills the keypool.");

        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        pwalletMain->TopUpKeyPool();

        if (pwalletMain->GetKeyPoolSize() < GetArg("-keypool", 100))
            throw JSONRPCError(-4, "Error refreshing keypool.");

        return Value::null;
    }


    void ThreadTopUpKeyPool(void* parg)
    {
        pwalletMain->TopUpKeyPool();
    }

    void ThreadCleanWalletPassphrase(void* parg)
    {
        int64_t nMyWakeTime = GetTimeMillis() + *((int64_t*)parg) * 1000;

        ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

        if (nWalletUnlockTime == 0)
        {
            nWalletUnlockTime = nMyWakeTime;

            do
            {
                if (nWalletUnlockTime==0)
                    break;
                int64_t nToSleep = nWalletUnlockTime - GetTimeMillis();
                if (nToSleep <= 0)
                    break;

                LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);
                Sleep(nToSleep);
                ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

            } while(1);

            if (nWalletUnlockTime)
            {
                nWalletUnlockTime = 0;
                pwalletMain->Lock();
            }
        }
        else
        {
            if (nWalletUnlockTime < nMyWakeTime)
                nWalletUnlockTime = nMyWakeTime;
        }

        LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);

        delete (int64_t*)parg;
    }

    Value walletpassphrase(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 3))
            throw runtime_error(
                "walletpassphrase <passphrase> <timeout> [mintonly]\n"
                "Stores the wallet decryption key in memory for <timeout> seconds.\n"
                "mintonly is optional true/false allowing only block minting.");
        if (fHelp)
            return true;
        if (!pwalletMain->IsCrypted())
            throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

        if (!pwalletMain->IsLocked())
            throw JSONRPCError(-17, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");

        // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
        SecureString strWalletPass;
        strWalletPass.reserve(100);
        // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
        // Alternately, find a way to make params[0] mlock()'d to begin with.
        strWalletPass = params[0].get_str().c_str();

        if (strWalletPass.length() > 0)
        {
            if (!pwalletMain->Unlock(strWalletPass))
                throw JSONRPCError(-14, "Error: The wallet passphrase entered was incorrect.");
        }
        else
            throw runtime_error(
                "walletpassphrase <passphrase> <timeout>\n"
                "Stores the wallet decryption key in memory for <timeout> seconds.");

        CreateThread(ThreadTopUpKeyPool, NULL);
        int64_t* pnSleepTime = new int64_t(params[1].get_int64());
        CreateThread(ThreadCleanWalletPassphrase, pnSleepTime);

        // Nexus: if user OS account compromised prevent trivial sendmoney commands
        if (params.size() > 2)
            Wallet::fWalletUnlockMintOnly = params[2].get_bool();
        else
            Wallet::fWalletUnlockMintOnly = false;

        return Value::null;
    }


    Value walletpassphrasechange(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
            throw runtime_error(
                "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
                "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");
        if (fHelp)
            return true;
        if (!pwalletMain->IsCrypted())
            throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

        // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
        // Alternately, find a way to make params[0] mlock()'d to begin with.
        SecureString strOldWalletPass;
        strOldWalletPass.reserve(100);
        strOldWalletPass = params[0].get_str().c_str();

        SecureString strNewWalletPass;
        strNewWalletPass.reserve(100);
        strNewWalletPass = params[1].get_str().c_str();

        if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
            throw runtime_error(
                "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
                "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

        if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
            throw JSONRPCError(-14, "Error: The wallet passphrase entered was incorrect.");

        return Value::null;
    }


    Value walletlock(const Array& params, bool fHelp)
    {
        if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
            throw runtime_error(
                "walletlock\n"
                "Removes the wallet encryption key from memory, locking the wallet.\n"
                "After calling this method, you will need to call walletpassphrase again\n"
                "before being able to call any methods which require the wallet to be unlocked.");
        if (fHelp)
            return true;
        if (!pwalletMain->IsCrypted())
            throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletlock was called.");

        {
            LOCK(cs_nWalletUnlockTime);
            pwalletMain->Lock();
            nWalletUnlockTime = 0;
        }

        return Value::null;
    }


    Value encryptwallet(const Array& params, bool fHelp)
    {
        if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
            throw runtime_error(
                "encryptwallet <passphrase>\n"
                "Encrypts the wallet with <passphrase>.");
        if (fHelp)
            return true;
        if (pwalletMain->IsCrypted())
            throw JSONRPCError(-15, "Error: running with an encrypted wallet, but encryptwallet was called.");

        // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
        // Alternately, find a way to make params[0] mlock()'d to begin with.
        SecureString strWalletPass;
        strWalletPass.reserve(100);
        strWalletPass = params[0].get_str().c_str();

        if (strWalletPass.length() < 1)
            throw runtime_error(
                "encryptwallet <passphrase>\n"
                "Encrypts the wallet with <passphrase>.");

        if (!pwalletMain->EncryptWallet(strWalletPass))
            throw JSONRPCError(-16, "Error: Failed to encrypt the wallet.");

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys.  So:
        StartShutdown();
        return "wallet encrypted; Nexus server stopping, restart to run with encrypted wallet";
    }


    Value validateaddress(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "validateaddress <Nexusaddress>\n"
                "Return information about <Nexusaddress>.");

        Wallet::NexusAddress address(params[0].get_str());
        bool isValid = address.IsValid();

        Object ret;
        ret.push_back(Pair("isvalid", isValid));
        if (isValid)
        {
            string currentAddress = address.ToString();
            ret.push_back(Pair("address", currentAddress));
            if (pwalletMain->HaveKey(address))
            {
                ret.push_back(Pair("ismine", true));
                std::vector<unsigned char> vchPubKey;
                pwalletMain->GetPubKey(address, vchPubKey);
                ret.push_back(Pair("pubkey", HexStr(vchPubKey)));
                Wallet::CKey key;
                key.SetPubKey(vchPubKey);
                ret.push_back(Pair("iscompressed", key.IsCompressed()));
            }
            else if (pwalletMain->HaveCScript(address.GetHash256()))
            {
                ret.push_back(Pair("isscript", true));
                Wallet::CScript subscript;
                pwalletMain->GetCScript(address.GetHash256(), subscript);
                ret.push_back(Pair("ismine", Wallet::IsMine(*pwalletMain, subscript)));
                std::vector<Wallet::NexusAddress> addresses;
                Wallet::TransactionType whichType;
                int nRequired;
                Wallet::ExtractAddresses(subscript, whichType, addresses, nRequired);
                ret.push_back(Pair("script", Wallet::GetTxnOutputType(whichType)));
                Array a;
                BOOST_FOREACH(const Wallet::NexusAddress& addr, addresses)
                    a.push_back(addr.ToString());
                ret.push_back(Pair("addresses", a));
                if (whichType == Wallet::TX_MULTISIG)
                    ret.push_back(Pair("sigsrequired", nRequired));
            }
            else
                ret.push_back(Pair("ismine", false));
            if (pwalletMain->mapAddressBook.count(address))
                ret.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
        }
        return ret;
    }


    Value getblockhash(const Array& params, bool fHelp)
    {

        if (fHelp || params.size() != 1)
            throw runtime_error(
                "getblockhash <index>\n"
                "Returns hash of block in best-block-chain at <index>.");

        int nHeight = params[0].get_int();
        if (nHeight < 0 || nHeight > Core::nBestHeight)
            throw runtime_error("Block number out of range.");

        Core::CBlock block;
        Core::CBlockIndex* pblockindex = Core::mapBlockIndex[Core::hashBestChain];
        while (pblockindex->nHeight > nHeight)
            pblockindex = pblockindex->pprev;
        return pblockindex->phashBlock->GetHex();
    }

    Value getblock(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "getblock <hash> [txinfo]\n"
                "txinfo optional to print more detailed tx info\n"
                "Returns details of a block with given block-hash.");

        std::string strHash = params[0].get_str();
        uint1024 hash(strHash);

        if (Core::mapBlockIndex.count(hash) == 0)
            throw JSONRPCError(-5, "Block not found");

        Core::CBlock block;
        Core::CBlockIndex* pblockindex = Core::mapBlockIndex[hash];
        block.ReadFromDisk(pblockindex, true);

        return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
    }


    // Nexus: reserve balance from being staked for network protection
    Value reservebalance(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 2)
            throw runtime_error(
                "reservebalance [<reserve> [amount]]\n"
                "<reserve> is true or false to turn balance reserve on or off.\n"
                "<amount> is a real and rounded to cent.\n"
                "Set reserve amount not participating in network protection.\n"
                "If no parameters provided current setting is printed.\n");

        if (params.size() > 0)
        {
            bool fReserve = params[0].get_bool();
            if (fReserve)
            {
                if (params.size() == 1)
                    throw runtime_error("must provide amount to reserve balance.\n");
                int64_t nAmount = AmountFromValue(params[1]);
                nAmount = (nAmount / CENT) * CENT;  // round to cent
                if (nAmount < 0)
                    throw runtime_error("amount cannot be negative.\n");
                mapArgs["-reservebalance"] = FormatMoney(nAmount).c_str();
            }
            else
            {
                if (params.size() > 1)
                    throw runtime_error("cannot specify amount to turn off reserve.\n");
                mapArgs["-reservebalance"] = "0";
            }
        }

        Object result;
        int64_t nReserveBalance = 0;
        if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
            throw runtime_error("invalid reserve balance amount\n");
        result.push_back(Pair("reserve", (nReserveBalance > 0)));
        result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
        return result;
    }


    // Nexus: check wallet integrity
    Value checkwallet(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
            throw runtime_error(
                "checkwallet\n"
                "Check wallet for integrity.\n");

        int nMismatchSpent;
        int64_t nBalanceInQuestion;
        pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion, true);
        Object result;
        if (nMismatchSpent == 0)
            result.push_back(Pair("wallet check passed", true));
        else
        {
            result.push_back(Pair("mismatched spent coins", nMismatchSpent));
            result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
        }
        return result;
    }

    /* List the trust keys this wallet.dat file contains. */
    Value listtrustkeys(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
            throw runtime_error(
                "listtrustkeys\n"
                "List all the Trust Keys this Node owns.\n");


        /** Check each Trust Key to See if we Own it if there is no Key. **/
        Object result;
        for(std::map<uint576, Core::CTrustKey>::iterator i = Core::cTrustPool.mapTrustKeys.begin(); i != Core::cTrustPool.mapTrustKeys.end(); ++i)
        {

            /** Check the Wallet and Trust Keys in Trust Pool to see if we own any keys. **/
            Wallet::NexusAddress address;
            address.SetPubKey(i->second.vchPubKey);
            if(pwalletMain->HaveKey(address))
                result.push_back(Pair(address.ToString(), i->second.Expired(0, Core::pindexBest->pprev->GetBlockHash()) ? "TRUE" : "FALSE"));

        }

        return result;
    }

    // Nexus: repair wallet
    Value repairwallet(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
            throw runtime_error(
                "repairwallet\n"
                "Repair wallet if checkwallet reports any problem.\n");

        int nMismatchSpent;
        int64_t nBalanceInQuestion;
        pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion);
        Object result;
        if (nMismatchSpent == 0)
            result.push_back(Pair("wallet check passed", true));
        else
        {
            result.push_back(Pair("mismatched spent coins", nMismatchSpent));
            result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
        }
        return result;
    }


    // Nexus: make a public-private key pair
    Value makekeypair(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 1)
            throw runtime_error(
                "makekeypair [prefix]\n"
                "Make a public/private key pair.\n"
                "[prefix] is optional preferred prefix for the public key.\n");

        string strPrefix = "";
        if (params.size() > 0)
            strPrefix = params[0].get_str();

        Wallet::CKey key;
        int nCount = 0;
        do
        {
            key.MakeNewKey(false);
            nCount++;
        } while (nCount < 10000 && strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()));

        if (strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()))
            return Value::null;

        Wallet::CPrivKey vchPrivKey = key.GetPrivKey();
        Object result;
        result.push_back(Pair("PrivateKey", HexStr<Wallet::CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
        result.push_back(Pair("PublicKey", HexStr(key.GetPubKey())));
        return result;
    }

    /** TODO: Account balance based on unspent outputs in the core.
        This will be good to determine the actual balance based on the registry of the current addresses
        Associated with the specific accounts. This needs to be fixed properly so thaat the wallet accounting
        is done properly.

        Second TODO: While at this the wallet core code needs to be reworked in orcder to mitigate the issue of
        having a large number of transactions in the actual memory map which can slow the entire process down. */
    Value unspentbalance(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 1)
            throw runtime_error(
                "unspentbalance [\"address\",...]\n"
                "Returns the total amount of unspent Nexus for given address\n"
                "This is a more accurate command than Get Balance.\n");

        set<Wallet::NexusAddress> setAddresses;
        if (params.size() > 0)
        {
            for(int i = 0; i < params.size(); i++)
            {
                Wallet::NexusAddress address(params[i].get_str());
                if (!address.IsValid()) {
                    throw JSONRPCError(-5, string("Invalid Nexus address: ")+params[i].get_str());
                }
                if (setAddresses.count(address)){
                    throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+params[i].get_str());
                }
               setAddresses.insert(address);
            }
        }

        vector<Wallet::COutput> vecOutputs;
        pwalletMain->AvailableCoins((unsigned int)GetUnifiedTimestamp(), vecOutputs, false);

        int64_t nCredit = 0;
        BOOST_FOREACH(const Wallet::COutput& out, vecOutputs)
        {
            if(setAddresses.size())
            {
                Wallet::NexusAddress address;
                if(!ExtractAddress(out.tx->vout[out.i].scriptPubKey, address))
                    continue;

                if (!setAddresses.count(address))
                    continue;
            }

            int64_t nValue = out.tx->vout[out.i].nValue;
            const Wallet::CScript& pk = out.tx->vout[out.i].scriptPubKey;
            Wallet::NexusAddress address;

            printf("txid %s | ", out.tx->GetHash().GetHex().c_str());
            printf("vout %u | ", out.i);
            if (Wallet::ExtractAddress(pk, address))
                printf("address %s |", address.ToString().c_str());

            printf("amount %f |", (double)nValue / COIN);
            printf("confirmations %u\n",out.nDepth);

            nCredit += nValue;
        }

        return ValueFromAmount(nCredit);
    }



    Value listunspent(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() > 3)
            throw runtime_error(
                "listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]\n"
                "Returns array of unspent transaction outputs\n"
                "with between minconf and maxconf (inclusive) confirmations.\n"
                "Optionally filtered to only include txouts paid to specified addresses.\n"
                "Results are an array of Objects, each of which has:\n"
                "{txid, vout, scriptPubKey, amount, confirmations}");

        int nMinDepth = 1;
        if (params.size() > 0)
            nMinDepth = params[0].get_int();

        int nMaxDepth = 9999999;
        if (params.size() > 1)
            nMaxDepth = params[1].get_int();

        set<Wallet::NexusAddress> setAddress;
        if (params.size() > 2)
        {
            Array inputs = params[2].get_array();
            BOOST_FOREACH(Value& input, inputs)
            {
                Wallet::NexusAddress address(input.get_str());
                if (!address.IsValid())
                    throw JSONRPCError(-5, string("Invalid Nexus address: ")+input.get_str());

                if (setAddress.count(address))
                    throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+input.get_str());

               setAddress.insert(address);
            }
        }

        Array results;
        vector<Wallet::COutput> vecOutputs;
        pwalletMain->AvailableCoins((unsigned int)GetUnifiedTimestamp(), vecOutputs, false);
        BOOST_FOREACH(const Wallet::COutput& out, vecOutputs)
        {
            if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
                continue;

            if(!setAddress.empty())
            {
                Wallet::NexusAddress address;
                if(!ExtractAddress(out.tx->vout[out.i].scriptPubKey, address))
                    continue;

                if (!setAddress.count(address))
                    continue;
            }

            int64_t nValue = out.tx->vout[out.i].nValue;
            const Wallet::CScript& pk = out.tx->vout[out.i].scriptPubKey;
            Wallet::NexusAddress address;
            Object entry;
            entry.push_back(Pair("txid", out.tx->GetHash().GetHex()));
            entry.push_back(Pair("vout", out.i));
            if (Wallet::ExtractAddress(pk, address))
            {
                entry.push_back(Pair("address", address.ToString()));
                if (pwalletMain->mapAddressBook.count(address))
                    entry.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
            }
            entry.push_back(Pair("scriptPubKey", HexStr(pk.begin(), pk.end())));
            entry.push_back(Pair("amount",ValueFromAmount(nValue)));
            entry.push_back(Pair("confirmations",out.nDepth));

            results.push_back(entry);
        }

        return results;
    }



    Value getrawmempool(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "getrawmempool\n"
                "Returns all transaction ids in memory pool.");

        vector<uint512> vtxid;
        Core::mempool.queryHashes(vtxid);

        Array a;
        BOOST_FOREACH(const uint512& hash, vtxid)
            a.push_back(hash.ToString());

        return a;
    }


    //
    // Call Table
    //

    static const CRPCCommand vRPCCommands[] =
    { //  name                      function                 safe mode?
      //  ------------------------  -----------------------  ----------
        { "help",                   &help,                   true  },
        { "stop",                   &stop,                   true  },
        { "getblockcount",          &getblockcount,          true  },
        { "getblocknumber",         &getblocknumber,         true  },
        { "getconnectioncount",     &getconnectioncount,     true  },
        { "getpeerinfo",            &getpeerinfo,            true  },
        { "getdifficulty",          &getdifficulty,          true  },
        { "getsupplyrates",         &getsupplyrates,         true  },
        { "getinfo",                &getinfo,                true  },
        { "getmininginfo",          &getmininginfo,          true  },
        { "getnewaddress",          &getnewaddress,          true  },
        { "getaccountaddress",      &getaccountaddress,      true  },
        { "setaccount",             &setaccount,             true  },
        { "getaccount",             &getaccount,             false },
        { "getaddressesbyaccount",  &getaddressesbyaccount,  true  },
        { "sendtoaddress",          &sendtoaddress,          false },
        { "getmoneysupply",         &getmoneysupply,         true  },
        { "getnetworkhashps",       &getnetworkhashps,       true  },
        { "getnetworkpps",          &getnetworkpps,          true  },
        { "getnetworktrustkeys",    &getnetworktrustkeys,    true  },
        { "getreceivedbyaddress",   &getreceivedbyaddress,   false },
        { "getreceivedbyaccount",   &getreceivedbyaccount,   false },
        { "listreceivedbyaddress",  &listreceivedbyaddress,  false },
        { "listreceivedbyaccount",  &listreceivedbyaccount,  false },
        { "exportkeys",             &exportkeys,             false },
        { "importkeys",             &importkeys,             false },
        { "rescan",                 &rescan,                 false },
        { "backupwallet",           &backupwallet,           true  },
        { "keypoolrefill",          &keypoolrefill,          true  },
        { "walletpassphrase",       &walletpassphrase,       true  },
        { "walletpassphrasechange", &walletpassphrasechange, false },
        { "walletlock",             &walletlock,             true  },
        { "encryptwallet",          &encryptwallet,          false },
        { "validateaddress",        &validateaddress,        true  },
        { "getbalance",             &getbalance,             false },
        { "move",                   &movecmd,                false },
        { "sendfrom",               &sendfrom,               false },
        { "sendmany",               &sendmany,               false },
        { "addmultisigaddress",     &addmultisigaddress,     false },
        { "getblock",               &getblock,               false },
        { "getblockhash",           &getblockhash,           false },
        { "gettransaction",         &gettransaction,         false },
        { "getrawtransaction",      &getrawtransaction,      false },
        { "sendrawtransaction",     &sendrawtransaction,     false },
        { "getglobaltransaction",   &getglobaltransaction,   false },
        { "getaddressbalance",      &getaddressbalance,      false },
        { "dumprichlist",           &dumprichlist,           false },
        { "gettransactions",        &gettransactions,        false },
        { "totaltransactions",      &totaltransactions,      false },
        { "listtransactions",       &listtransactions,       false },
        { "signmessage",            &signmessage,            false },
        { "verifymessage",          &verifymessage,          false },
        { "listaccounts",           &listaccounts,           false },
        { "listunspent",            &listunspent,            false },
        { "unspentbalance",         &unspentbalance,         false },
        { "settxfee",               &settxfee,               false },
        { "listsinceblock",         &listsinceblock,         false },
        { "dumpprivkey",            &dumpprivkey,            false },
        { "importprivkey",          &importprivkey,          false },
        { "reservebalance",         &reservebalance,         false },
        { "listtrustkeys",          &listtrustkeys,          false },
        { "checkwallet",            &checkwallet,            false },
        { "repairwallet",           &repairwallet,           false },
        { "makekeypair",            &makekeypair,            false },
        { "getMapCommandsKeyVector",&getMapCommandsKeyVector,false },
        { "getrawmempool",          &getrawmempool,          false }
    };

    CRPCTable::CRPCTable()
    {
        unsigned int vcidx;
        for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
        {
            const CRPCCommand *pcmd;

            pcmd = &vRPCCommands[vcidx];
            mapCommands[pcmd->name] = pcmd;
        }
    }

    const CRPCCommand *CRPCTable::operator[](string name) const
    {
        map<string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
        if (it == mapCommands.end())
            return NULL;
        return (*it).second;
    }

    //
    // HTTP protocol
    //
    // This ain't Apache.  We're just using HTTP header for the length field
    // and to be compatible with other JSON-RPC implementations.
    //

    string HTTPPost(const string& strMsg, const map<string,string>& mapRequestHeaders)
    {
        ostringstream s;
        s << "POST / HTTP/1.1\r\n"
          << "User-Agent: Nexus-json-rpc/" << FormatFullVersion() << "\r\n"
          << "Host: 127.0.0.1\r\n"
          << "Content-Type: application/json\r\n"
          << "Content-Length: " << strMsg.size() << "\r\n"
          << "Connection: close\r\n"
          << "Accept: application/json\r\n";
        BOOST_FOREACH(const PAIRTYPE(string, string)& item, mapRequestHeaders)
            s << item.first << ": " << item.second << "\r\n";
        s << "\r\n" << strMsg;

        return s.str();
    }

    string rfc1123Time()
    {
        char buffer[64];
        time_t now;
        time(&now);
        struct tm* now_gmt = gmtime(&now);
        string locale(setlocale(LC_TIME, NULL));
        setlocale(LC_TIME, "C"); // we want posix (aka "C") weekday/month strings
        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S +0000", now_gmt);
        setlocale(LC_TIME, locale.c_str());
        return string(buffer);
    }

    static string HTTPReply(int nStatus, const string& strMsg)
    {
        if (nStatus == 401)
            return strprintf("HTTP/1.0 401 Authorization Required\r\n"
                "Date: %s\r\n"
                "Server: Nexus-json-rpc/%s\r\n"
                "WWW-Authenticate: Basic realm=\"jsonrpc\"\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 296\r\n"
                "\r\n"
                "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\r\n"
                "\"http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd\">\r\n"
                "<HTML>\r\n"
                "<HEAD>\r\n"
                "<TITLE>Error</TITLE>\r\n"
                "<META HTTP-EQUIV='Content-Type' CONTENT='text/html; charset=ISO-8859-1'>\r\n"
                "</HEAD>\r\n"
                "<BODY><H1>401 Unauthorized.</H1></BODY>\r\n"
                "</HTML>\r\n", rfc1123Time().c_str(), FormatFullVersion().c_str());
        const char *cStatus;
             if (nStatus == 200) cStatus = "OK";
        else if (nStatus == 400) cStatus = "Bad Request";
        else if (nStatus == 403) cStatus = "Forbidden";
        else if (nStatus == 404) cStatus = "Not Found";
        else if (nStatus == 500) cStatus = "Internal Server Error";
        else cStatus = "";
        return strprintf(
                "HTTP/1.1 %d %s\r\n"
                "Date: %s\r\n"
                "Connection: close\r\n"
                "Content-Length: %d\r\n"
                "Content-Type: application/json\r\n"
                "Server: Nexus-json-rpc/%s\r\n"
                "\r\n"
                "%s",
            nStatus,
            cStatus,
            rfc1123Time().c_str(),
            strMsg.size(),
            FormatFullVersion().c_str(),
            strMsg.c_str());
    }

    int ReadHTTPStatus(std::basic_istream<char>& stream)
    {
        string str;
        getline(stream, str);
        vector<string> vWords;
        boost::split(vWords, str, boost::is_any_of(" "));
        if (vWords.size() < 2)
            return 500;
        return atoi(vWords[1].c_str());
    }

    int ReadHTTPHeader(std::basic_istream<char>& stream, map<string, string>& mapHeadersRet)
    {
        int nLen = 0;
        loop() {
            string str;
            std::getline(stream, str);
            if (str.empty() || str == "\r")
                break;
            string::size_type nColon = str.find(":");
            if (nColon != string::npos)
            {
                string strHeader = str.substr(0, nColon);
                boost::trim(strHeader);
                boost::to_lower(strHeader);
                string strValue = str.substr(nColon+1);
                boost::trim(strValue);
                mapHeadersRet[strHeader] = strValue;
                if (strHeader == "content-length")
                    nLen = atoi(strValue.c_str());
            }
        }
        return nLen;
    }

    int ReadHTTP(std::basic_istream<char>& stream, map<string, string>& mapHeadersRet, string& strMessageRet)
    {
        mapHeadersRet.clear();
        strMessageRet = "";

        // Read status
        int nStatus = ReadHTTPStatus(stream);

        // Read header
        int nLen = ReadHTTPHeader(stream, mapHeadersRet);
        if (nLen < 0 || nLen > (int)MAX_SIZE)
            return 500;

        // Read message
        if (nLen > 0)
        {
            vector<char> vch(nLen);
            stream.read(&vch[0], nLen);
            strMessageRet = string(vch.begin(), vch.end());
        }

        return nStatus;
    }

    bool HTTPAuthorized(map<string, string>& mapHeaders)
    {
        string strAuth = mapHeaders["authorization"];
        if (strAuth.substr(0,6) != "Basic ")
            return false;
        string strUserPass64 = strAuth.substr(6); boost::trim(strUserPass64);
        string strUserPass = DecodeBase64(strUserPass64);
        return strUserPass == strRPCUserColonPass;
    }

    //
    // JSON-RPC protocol.  Nexus speaks version 1.0 for maximum compatibility,
    // but uses JSON-RPC 1.1/2.0 standards for parts of the 1.0 standard that were
    // unspecified (HTTP errors and contents of 'error').
    //
    // 1.0 spec: http://json-rpc.org/wiki/specification
    // 1.2 spec: http://groups.google.com/group/json-rpc/web/json-rpc-over-http
    // http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
    //

    string JSONRPCRequest(const string& strMethod, const Array& params, const Value& id)
    {
        Object request;
        request.push_back(Pair("method", strMethod));
        request.push_back(Pair("params", params));
        request.push_back(Pair("id", id));
        return write_string(Value(request), false) + "\n";
    }

    string JSONRPCReply(const Value& result, const Value& error, const Value& id)
    {
        Object reply;
        if (error.type() != null_type)
            reply.push_back(Pair("result", Value::null));
        else
            reply.push_back(Pair("result", result));
        reply.push_back(Pair("error", error));
        reply.push_back(Pair("id", id));
        return write_string(Value(reply), false) + "\n";
    }

    void ErrorReply(std::ostream& stream, const Object& objError, const Value& id)
    {
        // Send error reply from json-rpc error object
        int nStatus = 500;
        int code = find_value(objError, "code").get_int();
        if (code == -32600) nStatus = 400;
        else if (code == -32601) nStatus = 404;
        string strReply = JSONRPCReply(Value::null, objError, id);
        stream << HTTPReply(nStatus, strReply) << std::flush;
    }

    bool ClientAllowed(const string& strAddress)
    {
        if (strAddress == asio::ip::address_v4::loopback().to_string())
            return true;
        const vector<string>& vAllow = mapMultiArgs["-rpcallowip"];
        BOOST_FOREACH(string strAllow, vAllow)
            if (WildcardMatch(strAddress, strAllow))
                return true;
        return false;
    }

    //
    // IOStream device that speaks SSL but can also speak non-SSL
    //
    class SSLIOStreamDevice : public iostreams::device<iostreams::bidirectional> {
    public:
        SSLIOStreamDevice(SSLStream &streamIn, bool fUseSSLIn) : stream(streamIn)
        {
            fUseSSL = fUseSSLIn;
            fNeedHandshake = fUseSSLIn;
        }

        void handshake(ssl::stream_base::handshake_type role)
        {
            if (!fNeedHandshake) return;
            fNeedHandshake = false;
            stream.handshake(role);
        }
        std::streamsize read(char* s, std::streamsize n)
        {
            handshake(ssl::stream_base::server); // HTTPS servers read first
            if (fUseSSL) return stream.read_some(asio::buffer(s, n));
            return stream.next_layer().read_some(asio::buffer(s, n));
        }
        std::streamsize write(const char* s, std::streamsize n)
        {
            handshake(ssl::stream_base::client); // HTTPS clients write first
            if (fUseSSL) return asio::write(stream, asio::buffer(s, n));
            return asio::write(stream.next_layer(), asio::buffer(s, n));
        }
        bool connect(const std::string& server, const std::string& port)
        {
            ip::tcp::resolver resolver(stream.get_io_service());
            ip::tcp::resolver::query query(server.c_str(), port.c_str());
            ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            ip::tcp::resolver::iterator end;
            boost::system::error_code error = asio::error::host_not_found;
            while (error && endpoint_iterator != end)
            {
                stream.lowest_layer().close();
                stream.lowest_layer().connect(*endpoint_iterator++, error);
            }
            if (error)
                return false;
            return true;
        }

    private:
        bool fNeedHandshake;
        bool fUseSSL;
        SSLStream& stream;
    };

    void ThreadRPCServer(void* parg)
    {
        IMPLEMENT_RANDOMIZE_STACK(ThreadRPCServer(parg));


        try
        {
            vnThreadsRunning[THREAD_RPCSERVER]++;
            ThreadRPCServer2(parg);
            vnThreadsRunning[THREAD_RPCSERVER]--;
        }
        catch (std::exception& e) {
            vnThreadsRunning[THREAD_RPCSERVER]--;
            PrintException(&e, "ThreadRPCServer()");
        } catch (...) {
            vnThreadsRunning[THREAD_RPCSERVER]--;
            PrintException(NULL, "ThreadRPCServer()");
        }

        //delete pMiningKey; pMiningKey = NULL;

        printf("ThreadRPCServer exiting\n");
    }

    void ThreadRPCServer2(void* parg)
    {
        printf("ThreadRPCServer started\n");

        strRPCUserColonPass = mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"];
        if (mapArgs["-rpcpassword"] == "")
        {
            unsigned char rand_pwd[32];
            RAND_bytes(rand_pwd, 32);
            string strWhatAmI = "To use Nexus";
            if (mapArgs.count("-server"))
                strWhatAmI = strprintf(_("To use the %s option"), "\"-server\"");
            else if (mapArgs.count("-daemon"))
                strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");
            ThreadSafeMessageBox(strprintf(
                _("%s, you must set a rpcpassword in the configuration file:\n %s\n"
                  "It is recommended you use the following random password:\n"
                  "rpcuser=rpcserver\n"
                  "rpcpassword=%s\n"
                  "(you do not need to remember this password)\n"
                  "If the file does not exist, create it with owner-readable-only file permissions.\n"),
                    strWhatAmI.c_str(),
                    GetConfigFile().string().c_str(),
                    Wallet::EncodeBase58(&rand_pwd[0],&rand_pwd[0]+32).c_str()),
                _("Error"), wxOK | wxMODAL);
            StartShutdown();
            return;
        }

        bool fUseSSL = GetBoolArg("-rpcssl");
        asio::ip::address bindAddress = mapArgs.count("-rpcallowip") ? asio::ip::address_v4::any() : asio::ip::address_v4::loopback();

        asio::io_service io_service;
        ip::tcp::endpoint endpoint(bindAddress, GetArg("-rpcport", fTestNet? TESTNET_RPC_PORT : RPC_PORT));
        ip::tcp::acceptor acceptor(io_service);
        try
        {
            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen(socket_base::max_connections);
        }
        catch(boost::system::system_error &e)
        {
            ThreadSafeMessageBox(strprintf(_("An error occured while setting up the RPC port %i for listening: %s"), endpoint.port(), e.what()),
                                 _("Error"), wxOK | wxMODAL);
            StartShutdown();
            return;
        }

        ssl::context context(ssl::context::sslv23);
        if (fUseSSL)
        {
            context.set_options(ssl::context::no_sslv2);

            filesystem::path pathCertFile(GetArg("-rpcsslcertificatechainfile", "server.cert"));
            if (!pathCertFile.is_complete()) pathCertFile = filesystem::path(GetDataDir()) / pathCertFile;
            if (filesystem::exists(pathCertFile)) context.use_certificate_chain_file(pathCertFile.string());
            else printf("ThreadRPCServer ERROR: missing server certificate file %s\n", pathCertFile.string().c_str());

            filesystem::path pathPKFile(GetArg("-rpcsslprivatekeyfile", "server.pem"));
            if (!pathPKFile.is_complete()) pathPKFile = filesystem::path(GetDataDir()) / pathPKFile;
            if (filesystem::exists(pathPKFile)) context.use_private_key_file(pathPKFile.string(), ssl::context::pem);
            else printf("ThreadRPCServer ERROR: missing server private key file %s\n", pathPKFile.string().c_str());

            string strCiphers = GetArg("-rpcsslciphers", "TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH");
            SSL_CTX_set_cipher_list(context.native_handle(), strCiphers.c_str());
        }

        loop() {
            // Accept connection
            SSLStream sslStream(io_service, context);
            SSLIOStreamDevice d(sslStream, fUseSSL);
            iostreams::stream<SSLIOStreamDevice> stream(d);

            ip::tcp::endpoint peer;
            vnThreadsRunning[THREAD_RPCSERVER]--;
            acceptor.accept(sslStream.lowest_layer(), peer);
            vnThreadsRunning[4]++;
            if (fShutdown)
                return;

            // Restrict callers by IP
            if (!ClientAllowed(peer.address().to_string()))
            {
                // Only send a 403 if we're not using SSL to prevent a DoS during the SSL handshake.
                if (!fUseSSL)
                    stream << HTTPReply(403, "") << std::flush;

                printf("[RPC] Failed to Authorize New Connection.");
                continue;
            }

            map<string, string> mapHeaders;
            string strRequest;

            boost::thread api_caller(ReadHTTP, boost::ref(stream), boost::ref(mapHeaders), boost::ref(strRequest));
            if (!api_caller.timed_join(boost::posix_time::seconds(GetArg("-rpctimeout", 30))))
            {   // Timed out:
                acceptor.cancel();
                printf("ThreadRPCServer ReadHTTP timeout\n");
                continue;
            }

            // Check authorization
            if (mapHeaders.count("authorization") == 0)
            {
                stream << HTTPReply(401, "") << std::flush;
                continue;
            }
            if (!HTTPAuthorized(mapHeaders))
            {
                printf("ThreadRPCServer incorrect password attempt from %s\n",peer.address().to_string().c_str());
                /* Deter brute-forcing short passwords.
                   If this results in a DOS the user really
                   shouldn't have their RPC port exposed.*/
                if (mapArgs["-rpcpassword"].size() < 20)
                    Sleep(250);

                stream << HTTPReply(401, "") << std::flush;
                continue;
            }

            Value id = Value::null;
            try
            {
                // Parse request
                Value valRequest;
                if (!read_string(strRequest, valRequest) || valRequest.type() != obj_type)
                    throw JSONRPCError(-32700, "Parse error");
                const Object& request = valRequest.get_obj();

                // Parse id now so errors from here on will have the id
                id = find_value(request, "id");

                // Parse method
                Value valMethod = find_value(request, "method");
                if (valMethod.type() == null_type)
                    throw JSONRPCError(-32600, "Missing method");
                if (valMethod.type() != str_type)
                    throw JSONRPCError(-32600, "Method must be a string");
                string strMethod = valMethod.get_str();
                printf("ThreadRPCServer method=%s\n", strMethod.c_str());

                // Parse params
                Value valParams = find_value(request, "params");
                Array params;
                if (valParams.type() == array_type)
                    params = valParams.get_array();
                else if (valParams.type() == null_type)
                    params = Array();
                else
                    throw JSONRPCError(-32600, "Params must be an array");

                Value result = tableRPC.execute(strMethod, params);

                // Send reply
                string strReply = JSONRPCReply(result, Value::null, id);
                stream << HTTPReply(200, strReply) << std::flush;
            }
            catch (Object& objError)
            {
                ErrorReply(stream, objError, id);
            }
            catch (std::exception& e)
            {
                ErrorReply(stream, JSONRPCError(-32700, e.what()), id);
            }
        }
    }

    json_spirit::Value CRPCTable::execute(const std::string &strMethod, const json_spirit::Array &params) const
    {
        // Find method
        const CRPCCommand *pcmd = tableRPC[strMethod];
        if (!pcmd)
            throw JSONRPCError(-32601, "Method not found");

        // Observe safe mode
        string strWarning = Core::GetWarnings("rpc");
        if (strWarning != "" && !GetBoolArg("-disablesafemode") &&
            !pcmd->okSafeMode)
            throw JSONRPCError(-2, string("Safe mode: ") + strWarning);

        try
        {
            // Execute
            Value result;
            {
                LOCK2(Core::cs_main, pwalletMain->cs_wallet);
                result = pcmd->actor(params, false);
            }
            return result;
        }
        catch (std::exception& e)
        {
            throw JSONRPCError(-1, e.what());
        }
    }


    Object CallRPC(const string& strMethod, const Array& params)
    {
        if (mapArgs["-rpcuser"] == "" && mapArgs["-rpcpassword"] == "")
            throw runtime_error(strprintf(
                _("You must set rpcpassword=<password> in the configuration file:\n%s\n"
                  "If the file does not exist, create it with owner-readable-only file permissions."),
                    GetConfigFile().string().c_str()));

        // Connect to localhost
        bool fUseSSL = GetBoolArg("-rpcssl");
        asio::io_service io_service;
        ssl::context context(ssl::context::sslv23);
        context.set_options(ssl::context::no_sslv2);
        SSLStream sslStream(io_service, context);
        SSLIOStreamDevice d(sslStream, fUseSSL);
        iostreams::stream<SSLIOStreamDevice> stream(d);
        if (!d.connect(GetArg("-rpcconnect", "127.0.0.1"), GetArg("-rpcport", CBigNum(fTestNet? TESTNET_RPC_PORT : RPC_PORT).ToString().c_str())))
            throw runtime_error("couldn't connect to server");

        // HTTP basic authentication
        string strUserPass64 = EncodeBase64(mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"]);
        map<string, string> mapRequestHeaders;
        mapRequestHeaders["Authorization"] = string("Basic ") + strUserPass64;

        // Send request
        string strRequest = JSONRPCRequest(strMethod, params, 1);
        string strPost = HTTPPost(strRequest, mapRequestHeaders);
        stream << strPost << std::flush;

        // Receive reply
        map<string, string> mapHeaders;
        string strReply;
        int nStatus = ReadHTTP(stream, mapHeaders, strReply);
        if (nStatus == 401)
            throw runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
        else if (nStatus >= 400 && nStatus != 400 && nStatus != 404 && nStatus != 500)
            throw runtime_error(strprintf("server returned HTTP error %d", nStatus));
        else if (strReply.empty())
            throw runtime_error("no response from server");

        // Parse reply
        Value valReply;
        if (!read_string(strReply, valReply))
            throw runtime_error("couldn't parse reply from server");
        const Object& reply = valReply.get_obj();
        if (reply.empty())
            throw runtime_error("expected reply to have result, error and id properties");

        return reply;
    }




    template<typename T>
    void ConvertTo(Value& value)
    {
        if (value.type() == str_type)
        {
            // reinterpret string as unquoted json value
            Value value2;
            if (!read_string(value.get_str(), value2))
                throw runtime_error("type mismatch");
            value = value2.get_value<T>();
        }
        else
        {
            value = value.get_value<T>();
        }
    }

    // Convert strings to command-specific RPC representation
    Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
    {
        Array params;
        BOOST_FOREACH(const std::string &param, strParams)
            params.push_back(param);

        int n = params.size();

        //
        // Special case non-string parameter types
        //
        if (strMethod == "setgenerate"            && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "dumprichlist"           && n > 0) ConvertTo<int>(params[0]);
        if (strMethod == "setgenerate"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "sendtoaddress"          && n > 1) ConvertTo<double>(params[1]);
        if (strMethod == "settxfee"               && n > 0) ConvertTo<double>(params[0]);
        if (strMethod == "getreceivedbyaddress"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "getreceivedbyaccount"   && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "listreceivedbyaddress"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listreceivedbyaddress"  && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "listreceivedbyaccount"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listreceivedbyaccount"  && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "getbalance"             && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "getblockhash"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "getblock"               && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "move"                   && n > 2) ConvertTo<double>(params[2]);
        if (strMethod == "move"                   && n > 3) ConvertTo<boost::int64_t>(params[3]);
        if (strMethod == "sendfrom"               && n > 2) ConvertTo<double>(params[2]);
        if (strMethod == "sendfrom"               && n > 3) ConvertTo<boost::int64_t>(params[3]);
        if (strMethod == "listtransactions"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "listtransactions"       && n > 2) ConvertTo<boost::int64_t>(params[2]);
        if (strMethod == "totaltransactions"      && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "gettransactions"        && n > 1) ConvertTo<int>(params[1]);
        if (strMethod == "gettransactions"        && n > 2) ConvertTo<bool>(params[2]);
        if (strMethod == "listNTransactions"      && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listaccounts"           && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listunspent"            && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listunspent"            && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "walletpassphrase"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "walletpassphrase"       && n > 2) ConvertTo<bool>(params[2]);
        if (strMethod == "listsinceblock"         && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "sendmany"               && n > 1)
        {
            string s = params[1].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != obj_type)
                throw runtime_error("type mismatch");
            params[1] = v.get_obj();
        }
        if (strMethod == "listunspent"           && n > 2)
        {
            string s = params[2].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != array_type)
                throw runtime_error("type mismatch "+s);
            params[2] = v.get_array();
        }

        if (strMethod == "importkeys"             && n > 0)
        {
            string s = params[0].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != obj_type)
                throw runtime_error("type mismatch");
            params[0] = v.get_obj();
        }

        if (strMethod == "sendmany"                && n > 2) ConvertTo<boost::int64_t>(params[2]);
        if (strMethod == "reservebalance"          && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "reservebalance"          && n > 1) ConvertTo<double>(params[1]);
        if (strMethod == "addmultisigaddress"      && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "addmultisigaddress"      && n > 1)
        {
            string s = params[1].get_str();
            Value v;
            if (!read_string(s, v) || v.type() != array_type)
                throw runtime_error("type mismatch "+s);
            params[1] = v.get_array();
        }
        return params;
    }

    int CommandLineRPC(int argc, char *argv[])
    {
        string strPrint;
        int nRet = 0;
        try
        {
            // Skip switches
            while (argc > 1 && IsSwitchChar(argv[1][0]))
            {
                argc--;
                argv++;
            }

            // Method
            if (argc < 2)
                throw runtime_error("too few parameters");
            string strMethod = argv[1];

            // Parameters default to strings
            std::vector<std::string> strParams(&argv[2], &argv[argc]);
            Array params = RPCConvertValues(strMethod, strParams);

            // Execute
            Object reply = CallRPC(strMethod, params);

            // Parse reply
            const Value& result = find_value(reply, "result");
            const Value& error  = find_value(reply, "error");

            if (error.type() != null_type)
            {
                // Error
                strPrint = "error: " + write_string(error, false);
                int code = find_value(error.get_obj(), "code").get_int();
                nRet = abs(code);
            }
            else
            {
                // Result
                if (result.type() == null_type)
                    strPrint = "";
                else if (result.type() == str_type)
                    strPrint = result.get_str();
                else
                    strPrint = write_string(result, true);
            }
        }
        catch (std::exception& e)
        {
            strPrint = string("error: ") + e.what();
            nRet = 87;
        }
        catch (...)
        {
            PrintException(NULL, "CommandLineRPC()");
        }

        if (strPrint != "")
        {
            fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
        }
        return nRet;
    }



    const CRPCTable tableRPC;

}

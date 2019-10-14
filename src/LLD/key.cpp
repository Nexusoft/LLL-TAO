/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/templates/key.h>
#include <LLD/include/enum.h>

namespace LLD
{

    /* Default Constructor. */
    SectorKey::SectorKey()
    : nState       (0)
    , nLength      (0)
    , nSectorFile  (0)
    , nSectorSize  (0)
    , nSectorStart (0)
    , vKey         ( )
    , nTimestamp   (0)
    {
    }


    /* Copy Constructor */
    SectorKey::SectorKey(const SectorKey& key)
    : nState       (key.nState)
    , nLength      (key.nLength)
    , nSectorFile  (key.nSectorFile)
    , nSectorSize  (key.nSectorSize)
    , nSectorStart (key.nSectorStart)
    , vKey         (key.vKey)
    , nTimestamp   (key.nTimestamp)
    {
    }


    /* Move Constructor */
    SectorKey::SectorKey(SectorKey&& key) noexcept
    : nState       (std::move(key.nState))
    , nLength      (std::move(key.nLength))
    , nSectorFile  (std::move(key.nSectorFile))
    , nSectorSize  (std::move(key.nSectorSize))
    , nSectorStart (std::move(key.nSectorStart))
    , vKey         (std::move(key.vKey))
    , nTimestamp   (std::move(key.nTimestamp))
    {
    }


    /* Copy Assignment Operator */
    SectorKey& SectorKey::operator=(const SectorKey& key)
    {
        nState          = key.nState;
        nLength         = key.nLength;
        nSectorFile     = key.nSectorFile;
        nSectorSize     = key.nSectorSize;
        nSectorStart    = key.nSectorStart;
        vKey            = key.vKey;
        nTimestamp      = key.nTimestamp;

        return *this;
    }


    /* Move Assignment Operator */
    SectorKey& SectorKey::operator=(SectorKey&& key) noexcept
    {
        nState          = std::move(key.nState);
        nLength         = std::move(key.nLength);
        nSectorFile     = std::move(key.nSectorFile);
        nSectorSize     = std::move(key.nSectorSize);
        nSectorStart    = std::move(key.nSectorStart);
        vKey            = std::move(key.vKey);
        nTimestamp      = std::move(key.nTimestamp);

        return *this;
    }


    /* Default Destructor */
    SectorKey::~SectorKey()
    {
    }


    /* Constructor */
    SectorKey::SectorKey(const uint8_t nStateIn, const std::vector<uint8_t>& vKeyIn,
              const uint16_t nSectorFileIn, const uint32_t nSectorStartIn, const uint32_t nSectorSizeIn)
    : nState(nStateIn)
    , nLength(static_cast<uint16_t>(vKeyIn.size()))
    , nSectorFile(nSectorFileIn)
    , nSectorSize(nSectorSizeIn)
    , nSectorStart(nSectorStartIn)
    , vKey(vKeyIn)
    , nTimestamp(0)
    {
    }


    /*  Set the key for this object. */
    void SectorKey::SetKey(const std::vector<uint8_t>& vKeyIn)
    {
        vKey = vKeyIn;
        nLength = static_cast<uint16_t>(vKey.size());
    }


    /*  Iterator to the beginning of the raw key. */
    uint32_t SectorKey::Begin() const
    {
        return 13;
    }


    /*  Return the size of the key sector on disk. */
    uint32_t SectorKey::Size() const
    {
        return (13 + nLength);
    }


    /*  Dump Key to Debug Console. */
    void SectorKey::Print() const
    {
        debug::log(0, "SectorKey(nState=", uint32_t(nState),
        ", nLength=", nLength,
        ", nSectorFile=", nSectorFile,
        ", nSectorSize=", nSectorSize,
        ", nSectorStart=)", nSectorStart);
    }


    /*  Determines if the key is in an empty state. */
    bool SectorKey::Empty() const
    {
        return (nState == STATE::EMPTY);
    }


    /*  Determines if the key is in a ready state. */
    bool SectorKey::Ready() const
    {
        return (nState == STATE::READY);
    }


    /*  Determines if the key is in a transaction state. */
    bool SectorKey::IsTxn() const
    {
        return (nState == STATE::TRANSACTION);
    }


}

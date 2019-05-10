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

    /** Default Constructor. **/
    SectorKey::SectorKey()
    : nState(0)
    , nLength(0)
    , nSectorFile(0)
    , nSectorSize(0)
    , nSectorStart(0)
    {
    }


    /** Constructor **/
    SectorKey::SectorKey(uint8_t nStateIn,
              std::vector<uint8_t> vKeyIn,
              uint16_t nSectorFileIn,
              uint32_t nSectorStartIn,
              uint32_t nSectorSizeIn)
    : nState(nStateIn)
    , nLength(static_cast<uint16_t>(vKeyIn.size()))
    , nSectorFile(nSectorFileIn)
    , nSectorSize(nSectorSizeIn)
    , nSectorStart(nSectorStartIn)
    , vKey(vKeyIn)
    {
    }


    /** Default Destructor **/
    SectorKey::~SectorKey()
    {

    }


    /** Copy Assignment Operator **/
    SectorKey& SectorKey::operator=(const SectorKey& key)
    {
        nState          = key.nState;
        nLength         = key.nLength;
        nSectorFile     = key.nSectorFile;
        nSectorSize     = key.nSectorSize;
        nSectorStart    = key.nSectorStart;
        vKey            = key.vKey;

        return *this;
    }


    /** Default Copy Constructor **/
    SectorKey::SectorKey(const SectorKey& key)
    {
        nState          = key.nState;
        nLength         = key.nLength;
        nSectorFile     = key.nSectorFile;
        nSectorSize     = key.nSectorSize;
        nSectorStart    = key.nSectorStart;
        vKey            = key.vKey;
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

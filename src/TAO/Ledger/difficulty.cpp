/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/constants.h>

#include <TAO/Ledger/types/state.h>

#include <Legacy/include/money.h>

namespace TAO::Ledger
{

	/* Break the Chain Age in Minutes into Days, Hours, and Minutes. */
	void GetChainTimes(uint32_t nAge, uint32_t& nDays, uint32_t& nHours, uint32_t& nMinutes)
	{
		nDays = nAge / 1440;
		nHours = (nAge - (nDays * 1440)) / 60;
		nMinutes = nAge % 60;
	}


	/* Determines the Decimal of nBits per Channel for a decent "Frame of Reference". */
	double GetDifficulty(uint32_t nBits, int32_t nChannel)
	{

		/* Prime Channel is just Decimal Held in Integer
			Multiplied and Divided by Significant Digits. */
		if(nChannel == 1)
			return nBits / 10000000.0;

		/* Check for divide by zero. */
		if(nBits == 0)
			return 0.0;

		/* Get the Proportion of the Bits First. */
		double dDiff = (double)0x0000ffff / (double)(nBits & 0x00ffffff);

		/* Calculate where on Compact Scale Difficulty is. */
		uint32_t nShift = nBits >> 24;

		/* Shift down if Position on Compact Scale is above 124. */
		while(nShift > 124)
		{
			dDiff = dDiff / 256.0;
			nShift--;
		}

		/* Shift up if Position on Compact Scale is below 124. */
		while(nShift < 124)
		{
			dDiff = dDiff * 256.0;
			nShift++;
		}

		/* Offset the number by 64 to give larger starting reference. */
		return dDiff * ((nChannel == 2) ? 64 : 1024 * 1024 * 256);
	}



	/* Gets a block time from a weighted average at given depth. */
    uint64_t GetWeightedTimes(const BlockState state, uint32_t nDepth)
	{
		uint32_t nIterator = 0, nWeightedAverage = 0;

        /* Find the introductory block. */
        BlockState first = state;
		for(int nIndex = nDepth; nIndex > 0; nIndex--)
		{
            /* Find the previous block. */
            BlockState last = first.Prev();
			if(!GetLastState(last, state.GetChannel()))
				break;

            /* Calculate the time. */
			uint32_t nTime = (uint32_t)std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t)1) * nIndex * 3;
            first = last;

            /* Weight the iterator based on the weight constant. */
			nIterator += (nIndex * 3);
			nWeightedAverage += nTime;
		}

        /* Calculate the weighted average. */
		nWeightedAverage /= nIterator;

		return nWeightedAverage;
	}


	/* Switching function for each difficulty re-target [each channel uses their own version] */
    uint32_t GetNextTargetRequired(const BlockState state, int nChannel)
	{
		if(nChannel == 0)
			return RetargetTrust(state);

		else if(nChannel == 1)
			return RetargetPrime(state);

		else if(nChannel == 2)
			return RetargetHash(state);

		return 0;
	}


	/* Trust Retargeting: Modulate Difficulty based on production rate. */
    uint32_t RetargetTrust(const BlockState state)
	{

		/* Get Last Block Index [1st block back in Channel]. **/
		BlockState first = state;
		if (!GetLastState(first, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Get Last Block Index [2nd block back in Channel]. */
        BlockState last = first.Prev();
		if (!GetLastState(last, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Get the Block Time and Target Spacing. */
		uint64_t nBlockTime   = GetWeightedTimes(first, 5);
		uint64_t nBlockTarget = STAKE_TARGET_SPACING;


		/* The Upper and Lower Bound Adjusters. */
		uint64_t nUpperBound = nBlockTarget;
		uint64_t nLowerBound = nBlockTarget;


		/** If the time is above target, reduce difficulty by modular
            of one interval past timespan multiplied by maximum decrease. **/
        if(nBlockTime >= nBlockTarget)
        {
            /** Take the Minimum overlap of Target Timespan to make that maximum interval. **/
            uint64_t nOverlap = (uint64_t)std::min((nBlockTime - nBlockTarget), (nBlockTarget * 2));

            /** Get the Mod from the Proportion of Overlap in one Interval. **/
            double nProportions = (double)nOverlap / (nBlockTarget * 2);

            /** Get Mod from Maximum Decrease Equation with Decimal portions multiplied by Propotions. **/
            double nMod = 1.0 - (0.15 * nProportions);
            nLowerBound = nBlockTarget * nMod;
        }

        /** If the time is below target, increase difficulty by modular
            of interval of 1 - Block Target with time of 1 being maximum increase **/
        else
        {
            /** Get the overlap in reference from Target Timespan. **/
            uint64_t nOverlap = nBlockTarget - nBlockTime;

            /** Get the mod from overlap proportion. Time of 1 will be closest to mod of 1. **/
            double nProportions = (double) nOverlap / nBlockTarget;

            /** Get the Mod from the Maximum Increase Equation with Decimal portion multiplied by Proportions. **/
            double nMod = 1.0 + (nProportions * 0.075);
            nLowerBound = nBlockTarget * nMod;
        }


		/* Get the Difficulty Stored in Bignum Compact. */
		LLC::CBigNum bnNew;
		bnNew.SetCompact(first.nBits);


		/* Change Number from Upper and Lower Bounds. */
		bnNew *= nUpperBound;
		bnNew /= nLowerBound;


		/* Don't allow Difficulty to decrease below minimum. */
		if (bnNew > bnProofOfWorkLimit[0])
			bnNew = bnProofOfWorkLimit[0];


		/* Verbose Debug Output. */
		uint32_t nDays, nHours, nMinutes;
		GetChainTimes(GetChainAge(first.GetBlockTime()), nDays, nHours, nMinutes);

		debug::log(3,
			"RETARGET weighted time=%" PRId64 " actual time =%" PRId64 "[%f %%]"
			"\tchain time: [%" PRId64 " / %" PRId64 "]"
			"\tdifficulty: [%f to %f]"
			"\ttrust height: %" PRId64 " [AGE %u days, %u hours, %u minutes]",

			nBlockTime, std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t) 1), ((100.0 * nLowerBound) / nUpperBound),
			nBlockTarget, nBlockTime,
			GetDifficulty(first.nBits, 0), GetDifficulty(bnNew.GetCompact(), 0),
			first.nChannelHeight, nDays, nHours, nMinutes);

		return bnNew.GetCompact();
	}



	/* Prime Retargeting: Modulate Difficulty based on production rate. */
	uint32_t RetargetPrime(const BlockState state)
	{

		/* Get Last Block Index [1st block back in Channel]. **/
		BlockState first = state;
		if (!GetLastState(first, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Get Last Block Index [2nd block back in Channel]. */
        BlockState last = first.Prev();
		if (!GetLastState(last, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Standard Time Proportions */
		uint64_t nBlockTime = ((state.nVersion >= 4) ?
			GetWeightedTimes(first, 5) : std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t)1));
		uint64_t nBlockTarget = nTargetTimespan;


		/* Chain Mod: Is a proportion to reflect outstanding released funds. Version 1 Deflates difficulty slightly
			to allow more blocks through when blockchain has been slow, Version 2 Deflates Target Timespan to lower the minimum difficulty.
			This helps stimulate transaction processing while helping get the Nexus production back on track */
		double nChainMod = GetFractionalSubsidy(GetChainAge(first.GetBlockTime()), 0,
			((state.nVersion >= 3) ? 40.0 : 20.0)) / (first.nReleasedReserve[0] + 1);

		nChainMod = std::min(nChainMod, 1.0);
		nChainMod = std::max(nChainMod, (state.nVersion == 1) ? 0.75 : 0.5);


		/* Enforce Block Version 2 Rule. Chain mod changes block time requirements, not actual mod after block times. */
		if(state.nVersion >= 2)
			nBlockTarget *= nChainMod;


		/* These figures reduce the increase and decrease max and mins as difficulty rises
			this is due to the time difference between each cluster size [ex. 1, 2, 3] being 50x */
		double nDifficulty = GetDifficulty(first.nBits, 1);


		/* The Mod to Change Difficulty. */
		double nMod = 1.0;


		/* Handle for Version 3 Blocks. Mod determined by time multiplied by max / min. */
		if(state.nVersion >= 3)
		{

			/* If the time is above target, reduce difficulty by modular
				of one interval past timespan multiplied by maximum decrease. */
			if(nBlockTime >= nBlockTarget)
			{
				/* Take the Minimum overlap of Target Timespan to make that maximum interval. */
				uint64_t nOverlap = (uint64_t)std::min((nBlockTime - nBlockTarget), (nBlockTarget * 2));

				/* Get the Mod from the Proportion of Overlap in one Interval. */
				double nProportions = (double)nOverlap / (nBlockTarget * 2);

				/* Get Mod from Maximum Decrease Equation with Decimal portions multiplied by Propotions. */
				nMod = 1.0 - (nProportions * (0.5 / ((nDifficulty - 1) * 5.0)));
			}

			/* If the time is below target, increase difficulty by modular
				of interval of 1 - Block Target with time of 1 being maximum increase */
			else
			{
				/* Get the overlap in reference from Target Timespan. */
				uint64_t nOverlap = nBlockTarget - nBlockTime;

				/* Get the mod from overlap proportion. Time of 1 will be closest to mod of 1. */
				double nProportions = (double) nOverlap / nBlockTarget;

				/* Get the Mod from the Maximum Increase Equation with Decimal portion multiplied by Proportions. */
				nMod = 1.0 + (nProportions * (0.125 / ((nDifficulty - 1) * 10.0)));
			}

		}

		/* Handle for Block Version 2 Difficulty Adjustments. */
		else
		{
			/* Equations to Determine the Maximum Increase / Decrease. */
			double nMaxDown = 1.0 - (0.5 / ((nDifficulty - 1) * ((state.nVersion == 1) ? 10.0 : 25.0)));
			double nMaxUp = (0.125 / ((nDifficulty - 1) * 50.0)) + 1.0;

			/* Block Modular Determined from Time Proportions. */
			double nBlockMod = (double) nBlockTarget / nBlockTime;
			nBlockMod = std::min(nBlockMod, 1.125);
			nBlockMod = std::max(nBlockMod, 0.50);

			/* Version 1 Block, Chain Modular Modifies Block Modular. **/
			nMod = nBlockMod;
			if(state.nVersion == 1)
				nMod *= nChainMod;

			/* Set Modular to Max / Min values. */
			nMod = std::min(nMod, nMaxUp);
			nMod = std::max(nMod, nMaxDown);
		}


		/* If there is a change in difficulty, multiply by mod. */
		nDifficulty *= nMod;


		/* Keep the target difficulty at minimum (allow -regtest difficulty) */
		uint32_t nBits = SetBits(nDifficulty);
		if (nBits < bnProofOfWorkLimit[0].getuint())
			nBits < bnProofOfWorkLimit[0].getuint();

		/* Console Output */
		uint32_t nDays, nHours, nMinutes;
		GetChainTimes(GetChainAge(first.GetBlockTime()), nDays, nHours, nMinutes);

		debug::log(3,
			"RETARGET weighted time=%" PRId64 " actual time %" PRId64 ", [%f %%]"
			"\tchain time: [%" PRId64 " / %" PRId64 "]"
			"\treleased reward: %" PRId64 " [%f %%]"
			"\tdifficulty: [%f to %f]"
			"\tprime height: %" PRId64 " [AGE %u days, %u hours, %u minutes]",

			nBlockTime, std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t) 1), nMod * 100.0,
			nBlockTarget, nBlockTime,
			first.nReleasedReserve[0] / Legacy::COIN, 100.0 * nChainMod,
			GetDifficulty(first.nBits, 1), GetDifficulty(nBits, 1),
			first.nChannelHeight, nDays, nHours, nMinutes);


		return nBits;
	}



	/* Trust Retargeting: Modulate Difficulty based on production rate. */
	uint32_t RetargetHash(const BlockState state)
	{

		/* Get Last Block Index [1st block back in Channel]. **/
		BlockState first = state;
		if (!GetLastState(first, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Get Last Block Index [2nd block back in Channel]. */
        BlockState last = first.Prev();
		if (!GetLastState(last, 0))
			return bnProofOfWorkStart[0].GetCompact();


		/* Get the Block Times with Minimum of 1 to Prevent Time Warps. */
		uint64_t nBlockTime = ((state.nVersion >= 4) ?
			GetWeightedTimes(first, 5) : std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t) 1));

		uint64_t nBlockTarget = nTargetTimespan;


		/* Get the Chain Modular from Reserves. */
		double nChainMod = GetFractionalSubsidy(GetChainAge(first.GetBlockTime()), 0,
			((state.nVersion >= 3) ? 40.0 : 20.0)) / (first.nReleasedReserve[0] + 1);

		nChainMod = std::min(nChainMod, 1.0);
		nChainMod = std::max(nChainMod, (state.nVersion == 1) ? 0.75 : 0.5);


		/* Enforce Block Version 2 Rule. Chain mod changes block time requirements, not actual mod after block times. */
		if(state.nVersion >= 2)
			nBlockTarget *= nChainMod;


		/* The Upper and Lower Bound Adjusters. */
		uint64_t nUpperBound = nBlockTarget;
		uint64_t nLowerBound = nBlockTarget;


		/* Handle for Version 3 Blocks. Mod determined by time multiplied by max / min. */
		if(state.nVersion >= 3)
		{

			/* If the time is above target, reduce difficulty by modular
				of one interval past timespan multiplied by maximum decrease. */
			if(nBlockTime >= nBlockTarget)
			{
				/* Take the Minimum overlap of Target Timespan to make that maximum interval. */
				uint64_t nOverlap = (uint64_t)std::min((nBlockTime - nBlockTarget), (nBlockTarget * 2));

				/* Get the Mod from the Proportion of Overlap in one Interval. */
				double nProportions = (double)nOverlap / (nBlockTarget * 2);

				/* Get Mod from Maximum Decrease Equation with Decimal portions multiplied by Propotions. */
				double nMod = 1.0 - (((state.nVersion >= 4) ? 0.15 : 0.75) * nProportions);
				nLowerBound = nBlockTarget * nMod;
			}

			/* If the time is below target, increase difficulty by modular
				of interval of 1 - Block Target with time of 1 being maximum increase */
			else
			{
				/* Get the overlap in reference from Target Timespan. */
				uint64_t nOverlap = nBlockTarget - nBlockTime;

				/* Get the mod from overlap proportion. Time of 1 will be closest to mod of 1. */
				double nProportions = (double) nOverlap / nBlockTarget;

				/* Get the Mod from the Maximum Increase Equation with Decimal portion multiplied by Proportions. */
				double nMod = 1.0 + (nProportions * 0.075);
				nLowerBound = nBlockTarget * nMod;
			}
		}


		/* Handle for Version 2 Difficulty Adjustments. */
		else
		{
			double nBlockMod = (double) nBlockTarget / nBlockTime;
			nBlockMod = std::min(nBlockMod, 1.125);
			nBlockMod = std::max(nBlockMod, 0.75);

			/* Calculate the Lower Bounds. */
			nLowerBound = nBlockTarget * nBlockMod;

			/* Version 1 Blocks Change Lower Bound from Chain Modular. */
			if(state.nVersion == 1)
				nLowerBound *= nChainMod;

			/* Set Maximum [difficulty] up to 8%, and Minimum [difficulty] down to 50% */
			nLowerBound = std::min(nLowerBound, (uint64_t)(nUpperBound + (nUpperBound / 8)));
			nLowerBound = std::max(nLowerBound, (3 * nUpperBound ) / 4);
		}


		/* Get the Difficulty Stored in Bignum Compact. */
		LLC::CBigNum bnNew;
		bnNew.SetCompact(first.nBits);


		/* Change Number from Upper and Lower Bounds. */
		bnNew *= nUpperBound;
		bnNew /= nLowerBound;


		/* Don't allow Difficulty to decrease below minimum. */
		if (bnNew > bnProofOfWorkLimit[2])
            bnNew = bnProofOfWorkLimit[2];


		/* Console Output if Flagged. */
		uint32_t nDays, nHours, nMinutes;
		GetChainTimes(GetChainAge(first.GetBlockTime()), nDays, nHours, nMinutes);

		debug::log(3,
			"RETARGET weighted time=%" PRId64 " actual time %" PRId64 " [%f %%]"
			"\tchain time: [%" PRId64 " / %" PRId64 "]"
			"\treleased reward: %" PRId64 " [%f %%]"
			"\tdifficulty: [%f to %f]"
			"\thash height: %" PRId64 " [AGE %u days, %u hours, %u minutes]",

			nBlockTime, std::max(first.GetBlockTime() - last.GetBlockTime(), (uint64_t) 1), (100.0 * nLowerBound) / nUpperBound,
			nBlockTarget, nBlockTime,
			first.nReleasedReserve[0] / Legacy::COIN, 100.0 * nChainMod,
			GetDifficulty(first.nBits, 2), GetDifficulty(bnNew.GetCompact(), 2),
			first.nChannelHeight, nDays, nHours, nMinutes);

		return bnNew.GetCompact();
	}

}

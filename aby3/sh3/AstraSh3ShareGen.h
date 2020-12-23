#pragma once
#include "Sh3Types.h"
#include <cryptoTools/Crypto/PRNG.h>

namespace aby3
{
    struct AstraSh3ShareGen
    {
        void init(block prevSeed, block nextSeed, u64 buffSize = 256)
        {
      			mCommon.SetSeed(oc::toBlock(3488535245, 2454523));
      			mNextCommon.SetSeed(nextSeed);
      			mPrevCommon.SetSeed(prevSeed);

            mShareBuffPrevCommon.resize(buffSize);
            mShareBuffNextCommon.resize(buffSize);
            mShareBuffCommon.resize(buffSize);

            mShareGenPrevCommon.setKey(mPrevCommon.get<block>());
            mShareGenNextCommon.setKey(mNextCommon.get<block>());
            mShareGenCommon.setKey(mCommon.get<block>());

            refillBuffer(1, 0, 0);
            refillBuffer(0, 1, 0);
            refillBuffer(0, 0, 1);
        }

        void init(CommPkg& comm, block& seed, u64 buffSize = 256)
        {
            comm.mNext.asyncSendCopy(seed);
            block prevSeed;
            comm.mPrev.recv(prevSeed);
            init(prevSeed, seed, buffSize);
        }

        u64 mShareIdxPrevCommon = 0, mShareIdxNextCommon = 0, mShareIdxCommon = 0, mShareGenIdxPrevCommon = 0, mShareGenIdxNextCommon = 0, mShareGenIdxCommon = 0;
    		oc::PRNG mNextCommon, mPrevCommon, mCommon;
        oc::AES mShareGenNextCommon, mShareGenPrevCommon, mShareGenCommon;
        std::vector<block> mShareBuffNextCommon, mShareBuffPrevCommon, mShareBuffCommon;

        void refillBuffer(bool prevCommon, bool nextCommon, bool common)
        {
            if(prevCommon)
            {
              mShareGenPrevCommon.ecbEncCounterMode(mShareGenIdxPrevCommon, mShareBuffPrevCommon.size(), mShareBuffPrevCommon.data());
              mShareGenIdxPrevCommon += mShareBuffPrevCommon.size();
              mShareIdxPrevCommon = 0;
            }
            else if(nextCommon)
            {
              mShareGenNextCommon.ecbEncCounterMode(mShareGenIdxNextCommon, mShareBuffNextCommon.size(), mShareBuffNextCommon.data());
              mShareGenIdxNextCommon += mShareBuffNextCommon.size();
              mShareIdxNextCommon = 0;
            }
            else if(common)
            {
              mShareGenCommon.ecbEncCounterMode(mShareGenIdxCommon, mShareBuffCommon.size(), mShareBuffCommon.data());
              mShareGenIdxCommon += mShareBuffCommon.size();
              mShareIdxCommon = 0;
            }

        }

        i64 getShare(bool prevCommon, bool nextCommon, bool common, bool binary_share=0)
        {
            i64 ret;
            if(prevCommon)
            {
                if (mShareIdxPrevCommon + sizeof(i64) > mShareBuffPrevCommon.size() * sizeof(block))
                {
                    refillBuffer(prevCommon, nextCommon, common);
                }

                ret
                    = *(u64*)((u8*)mShareBuffPrevCommon.data() + mShareIdxPrevCommon);

                mShareIdxPrevCommon += sizeof(i64);

            }
            else if(nextCommon)
            {
                if (mShareIdxNextCommon + sizeof(i64) > mShareBuffNextCommon.size() * sizeof(block))
                {
                    refillBuffer(prevCommon, nextCommon, common);
                }

                ret
                    = *(u64*)((u8*)mShareBuffNextCommon.data() + mShareIdxNextCommon);

                mShareIdxNextCommon += sizeof(i64);

            }
            else if(common)
            {
                if (mShareIdxCommon + sizeof(i64) > mShareBuffCommon.size() * sizeof(block))
                {
                    refillBuffer(prevCommon, nextCommon, common);
                }

                ret
                    = *(u64*)((u8*)mShareBuffCommon.data() + mShareIdxCommon);

                mShareIdxCommon += sizeof(i64);

            }
            if(binary_share)
              return ret&1;
            else
              return ret;
        }

    };
}

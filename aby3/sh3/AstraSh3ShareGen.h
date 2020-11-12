#pragma once
#include "Sh3Types.h"
#include <cryptoTools/Crypto/PRNG.h>

namespace aby3
{
    struct AstraSh3ShareGen
    {
        void init(u64 buffSize = 256)
        {
      			f012.SetSeed(oc::toBlock(3488535245, 2454523));
            f01.SetSeed(oc::toBlock(0, 1));
            f12.SetSeed(oc::toBlock(1, 2));
            f02.SetSeed(oc::toBlock(0, 2));

            shareBuff01[0].resize(buffSize);
            shareBuff01[1].resize(buffSize);

            shareBuff12[0].resize(buffSize);
            shareBuff12[1].resize(buffSize);

            shareBuff02[0].resize(buffSize);
            shareBuff02[1].resize(buffSize);

            shareBuff012[0].resize(buffSize);
            shareBuff012[1].resize(buffSize);
            shareBuff012[2].resize(buffSize);
        
            auto shared_01 = f01.get<block>();
            shareGen01[0].setKey(shared_01);
            shareGen01[1].setKey(shared_01);
            
            auto shared_12 = f12.get<block>();
            shareGen12[0].setKey(shared_12);
            shareGen12[1].setKey(shared_12);

            auto shared_02 = f02.get<block>();
            shareGen02[0].setKey(shared_02);
            shareGen02[1].setKey(shared_02);
            
            auto shared_012 = f012.get<block>();
            shareGen012[0].setKey(shared_012);
            shareGen012[1].setKey(shared_012);
            shareGen012[2].setKey(shared_012);

            refillBuffer(0, 1, -1, 0);
            refillBuffer(0, 1, -1, 1);
            
            refillBuffer(1, 2, -1, 1);
            refillBuffer(1, 2, -1, 2);

            refillBuffer(0, 2, -1, 0);
            refillBuffer(0, 2, -1, 2);

            refillBuffer(0, 1, 2, 0);
            refillBuffer(0, 1, 2, 1);
            refillBuffer(0, 1, 2, 2);
        }

        std::array<u64, 2> shareIdx01 = {0, 0};
        std::array<u64, 2> shareIdx12 = {0, 0};
        std::array<u64, 2> shareIdx02 = {0, 0};
        std::array<u64, 3> shareIdx012 = {0, 0, 0};

        std::array<u64, 2> shareGenIdx01 = {0, 0};
        std::array<u64, 2> shareGenIdx12 = {0, 0};
        std::array<u64, 2> shareGenIdx02 = {0, 0};
        std::array<u64, 3> shareGenIdx012 = {0, 0, 0};

		    oc::PRNG f01, f12, f02, f012;

        std::array<oc::AES, 2> shareGen01;
        std::array<oc::AES, 2> shareGen12;
        std::array<oc::AES, 2> shareGen02;
        std::array<oc::AES, 3> shareGen012;

        std::array<std::vector<block>, 2> shareBuff01;
        std::array<std::vector<block>, 2> shareBuff12;
        std::array<std::vector<block>, 2> shareBuff02;
        std::array<std::vector<block>, 3> shareBuff012;

        void refillBuffer(int partyIdx1, int partyIdx2, int partyIdx3, int requesting_party)
        {
            if(partyIdx1 == 0 && partyIdx2 == 1 && partyIdx3 == 2)
            {
            
                shareGen012[requesting_party].ecbEncCounterMode(shareGenIdx012[requesting_party], shareBuff012[requesting_party].size(), shareBuff012[requesting_party].data());
                shareGenIdx012[requesting_party] += shareBuff012[requesting_party].size();
                shareIdx012[requesting_party] = 0;
            }
            else if(partyIdx1 == 0 && partyIdx2 == 1)
            {
                shareGen01[requesting_party].ecbEncCounterMode(shareGenIdx01[requesting_party], shareBuff01[requesting_party].size(), shareBuff01[requesting_party].data());
                shareGenIdx01[requesting_party] += shareBuff01[requesting_party].size();
                shareIdx01[requesting_party] = 0;
            }
            else if(partyIdx1 == 1 && partyIdx2 == 2)
            {
                shareGen12[requesting_party-1].ecbEncCounterMode(shareGenIdx12[requesting_party-1], shareBuff12[requesting_party-1].size(), shareBuff12[requesting_party-1].data());
                shareGenIdx12[requesting_party-1] += shareBuff12[requesting_party-1].size();
                shareIdx12[requesting_party-1] = 0;
            }
            else if(partyIdx1 == 0 && partyIdx2 == 2)
            {
              if(requesting_party == 0)
              {
                shareGen02[requesting_party].ecbEncCounterMode(shareGenIdx02[requesting_party], shareBuff02[requesting_party].size(), shareBuff02[requesting_party].data());
                shareGenIdx02[requesting_party] += shareBuff02[requesting_party].size();
                shareIdx02[requesting_party] = 0;
              }
              else if(requesting_party == 2)
              {
                shareGen02[requesting_party-1].ecbEncCounterMode(shareGenIdx02[requesting_party-1], shareBuff02[requesting_party-1].size(), shareBuff02[requesting_party-1].data());
                shareGenIdx02[requesting_party-1] += shareBuff02[requesting_party-1].size();
                shareIdx02[requesting_party-1] = 0;

              }
            }
        }

        i64 getShare(int partyIdx1, int partyIdx2, int partyIdx3, int requesting_party, bool binary_share=0)
        {
            i64 ret;
            if(partyIdx1 == 0 && partyIdx2 == 1 && partyIdx3 == 2)
            {
            
                if (shareIdx012[requesting_party] + sizeof(i64) > shareBuff012[requesting_party].size() * sizeof(block))
                {
                    refillBuffer(partyIdx1, partyIdx2, partyIdx3, requesting_party);
                }

                ret
                    = *(u64*)((u8*)shareBuff012[requesting_party].data() + shareIdx012[requesting_party]);

                shareIdx012[requesting_party] += sizeof(i64);
            }
            else if(partyIdx1 == 0 && partyIdx2 == 1)
            {
                
                if (shareIdx01[requesting_party] + sizeof(i64) > shareBuff01[requesting_party].size() * sizeof(block))
                {
                    refillBuffer(partyIdx1, partyIdx2, partyIdx3, requesting_party);
                }

                ret
                    = *(u64*)((u8*)shareBuff01[requesting_party].data() + shareIdx01[requesting_party]);

                shareIdx01[requesting_party] += sizeof(i64);
            }
            else if(partyIdx1 == 1 && partyIdx2 == 2)
            {

                if (shareIdx12[requesting_party-1] + sizeof(i64) > shareBuff12[requesting_party-1].size() * sizeof(block))
                {
                    refillBuffer(partyIdx1, partyIdx2, partyIdx3, requesting_party);
                }

                ret
                    = *(u64*)((u8*)shareBuff12[requesting_party-1].data() + shareIdx12[requesting_party-1]);

                shareIdx12[requesting_party-1] += sizeof(i64);
            }
            else if(partyIdx1 == 0 && partyIdx2 == 2)
            {
              if(requesting_party == 0)
              {

                if (shareIdx02[requesting_party] + sizeof(i64) > shareBuff02[requesting_party].size() * sizeof(block))
                {
                    refillBuffer(partyIdx1, partyIdx2, partyIdx3, requesting_party);
                }

                ret
                    = *(u64*)((u8*)shareBuff02[requesting_party].data() + shareIdx02[requesting_party]);

                shareIdx02[requesting_party] += sizeof(i64);
              }
              else if(requesting_party == 2)
              {

                if (shareIdx02[requesting_party-1] + sizeof(i64) > shareBuff02[requesting_party-1].size() * sizeof(block))
                {
                    refillBuffer(partyIdx1, partyIdx2, partyIdx3, requesting_party);
                }

                ret
                    = *(u64*)((u8*)shareBuff02[requesting_party-1].data() + shareIdx02[requesting_party-1]);

                shareIdx02[requesting_party-1] += sizeof(i64);
              }
            }
            if(binary_share)
              return ret&1;
            else
              return ret;
        }

    };
}

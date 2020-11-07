#include "Sh3Evaluator.h"
#include <cryptoTools/Crypto/PRNG.h>
#include <iomanip>
#include <cryptoTools/Common/Log.h>
using namespace oc;
namespace aby3
{
    void Sh3Evaluator::init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize)
    {
        mShareGen.init(prevSeed, nextSeed, buffSize);
        mPartyIdx = partyIdx;
        mOtPrev.setSeed(mShareGen.mNextCommon.get<block>());
        mOtNext.setSeed(mShareGen.mPrevCommon.get<block>());
    }

    void Sh3Evaluator::init(u64 partyIdx, CommPkg& comm, block seed, u64 buffSize)
    {
        mShareGen.init(comm, seed, buffSize);
        mPartyIdx = partyIdx;
        mOtPrev.setSeed(mShareGen.mNextCommon.get<block>());
        mOtNext.setSeed(mShareGen.mPrevCommon.get<block>());
    }

/*    void Sh3Evaluator::astra_preprocess_mult_matrix_step2_0(CommPkg& comm, si64Matrix share1, si64Matrix share2)
    {
        i64 y = 0,y1 = mShareGen.getShare(),y2;
        for (u64 i=0; i<share1.size(); ++i)
            y = y + (share1.mShares[0](i) + share1.mShares[1](i))*(share2.mShares[0](i) + share2.mShares[1](i));
        y2 = y - y1;
        comm.mNext.asyncSendCopy(y1);
        comm.mPrev.asyncSendCopy(y2);

    }
    
    void Sh3Evaluator::astra_binary_preprocess_mult_step2_0_0(CommPkg& comm, sb64 share1)
    {
        i64 y = 0,y1 = mShareGen.getShare(),y2;
        //for (u64 i=0; i<share1.size(); ++i)
          //  y = y + (share1.mShares[0](i) + share1.mShares[1](i))*(share2.mShares[0](i) + share2.mShares[1](i));
        y =  (share1.mData[0] ^ share1.mData[1] );
        y2 = y - y1;
        comm.mNext.asyncSendCopy(y1);
        comm.mPrev.asyncSendCopy(y2);

    }
    
    void Sh3Evaluator::astra_binary_preprocess_mult_step2_0_1(CommPkg& comm, sb64 share1, sb64 share2)
    {
        i64 y = 0,y1 = mShareGen.getShare(),y2;
        //for (u64 i=0; i<share1.size(); ++i)
          //  y = y + (share1.mShares[0](i) + share1.mShares[1](i))*(share2.mShares[0](i) + share2.mShares[1](i));
        y =  (share1.mData[0]  ^ share1.mData[1] ) * (share2.mData[0] + share2.mData[1]);
        y2 = y - y1;
        comm.mNext.asyncSendCopy(y1);
        comm.mPrev.asyncSendCopy(y2);  
    }

    i64 Sh3Evaluator::astra_preprocess_mult_matrix_step2(CommPkg& comm, int partyIdx)
    {
        i64 extra_term;
        if(partyIdx == 1)
        {
            comm.mPrev.recv(extra_term);
        }
        else if(partyIdx == 2)
        {
            comm.mNext.recv(extra_term);
        }
        return extra_term;
    }

    i64 Sh3Evaluator::astra_bit2a_online_mult(CommPkg& comm, si64 share1, si64 share2, i64 extra_term, i64 alpha_prod_share, int partyIdx)
    {
        i64 beta_prod_share;
        if(partyIdx == 1)
        {
            beta_prod_share = (share1.mData[1]*share2.mData[1]) -
            (share1.mData[1]*share2.mData[0]) -
            (share1.mData[0]*share2.mData[1]) + extra_term + alpha_prod_share;
        }
        else if(partyIdx == 2)
        {
            beta_prod_share = -(share1.mData[1]*share2.mData[0]) -
            (share2.mData[1]*share1.mData[0]) + extra_term +alpha_prod_share;
        }
        return beta_prod_share;
    }

    i64 Sh3Evaluator::astra_online_mult_matrix(CommPkg& comm, si64Matrix share1, si64Matrix share2, i64 extra_term, i64 alpha_prod_share, int partyIdx)
    {
        i64 beta_prod_share1, beta_prod_share2;
        if(partyIdx == 1)
        {
            beta_prod_share1 = (share1.mShares[1]*share2.mShares[1].transpose())(0) -
            (share1.mShares[1]*share2.mShares[0].transpose())(0) -
            (share1.mShares[0]*share2.mShares[1].transpose())(0) + extra_term + alpha_prod_share;
            comm.mNext.asyncSendCopy(beta_prod_share1);
            comm.mNext.recv(beta_prod_share2);
        }
        else if(partyIdx == 2)
        {
            beta_prod_share2 = -((share1.mShares[1]*share2.mShares[0].transpose())(0)) -
            ((share2.mShares[1]*share1.mShares[0].transpose())(0)) + extra_term +alpha_prod_share;
            comm.mPrev.recv(beta_prod_share1);
            comm.mPrev.asyncSendCopy(beta_prod_share2);
        }
        return beta_prod_share1+beta_prod_share2;
    }

    i64 Sh3Evaluator::astra_online_bit_injection(CommPkg& comm, sb64 b_Shares, si64 a_Shares, i64 alpha_b_share, i64 alpha_b_alpha_x_share, int partyIdx)
    {
        i64 result_share_1, result_share_2;
        if(partyIdx == 1)
        {
            //[Y]_1 = (beta_b * beta_x) - (beta_b * alpha_x) + ([alpha_b]_1 * beta_x) - ([alpha_b * alpha_x]_1) 
            //        + 2([alpha_b]_1 * beta_b * beta_x) + 2(beta_b * [alpha_b * alpha_x]_1)
            
            result_share_1 = (b_Shares.mData[1] * a_Shares.mData[1]) - (b_Shares.mData[1] * a_Shares.mData[0]) + (alpha_b_share * a_Shares.mData[1] )
                               - alpha_b_alpha_x_share + 2(alpha_b_share * b_Shares.mData[1] * a_Shares.mData[1]) + 2(b_Shares.mData[1] * alpha_b_alpha_x_share );
                
            comm.mNext.asyncSendCopy(result_share_1);
            comm.mNext.recv(result_share_2);
        }
        else if(partyIdx == 2)
        {
            
            //[Y]_2 = 0 - 0 + ((alpha_b - [alpha_b]_1) * beta_x) - (alpha_b * alpha_x - [alpha_b * alpha_x]_1) + 2((alpha_b - [alpha_b]_1) * beta_b * beta_x) 
   			//		  + 2(beta_b * (alpha_b * alpha_x - [alpha_b * alpha_x]_1))
            
            result_share_2 = (alpha_b_share * a_Shares.mData[1]) - alpha_b_alpha_x_share + 2(alpha_b_share * b_Shares.mData[1] * a_Shares.mData[1])
                              + 2((b_Shares.mData[1] * alpha_b_alpha_x_share);
            
            comm.mPrev.recv(result_share_1);
            comm.mPrev.asyncSendCopy(result_share_2);
        }
        return result_share_1+result_share_2;
    }

    i64 Sh3Evaluator::astra_reveal_mult_matrix(CommPkg& comm, int partyIdx, i64 beta_prod, i64 alpha_prod_share, si64 bias)
    {
        i64 other_alpha_prod_share,other_bias_share;
        if(partyIdx == 1)
        {
            comm.mNext.asyncSendCopy(alpha_prod_share);
            comm.mNext.recv(other_alpha_prod_share);
            comm.mNext.asyncSendCopy(bias.mData[0]);
            comm.mNext.recv(other_bias_share);
        }
        else if(partyIdx == 2)
        {
            comm.mPrev.recv(other_alpha_prod_share);
            comm.mPrev.asyncSendCopy(alpha_prod_share);
            comm.mPrev.recv(other_bias_share);
            comm.mPrev.asyncSendCopy(bias.mData[0]);
        }
        //oc::lout<<(beta_prod-(alpha_prod_share+other_alpha_prod_share))<<std::endl;
        return (beta_prod - (alpha_prod_share + other_alpha_prod_share) + bias.mData[1] - (bias.mData[0] + other_bias_share));
    }


    i64 Sh3Evaluator::astra_bit2a_reveal(CommPkg& comm, int partyIdx, i64 beta_prod_share, i64 alpha_prod_share, i64 alpha_share, i64 beta_share)
    {
        i64 other_beta_prod_share, other_alpha_prod_share, other_beta_share, other_alpha_share;
        if(partyIdx == 1)
        {
            comm.mNext.asyncSendCopy(alpha_prod_share);
            comm.mNext.recv(other_alpha_prod_share);
            comm.mNext.asyncSendCopy(beta_prod_share);
            comm.mNext.recv(other_beta_prod_share);
            comm.mNext.asyncSendCopy(alpha_share);
            comm.mNext.recv(other_alpha_share);
            comm.mNext.asyncSendCopy(beta_share);
            comm.mNext.recv(other_beta_share);
        }
        else if(partyIdx == 2)
        {
            comm.mPrev.asyncSendCopy(alpha_prod_share);
            comm.mPrev.recv(other_alpha_prod_share);
            comm.mPrev.asyncSendCopy(beta_prod_share);
            comm.mPrev.recv(other_beta_prod_share);
            comm.mPrev.asyncSendCopy(alpha_share);
            comm.mPrev.recv(other_alpha_share);
            comm.mPrev.asyncSendCopy(beta_share);
            comm.mPrev.recv(other_beta_share);
        }
        //oc::lout<<(beta_prod-(alpha_prod_share+other_alpha_prod_share))<<std::endl;
        return (beta_share + other_beta_share + alpha_share + other_alpha_share - 2*(beta_prod_share + other_beta_prod_share - alpha_prod_share - other_alpha_prod_share);
    }
*/
    //void Sh3Evaluator::mul(
    //	CommPkg& comm,
    //	const si64Matrix& A,
    //	const si64Matrix& B,
    //	si64Matrix& C)
    //{
    //	C.mShares[0]
    //		= A.mShares[0] * B.mShares[0]
    //		+ A.mShares[0] * B.mShares[1]
    //		+ A.mShares[1] * B.mShares[0];

    //	for (u64 i = 0; i < C.size(); ++i)
    //	{
    //		C.mShares[0](i) += mShareGen.getShare();
    //	}

    //	C.mShares[1].resizeLike(C.mShares[0]);

    //	comm.mNext.asyncSendCopy(C.mShares[0].data(), C.mShares[0].size());
    //	comm.mPrev.recv(C.mShares[1].data(), C.mShares[1].size());
    //}

    //CompletionHandle Sh3Evaluator::asyncMul(
    //    CommPkg& comm, 
    //    const si64Matrix & A, 
    //    const si64Matrix & B, 
    //    si64Matrix & C)
    //{
    //    C.mShares[0]
    //        = A.mShares[0] * B.mShares[0]
    //        + A.mShares[0] * B.mShares[1]
    //        + A.mShares[1] * B.mShares[0];

    //    for (u64 i = 0; i < C.size(); ++i)
    //    {
    //        C.mShares[0](i) += mShareGen.getShare();
    //    }

    //    C.mShares[1].resizeLike(C.mShares[0]);

    //    comm.mNext.asyncSendCopy(C.mShares[0].data(), C.mShares[0].size());
    //    auto fu = comm.mPrev.asyncRecv(C.mShares[1].data(), C.mShares[1].size()).share();

    //    return { [fu = std::move(fu)](){ fu.get(); } };
    //}


    Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64& A, const si64& B, si64& C)
    {
        return dependency.then([&](CommPkg& comm, Sh3Task self)
            {
                C[0]
                    = A[0] * B[0]
                    + A[0] * B[1]
                    + A[1] * B[0]
                    + mShareGen.getShare();

                comm.mNext.asyncSendCopy(C[0]);
                auto fu = comm.mPrev.asyncRecv(C[1]).share();

                self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self){
                    fu.get();
                });
            }).getClosure();
    }


    Sh3Task Sh3Evaluator::asyncMul(Sh3Task dependency, const si64Matrix& A, const si64Matrix& B, si64Matrix& C)
    {
        return dependency.then([&](CommPkg& comm, Sh3Task self)
            {
                C.mShares[0]
                    = A.mShares[0] * B.mShares[0]
                    + A.mShares[0] * B.mShares[1]
                    + A.mShares[1] * B.mShares[0];

                for (u64 i = 0; i < C.size(); ++i)
                {
                    C.mShares[0](i) += mShareGen.getShare();
                }

                C.mShares[1].resizeLike(C.mShares[0]);

                comm.mNext.asyncSendCopy(C.mShares[0].data(), C.mShares[0].size());
                auto fu = comm.mPrev.asyncRecv(C.mShares[1].data(), C.mShares[1].size()).share();

                self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self){
                    fu.get();
                });
            }).getClosure();
    }

    //std::string prettyShare(int partyIdx, i64 v0, i64 v1 = -1, i64 v2 = -1)
    //{
    //	std::array<u64, 3> shares;
    //	shares[partyIdx] = v0;
    //	shares[(partyIdx + 2) % 3] = v1;
    //	shares[(partyIdx + 1) % 3] = v2;

    //	std::stringstream ss;
    //	ss << "(";
    //	if (shares[0] == -1) ss << "               _ ";
    //	else ss << std::hex << std::setw(16) << std::setfill('0') << shares[0] << " ";
    //	if (shares[1] == -1) ss << "               _ ";
    //	else ss << std::hex << std::setw(16) << std::setfill('0') << shares[1] << " ";
    //	if (shares[2] == -1) ss << "               _)";
    //	else ss << std::hex << std::setw(16) << std::setfill('0') << shares[2] << ")";

    //	return ss.str();
    //}



    Sh3Task Sh3Evaluator::asyncMul(
        Sh3Task dep,
        const si64Matrix& a,
        const sbMatrix& b,
        si64Matrix& c)
    {
        return dep.then([&](CommPkg& comm, Sh3Task self) {
            switch (mPartyIdx)
            {
            case 0:
            {
                std::vector<std::array<i64, 2>> s0(a.size());
                BitVector c1(a.size());
                for (u64 i = 0; i < s0.size(); ++i)
                {
                    auto bb = b.mShares[0](i) ^ b.mShares[1](i);
                    //if (bb < 0 || bb > 1)
                    //	throw std::runtime_error(LOCATION);

                    auto zeroShare = mShareGen.getShare();

                    s0[i][bb] = zeroShare;
                    s0[i][bb ^ 1] = a.mShares[1](i) + zeroShare;

                    //std::cout << "b=(" << b.mShares[0](i) << ",  , " << b.mShares[1](i) << ")" << std::endl;
                    //std::cout << "s0[" << i << "] = " << bb * a.mShares[1](i) << std::endl;

                    c1[i] = static_cast<u8>(b.mShares[1](i));
                }


                //std::cout << "p0 " << s0[0][0] << " "<< s0[0][1] << " "<< c1 << std::endl;
                // share 0: from p0 to p1,p2
                mOtNext.send(comm.mNext, s0);
                mOtPrev.send(comm.mPrev, s0);

                // share 1: from p1 to p0,p2 
                mOtPrev.help(comm.mPrev, c1);



                auto fu1 = comm.mPrev.asyncRecv(c.mShares[0].data(), c.size()).share();
                i64* dd = c.mShares[1].data();
                auto fu2 = SharedOT::asyncRecv(comm.mNext, comm.mPrev, std::move(c1), { dd, i64(c.size()) }).share();

                self.then([
                    fu1 = std::move(fu1),
                        fu2 = std::move(fu2)]
                        (CommPkg& comm, Sh3Task self) mutable {
                        fu1.get();
                        fu2.get();
                    });
                break;
            }
            case 1:
            {
                std::vector<std::array<i64, 2>> s1(a.size());
                BitVector c0(a.size());
                for (u64 i = 0; i < s1.size(); ++i)
                {
                    auto bb = b.mShares[0](i) ^ b.mShares[1](i);
                    //if (bb < 0 || bb > 1)
                    //	throw std::runtime_error(LOCATION);
                    auto ss = mShareGen.getShare();

                    s1[i][bb] = ss;
                    s1[i][bb ^ 1] = (a.mShares[0](i) + a.mShares[1](i)) + ss;

                    //std::cout << "b=(   ," << b.mShares[0](i) << ",   )" << "  " << (a.mShares[0](i) + a.mShares[1](i)) << std::endl;
                    //std::cout << "s1[" << i << "] = " << bb * (a.mShares[0](i) + a.mShares[1](i)) << " = b *  (" <<a.mShares[0](i) <<" +  "<<a.mShares[1](i) <<")" << std::endl;

                    c0[i] = static_cast<u8>(b.mShares[0](i));
                }

                //std::cout << "p1 " << s1[0][0] << " " << s1[0][1] << " " << c0 << std::endl;


                // share 0: from p0 to p1,p2
                mOtNext.help(comm.mNext, c0);

                // share 1: from p1 to p0,p2 
                mOtNext.send(comm.mNext, s1);
                mOtPrev.send(comm.mPrev, s1);


                // share 0: from p0 to p1,p2
                i64* dd = c.mShares[0].data();
                auto fu1 = SharedOT::asyncRecv(comm.mPrev, comm.mNext, std::move(c0), { dd, i64(c.size()) }).share();

                // share 1:
                auto fu2 = comm.mNext.asyncRecv(c.mShares[1].data(), c.size()).share();

                self.then([
                    fu1 = std::move(fu1),
                        fu2 = std::move(fu2),
                        &c,
                        _2 = std::move(c0)]
                        (CommPkg& comm, Sh3Task self) mutable {
                        fu1.get();
                        fu2.get();
                        //std::cout << "P1.get() " << c.mShares[0](0) << " " << c.mShares[1](0) << std::endl;
                    });

                break;
            }
            case 2:
            {
                BitVector c0(a.size()), c1(a.size());
                std::vector<i64> s0(a.size()), s1(a.size());
                for (u64 i = 0; i < a.size(); ++i)
                {
                    c0[i] = static_cast<u8>(b.mShares[1](i));
                    c1[i] = static_cast<u8>(b.mShares[0](i));

                    s0[i] = s1[i] = mShareGen.getShare();
                }

                //std::cout << "p0 " << s0[0] << " " << c0 << " " << c1 << std::endl;


                // share 0: from p0 to p1,p2
                mOtPrev.help(comm.mPrev, c0);
                comm.mNext.asyncSend(std::move(s0));

                // share 1: from p1 to p0,p2 
                mOtNext.help(comm.mNext, c1);
                comm.mPrev.asyncSend(std::move(s1));

                // share 0: from p0 to p1,p2
                i64* dd0 = c.mShares[1].data();
                auto fu1 = SharedOT::asyncRecv(comm.mNext, comm.mPrev, std::move(c0), { dd0, i64(c.size()) }).share();

                // share 1: from p1 to p0,p2
                i64* dd1 = c.mShares[0].data();
                auto fu2 = SharedOT::asyncRecv(comm.mPrev, comm.mNext, std::move(c1), { dd1, i64(c.size()) }).share();

                self.then([
                    fu1 = std::move(fu1),
                        fu2 = std::move(fu2),
                        &c]
                        (CommPkg& comm, Sh3Task self) mutable {
                        fu1.get();
                        fu2.get();
                        //std::cout << "P1.get() " << c.mShares[0](0) << " " << c.mShares[1](0) << std::endl;
                    });
                break;
            }
            default:
                throw std::runtime_error(LOCATION);
            }
            }
        ).getClosure();
    }


    Sh3Task Sh3Evaluator::asyncMul(
        Sh3Task dep,
        const i64& a,
        const sbMatrix& b,
        si64Matrix& c)
    {
        return dep.then([&](CommPkg& comm, Sh3Task self) {
            if (b.bitCount() != 1)
                throw RTE_LOC;

            switch (mPartyIdx)
            {
            case 0:
            {
                std::vector<std::array<i64, 2>> s0(b.rows());
                for (u64 i = 0; i < s0.size(); ++i)
                {
                    auto bb = b.mShares[0](i) ^ b.mShares[1](i);
                    auto zeroShare = mShareGen.getShare();

                    s0[i][bb] = zeroShare;
                    s0[i][bb ^ 1] = a + zeroShare;
                }

                // share 0: from p0 to p1,p2
                mOtNext.send(comm.mNext, s0);
                mOtPrev.send(comm.mPrev, s0);

                auto fu1 = comm.mNext.asyncRecv(c.mShares[0].data(), c.size()).share();
                auto fu2 = comm.mPrev.asyncRecv(c.mShares[1].data(), c.size()).share();
                self.then([fu1 = std::move(fu1), fu2 = std::move(fu2)](CommPkg& _, Sh3Task __) mutable {
                    fu1.get();
                    fu2.get();
                });
                break;
            }
            case 1:
            {
                BitVector c0(b.rows());
                for (u64 i = 0; i < b.rows(); ++i)
                {
                    c.mShares[1](i) = mShareGen.getShare();
                    c0[i] = static_cast<u8>(b.mShares[0](i));
                }

                // share 0: from p0 to p1,p2
                mOtNext.help(comm.mNext, c0);
                comm.mPrev.asyncSendCopy(c.mShares[1].data(), c.size());

                i64* dd = c.mShares[0].data();
                auto fu1 = SharedOT::asyncRecv(comm.mPrev, comm.mNext, std::move(c0), { dd, i64(c.size()) }).share();
                self.then([fu1 = std::move(fu1)](CommPkg& _, Sh3Task __) mutable {
                    fu1.get();
                });

                break;
            }
            case 2:
            {
                BitVector c0(b.rows());
                for (u64 i = 0; i < b.rows(); ++i)
                {
                    c.mShares[0](i) = mShareGen.getShare();
                    c0[i] = static_cast<u8>(b.mShares[1](i));
                }

                // share 0: from p0 to p1,p2
                mOtPrev.help(comm.mPrev, c0);
                comm.mNext.asyncSendCopy(c.mShares[0].data(), c.size());

                i64* dd0 = c.mShares[1].data();
                auto fu1 = SharedOT::asyncRecv(comm.mNext, comm.mPrev, std::move(c0), { dd0, i64(c.size()) }).share();

                self.then([fu1 = std::move(fu1)](CommPkg& _, Sh3Task __) mutable {
                    fu1.get();
                });
                break;
            }
            default:
                throw std::runtime_error(LOCATION);
            }
            }
        ).getClosure();
    }

    TruncationPair Sh3Evaluator::getTruncationTuple(u64 xSize, u64 ySize, u64 d)
    {
        TruncationPair pair;
        if (DEBUG_disable_randomization)
        {
            pair.mR.resize(xSize, ySize);
            pair.mR.setZero();

            pair.mRTrunc.mShares[0].resize(xSize, ySize);
            pair.mRTrunc.mShares[1].resize(xSize, ySize);
            pair.mRTrunc.mShares[0].setZero();
            pair.mRTrunc.mShares[1].setZero();

        }
        else
        {
            const auto d2 = d + 2;
            pair.mR.resize(xSize, ySize);
            pair.mRTrunc.resize(xSize, ySize);
            const u64 mask = (~0ull) >> 1;
            //if (mPartyIdx == 0)
            {
                //mShareGen.mPrevCommon.get(pair.mR.data(), pair.mR.size());
                mShareGen.mNextCommon.get(pair.mRTrunc[0].data(), pair.mRTrunc[0].size());
                mShareGen.mPrevCommon.get(pair.mRTrunc[1].data(), pair.mRTrunc[1].size());
                for (u64 i = 0; i < pair.mR.size(); ++i)
                {
                    auto& t0 = pair.mRTrunc[0](i);
                    auto& t1 = pair.mRTrunc[1](i);
                    auto& r0 = pair.mR(i);

                    r0 = t0 >> 2;
                    t0 >>= d2;
                    t1 >>= d2;
                }
            }
            //else if (mPartyIdx == 1)
            //{
            //    mShareGen.mNextCommon.get(pair.mR.data(), pair.mR.size());
            //    mShareGen.mNextCommon.get(pair.mRTrunc[0].data(), pair.mRTrunc[0].size());
            //    pair.mRTrunc[1].setZero();
            //}
            //else
            //{
            //    i64Matrix R1(xSize, ySize);
            //    mShareGen.mNextCommon.get(pair.mR.data(), pair.mR.size());
            //    mShareGen.mPrevCommon.get(R1.data(), R1.size());

            //    mShareGen.mNextCommon.get(pair.mRTrunc[0].data(), pair.mRTrunc[0].size());
            //    mShareGen.mPrevCommon.get(pair.mRTrunc[1].data(), pair.mRTrunc[1].size());


            //    for (u64 i = 0; i < pair.mR.size(); i++)
            //    {
            //        auto rt = pair.mRTrunc[0](i) + pair.mRTrunc[1](i);


            //        pair.mR(i) = (-pair.mR(i) - R1(i))
            //         +(());
            //    }
            //}
        }
        return pair;
    }

    Sh3Task aby3::Sh3Evaluator::asyncMul(
        Sh3Task  dependency,
        const si64& A,
        const si64& B,
        si64& C,
        u64 shift)
    {
        return dependency.then([&, shift](CommPkg& comm, Sh3Task& self) -> void {

            auto truncationTuple = getTruncationTuple(1, 1, shift);

            auto abMinusR
                = A.mData[0] * B.mData[0]
                + A.mData[0] * B.mData[1]
                + A.mData[1] * B.mData[0];
            //+ mShareGen.getShare();


        //oc::ostreamLock(std::cout) << "ab " << mPartyIdx << ": " << abMinusR << " - "<< truncationTuple.mR(0) << 
        //	" = " << abMinusR - truncationTuple.mR(0) << std::endl;

            abMinusR -= truncationTuple.mR(0);
            C = truncationTuple.mRTrunc(0);


            //lout << mPartyIdx << " " << abMinusR << std::endl;
            //{
            //	comm.mPrev.asyncSend(truncationTuple.mR(0));
            //	comm.mNext.asyncSend(truncationTuple.mR(0));
            //	i64 l1, l2;
            //	comm.mPrev.recv(l1);
            //	comm.mNext.recv(l2);
            //	auto l = truncationTuple.mR(0) + l1 + l2;
            //	auto ls = (l / (1ll << C.mDecimal));
            //	comm.mPrev.asyncSend(truncationTuple.mRTrunc.mShares[0](0));
            //	i64 s2;
            //	comm.mNext.recv(s2);
            //	auto s = s2 + truncationTuple.mRTrunc.mShares[0](0) + truncationTuple.mRTrunc.mShares[1](0);
            //	oc::ostreamLock(std::cout) << "sss " << l << " " << s << " " << ls << " " << s - ls << std::endl;
            //}

            auto& rt = self.getRuntime();

            // reveal dependency.getRuntime().the value to party 0, 1
            auto next = (rt.mPartyIdx + 1) % 3;
            auto prev = (rt.mPartyIdx + 2) % 3;
            if (next < 2) comm.mNext.asyncSendCopy(abMinusR);
            if (prev < 2) comm.mPrev.asyncSendCopy(abMinusR);

            if (rt.mPartyIdx < 2)
            {
                // these will hold the three shares of r-xy
                std::unique_ptr<std::array<i64, 3>> shares(new std::array<i64, 3>);

                // perform the async receives
                auto fu0 = comm.mNext.asyncRecv((*shares)[0]).share();
                auto fu1 = comm.mPrev.asyncRecv((*shares)[1]).share();
                (*shares)[2] = abMinusR;

                // set the completion handle complete the computation
                self.then([fu0, fu1, shares = std::move(shares), &C, shift, this]
                (CommPkg& comm, Sh3Task self) mutable
                {
                    fu0.get();
                    fu1.get();

                    // xy-r
                    (*shares)[0] += (*shares)[1] + (*shares)[2];


                    // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
                    C.mData[mPartyIdx] += (*shares)[0] >> shift;

                    //lout << mPartyIdx << " " << C.mShare.mData[mPartyIdx] << std::endl
                    //	<<  "* " << C.mShare.mData[1^mPartyIdx] << std::endl;
                });
            }

            }).getClosure();
    }



    Sh3Task aby3::Sh3Evaluator::asyncMul(
        Sh3Task dependency,
        const si64Matrix& A,
        const si64Matrix& B,
        si64Matrix& C,
        u64 shift)
    {
        return dependency.then([&, shift](CommPkg& comm, Sh3Task& self) -> void {

            //oc::lout << self.mRuntime->mPartyIdx << " mult Send" << std::endl;

            i64Matrix abMinusR
                = A.mShares[0] * B.mShares[0]
                + A.mShares[0] * B.mShares[1]
                + A.mShares[1] * B.mShares[0];

            auto truncationTuple = getTruncationTuple(abMinusR.rows(), abMinusR.cols(), shift);

            abMinusR -= truncationTuple.mR;
            C.mShares = std::move(truncationTuple.mRTrunc.mShares);

            //lout << "p" << mPartyIdx << " ab \n" << abMinusR << std::endl;
                //<< "p"<< mPartyIdx << " c \n" << C.mShares[0]<<",\n"<< C.mShares[1] << std::endl;

            auto& rt = self.getRuntime();

            // reveal dependency.getRuntime().the value to party 0, 1
            auto next = (rt.mPartyIdx + 1) % 3;
            auto prev = (rt.mPartyIdx + 2) % 3;
            if (next < 2) comm.mNext.asyncSendCopy(abMinusR.data(), abMinusR.size());
            if (prev < 2) comm.mPrev.asyncSendCopy(abMinusR.data(), abMinusR.size());

            if (rt.mPartyIdx < 2)
            {
                // these will hold the three shares of r-xy
                //std::unique_ptr<std::array<i64, 3>> shares(new std::array<i64, 3>);
                auto shares = std::make_unique<std::array<i64Matrix, 3>>();

                //i64Matrix& rr = (*shares)[0]);

                (*shares)[0].resize(abMinusR.rows(), abMinusR.cols());
                (*shares)[1].resize(abMinusR.rows(), abMinusR.cols());

                // perform the async receives
                auto fu0 = comm.mNext.asyncRecv((*shares)[0].data(), (*shares)[0].size()).share();
                auto fu1 = comm.mPrev.asyncRecv((*shares)[1].data(), (*shares)[1].size()).share();
                (*shares)[2] = std::move(abMinusR);

                // set the completion handle complete the computation
                self.then([fu0, fu1, shares = std::move(shares), &C, shift, this]
                (CommPkg& comm, Sh3Task self) mutable
                {
                    //oc::lout << self.mRuntime->mPartyIdx << " mult recv" << std::endl;
                    fu0.get();
                    fu1.get();

                    //lout << "p" << mPartyIdx << " ab \n" << (*shares)[0] << ",\n" << (*shares)[1] << ",\n" << (*shares)[2] << std::endl;
                    // xy-r
                    (*shares)[0] += (*shares)[1] + (*shares)[2];

                    // xy/2^d = (r/2^d) + ((xy-r) / 2^d)
                    auto& v = C.mShares[mPartyIdx];
                    auto& s = (*shares)[0];
                    for (u64 i = 0; i < v.size(); ++i)
                        v(i) += s(i) >> shift;


                    //lout <<"p" << mPartyIdx << "\n" << v <<",\n" 						<<  C.mShares[1 ^ mPartyIdx] << std::endl;

                });
            }
            //else
            //{
            //	lout << "p " << mPartyIdx << "\n" << C.mShares[0] << ",\n" << C.mShares[1] << std::endl;
            //}
            }).getClosure();
    }

}

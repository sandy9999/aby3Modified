#include "AstraSh3Evaluator.h"
#include <cryptoTools/Crypto/PRNG.h>
#include <iomanip>
#include <cryptoTools/Common/Log.h>
using namespace oc;
namespace aby3
{
    void AstraSh3Evaluator::init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize)
    {
        mShareGen.init(prevSeed, nextSeed, buffSize);
        mPartyIdx = partyIdx;
    }

    void AstraSh3Evaluator::init(u64 partyIdx, CommPkg& comm, block seed, u64 buffSize)
    {
        mShareGen.init(comm, seed, buffSize);
        mPartyIdx = partyIdx;
    }
    
    Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_distributor(Sh3Task dep, const si64Matrix& left, const si64Matrix& right, si64Matrix& product_share)
    {
        return dep.then([this, &left, &right, &product_share](CommPkg& comm, Sh3Task& self) {

            for(u64 i = 0; i<product_share.size(); ++i)
            {
              product_share[0](i) = mShareGen.getShare(0,1,0);
              product_share[1](i) = mShareGen.getShare(1,0,0);
            }
            i64Matrix alpha_products_sum(left.rows(), right.cols());
            alpha_products_sum = (left[0] + left[1])*(right[0] + right[1]);
            i64Matrix alpha_left_alpha_right_share_1, alpha_left_alpha_right_share_2;
            alpha_left_alpha_right_share_1.resizeLike(alpha_products_sum);
            alpha_left_alpha_right_share_2.resizeLike(alpha_products_sum);
            for(u64 i = 0; i<alpha_left_alpha_right_share_1.size(); ++i)
            alpha_left_alpha_right_share_1(i) = mShareGen.getShare(0, 1, 0);
            alpha_left_alpha_right_share_2 = alpha_products_sum - alpha_left_alpha_right_share_1; 
            comm.mNext.asyncSendCopy(product_share[0].data(), product_share[0].size());
            comm.mPrev.asyncSendCopy(product_share[1].data(), product_share[1].size());
            comm.mNext.asyncSendCopy(alpha_left_alpha_right_share_1.data(), alpha_left_alpha_right_share_1.size());
            comm.mPrev.asyncSendCopy(alpha_left_alpha_right_share_2.data(), alpha_left_alpha_right_share_2.size());

        }).getClosure();
    }

    Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_evaluator(Sh3Task dep, i64Matrix& product_alpha_share, i64Matrix& alpha_left_alpha_right_share)
    {
      return dep.then([this, &product_alpha_share, &alpha_left_alpha_right_share](CommPkg& comm, Sh3Task& self) {
          self.mRuntime->mPartyIdx == 1 ? comm.mPrev.recv(product_alpha_share.data(), product_alpha_share.size()) : comm.mNext.recv(product_alpha_share.data(), product_alpha_share.size());
          self.mRuntime->mPartyIdx == 1 ? comm.mPrev.recv(alpha_left_alpha_right_share.data(), alpha_left_alpha_right_share.size()) : comm.mNext.recv(alpha_left_alpha_right_share.data(), alpha_left_alpha_right_share.size());
          }).getClosure();
    }

    Sh3Task AstraSh3Evaluator::astra_asyncMul_online(Sh3Task dep, const si64Matrix& left , const si64Matrix& right, si64Matrix& product_share, i64Matrix& alpha_left_alpha_right_share, i64Matrix& product_alpha_share)
    {
        return dep.then([this, &left, &right, &product_share, &alpha_left_alpha_right_share, &product_alpha_share](CommPkg& comm, Sh3Task& self) {
            product_share[0] = product_alpha_share;
            product_share[1] = - (left[1]*right[0]) - (left[0]*right[1]) + product_alpha_share + alpha_left_alpha_right_share;
            if(self.mRuntime->mPartyIdx == 1)
              product_share[1]+=(left[1]*right[1]);

            i64Matrix other_product_beta_share;
            other_product_beta_share.resizeLike(product_alpha_share);
            if(self.mRuntime->mPartyIdx == 1)
            {
              comm.mNext.asyncSendCopy(product_share[1].data(), product_share[1].size()); 
            }
            else if(self.mRuntime->mPartyIdx == 2)
            {
              comm.mPrev.asyncSendCopy(product_share[1].data(), product_share[1].size());
            }
            self.mRuntime->mPartyIdx == 1 ? comm.mNext.recv(other_product_beta_share.data(), other_product_beta_share.size()) : comm.mPrev.recv(other_product_beta_share.data(), other_product_beta_share.size());
            product_share[1]+=other_product_beta_share;

            }).getClosure();
    }

}

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

    Sh3Task AstraSh3Evaluator::astra_asyncMulTruncation_preprocess_distributor(Sh3Task dep, const si64Matrix& left, const si64Matrix& right, si64Matrix& u_shares, Decimal D)
    {
        return dep.then([this, &left, &right, &u_shares, D](CommPkg& comm, Sh3Task& self) {
            
            i64Matrix m1, m2;
            m1.resizeLike(u_shares[0]);
            m2.resizeLike(u_shares[0]);

            for(u64 i = 0; i<m1.size(); ++i)
            {
              m1(i) = mShareGen.getShare(0,1,0);
              m2(i) = mShareGen.getShare(1,0,0);
            }
            
            i64Matrix alpha_z;
            alpha_z = m1 + m2 - (left[0] + left[1])*(right[0] + right[1]);

            //Local truncation
            for(u64 i = 0; i < alpha_z.size(); ++i)
              alpha_z(i)>>=D;
            
            for(u64 i = 0; i<u_shares[0].size(); ++i)
                u_shares[0](i) = mShareGen.getShare(0, 1, 0);
            for(u64 i = 0; i<u_shares[1].size(); ++i)
                u_shares[1](i) = mShareGen.getShare(1, 0, 0);
            i64Matrix beta_u;
            beta_u = alpha_z + u_shares[0] + u_shares[1];

            comm.mNext.asyncSendCopy(beta_u.data(), beta_u.size());
            comm.mPrev.asyncSendCopy(beta_u.data(), beta_u.size());

        }).getClosure();
    }
    
    Sh3Task AstraSh3Evaluator::astra_asyncMulTruncation_preprocess_evaluator(Sh3Task dep, si64Matrix& u_shares, i64Matrix& m_share, Decimal D)
    {
      return dep.then([this, &u_shares, &m_share, D](CommPkg& comm, Sh3Task& self) {
          if(self.mRuntime->mPartyIdx == 1)
          {
            for(u64 i = 0; i<m_share.size(); ++i)
              m_share(i) = mShareGen.getShare(1,0,0);
            for(u64 i = 0; i<u_shares[0].size(); ++i)
              u_shares[0](i) = mShareGen.getShare(1, 0, 0);
            auto fu = comm.mPrev.asyncRecv(u_shares[1].data(), u_shares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
            });
          }
          else
          {

            for(u64 i = 0; i<m_share.size(); ++i)
              m_share(i) = mShareGen.getShare(0,1,0);

            for(u64 i = 0; i<u_shares[0].size(); ++i)
              u_shares[0](i) = mShareGen.getShare(0, 1, 0);
          
            auto fu = comm.mNext.asyncRecv(u_shares[1].data(), u_shares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
          }
          
        }).getClosure();
    }
    
    Sh3Task AstraSh3Evaluator::astra_asyncMulTruncation_online(Sh3Task dep, const si64Matrix& left , const si64Matrix& right, si64Matrix& u_shares, i64Matrix& m_share, si64Matrix& v_shares, Decimal D)
    {
        return dep.then([this, &left, &right, &u_shares, &m_share, &v_shares, D](CommPkg& comm, Sh3Task& self) {

            v_shares[1] = - (left[1]*right[0]) - (left[0]*right[1]) + m_share;
            i64Matrix other_beta_v_share;
            other_beta_v_share.resizeLike(v_shares[1]);
            if(self.mRuntime->mPartyIdx == 1)
            {
              v_shares[1]+=(left[1]*right[1]);
              comm.mNext.asyncSendCopy(v_shares[1].data(), v_shares[1].size()); 
            }
            else if(self.mRuntime->mPartyIdx == 2)
            {
              comm.mPrev.asyncSendCopy(v_shares[1].data(), v_shares[1].size());
            }
            self.mRuntime->mPartyIdx == 1 ? comm.mNext.recv(other_beta_v_share.data(), other_beta_v_share.size()) : comm.mPrev.recv(other_beta_v_share.data(), other_beta_v_share.size());
            v_shares[1]+=other_beta_v_share;
            
            for(u64 i = 0; i < v_shares[1].size(); ++i)
              v_shares[1](i)>>=D;

            }).getClosure();
    }
    
    Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_distributor(Sh3Task dep, const si64Matrix& left, const si64Matrix& right, si64Matrix& product_share)
    {
        return dep.then([this, &left, &right, &product_share](CommPkg& comm, Sh3Task& self) {

            for(u64 i = 0; i<product_share.size(); ++i)
            {
              product_share[0](i) = mShareGen.getShare(0,1,0);
              product_share[1](i) = mShareGen.getShare(1,0,0);
            }
            i64Matrix alpha_products_sum, alpha_left_alpha_right_share_1, alpha_left_alpha_right_share_2;
            alpha_products_sum = (left[0] + left[1])*(right[0] + right[1]);
            alpha_left_alpha_right_share_1.resizeLike(alpha_products_sum);
            for(u64 i = 0; i<alpha_left_alpha_right_share_1.size(); ++i)
            alpha_left_alpha_right_share_1(i) = mShareGen.getShare(0, 1, 0);
            alpha_left_alpha_right_share_2 = alpha_products_sum - alpha_left_alpha_right_share_1; 
            comm.mPrev.asyncSendCopy(alpha_left_alpha_right_share_2.data(), alpha_left_alpha_right_share_2.size());

        }).getClosure();
    }

    Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_evaluator(Sh3Task dep, i64Matrix& product_alpha_share, i64Matrix& alpha_left_alpha_right_share)
    {
      return dep.then([this, &product_alpha_share, &alpha_left_alpha_right_share](CommPkg& comm, Sh3Task& self) {
          if(self.mRuntime->mPartyIdx == 1)
          {
            for(u64 i = 0; i<product_alpha_share.size(); ++i)
              product_alpha_share(i) = mShareGen.getShare(1,0,0);
            for(u64 i = 0; i<alpha_left_alpha_right_share.size(); ++i)
              alpha_left_alpha_right_share(i) = mShareGen.getShare(1, 0, 0);
          }
          else
          {
            for(u64 i = 0; i<product_alpha_share.size(); ++i)
              product_alpha_share(i) = mShareGen.getShare(0,1,0);
            auto fu = comm.mNext.asyncRecv(alpha_left_alpha_right_share.data(), alpha_left_alpha_right_share.size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
          }
        }).getClosure();
    }

    Sh3Task AstraSh3Evaluator::astra_asyncMul_online(Sh3Task dep, const si64Matrix& left , const si64Matrix& right, si64Matrix& product_share, i64Matrix& alpha_left_alpha_right_share, i64Matrix& product_alpha_share)
    {
        return dep.then([this, &left, &right, &product_share, &alpha_left_alpha_right_share, &product_alpha_share](CommPkg& comm, Sh3Task& self) {
            product_share[0] = product_alpha_share;
            product_share[1] = - (left[1]*right[0]) - (left[0]*right[1]) + product_alpha_share + alpha_left_alpha_right_share;
            i64Matrix other_product_beta_share;
            other_product_beta_share.resizeLike(product_alpha_share);
            if(self.mRuntime->mPartyIdx == 1)
            {
              product_share[1]+=(left[1]*right[1]);
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

    Sh3Task AstraSh3Evaluator::astra_bit_injection_preprocess_distributor(Sh3Task dep, si64Matrix& c, si64Matrix& x)
    {
        return dep.then([this, &c, &x](CommPkg& comm, Sh3Task& self) {
            
            i64Matrix alpha_c_1, alpha_c_2, alpha_c;
            alpha_c_1.resizeLike(c[0]);
            alpha_c_2.resizeLike(c[0]);
            alpha_c.resizeLike(c[0]);
            
            alpha_c = (c.mShares[0] + c.mShares[1]);
            for(u64 i = 0; i<alpha_c_1.size(); ++i)
              alpha_c_1(i) = mShareGen.getShare(0, 1, 0);
            
            alpha_c_2 = alpha_c - alpha_c_1; 
            
            i64Matrix alpha_products_sum, alpha_left_alpha_right_share_1, alpha_left_alpha_right_share_2;
            alpha_products_sum.resizeLike(c[0]);
            for(u64 i = 0; i<alpha_products_sum.size(); ++i)
              alpha_products_sum(i) = (c[0](i) + c[1](i))*(x[0](i) + x[1](i));
            alpha_left_alpha_right_share_1.resizeLike(c[0]);
            alpha_left_alpha_right_share_2.resizeLike(c[0]);
            for(u64 i = 0; i<alpha_left_alpha_right_share_1.size(); ++i)
            alpha_left_alpha_right_share_1(i) = mShareGen.getShare(0, 1, 0);
            alpha_left_alpha_right_share_2 = alpha_products_sum - alpha_left_alpha_right_share_1;

            comm.mPrev.asyncSendCopy(alpha_c_2.data(), alpha_c_2.size());
            comm.mPrev.asyncSendCopy(alpha_left_alpha_right_share_2.data(), alpha_left_alpha_right_share_2.size());

        }).getClosure();
    }
    
    Sh3Task AstraSh3Evaluator::astra_bit_injection_preprocess_evaluator(Sh3Task dep, i64Matrix& alpha_c_share, i64Matrix& alpha_left_alpha_right_share)
    {
      return dep.then([this, &alpha_c_share, &alpha_left_alpha_right_share](CommPkg& comm, Sh3Task& self) {
      if(self.mRuntime->mPartyIdx == 1)
      {
            for(u64 i = 0; i<alpha_c_share.size(); ++i)
              alpha_c_share(i) = mShareGen.getShare(1, 0, 0);

            for(u64 i = 0; i<alpha_left_alpha_right_share.size(); ++i)
              alpha_left_alpha_right_share(i) = mShareGen.getShare(1, 0, 0);
      }
      else
      {
          comm.mNext.recv(alpha_c_share.data(), alpha_c_share.size());
          comm.mNext.recv(alpha_left_alpha_right_share.data(), alpha_left_alpha_right_share.size());
      }

          }).getClosure();
    }
    
    Sh3Task AstraSh3Evaluator::astra_bit_injection_online(Sh3Task dep, si64Matrix& c , si64Matrix& x, i64Matrix& alpha_c_share, i64Matrix& alpha_left_alpha_right_share, si64Matrix& final_share)
    {
        return dep.then([this, &c, &x, &alpha_c_share, &alpha_left_alpha_right_share, &final_share](CommPkg& comm, Sh3Task& self) {
            i64Matrix cx_share, beta_c_beta_x(c.rows(), c.cols()), beta_c_alpha_x_share(c.rows(), c.cols()), alpha_c_beta_x_share(c.rows(), c.cols());

            for (u64 i = 0; i<beta_c_beta_x.size(); ++i)
            {
              beta_c_beta_x(i) = c.mShares[1](i)*x[1](i);
              beta_c_alpha_x_share(i) = c.mShares[1](i)*x[0](i);
              alpha_c_beta_x_share(i) = alpha_c_share(i)*x[1](i);
            }
            cx_share = - beta_c_alpha_x_share - alpha_c_beta_x_share + alpha_left_alpha_right_share;

            if(self.mRuntime->mPartyIdx == 1)
            {
                cx_share+=beta_c_beta_x;
                i64Matrix alpha_cx_share_1, alpha_cx_share_2;
                alpha_cx_share_1.resizeLike(cx_share);
                alpha_cx_share_2.resizeLike(cx_share);
                for (u64 i = 0; i<alpha_cx_share_1.size(); ++i)
                  alpha_cx_share_1(i) = mShareGen.getShare(1, 0, 0);
                
                for(u64 i = 0; i<final_share.size(); ++i)
                  final_share[0](i) = mShareGen.getShare(0, 0, 1);

                for (u64 i = 0; i<alpha_cx_share_2.size(); ++i)
                  alpha_cx_share_2(i) = mShareGen.getShare(0, 0, 1);

                final_share[0]+=alpha_cx_share_1;
                final_share[1] = alpha_cx_share_1 + alpha_cx_share_2 + cx_share;
                i64Matrix other_beta_cx_share;
                other_beta_cx_share.resizeLike(alpha_cx_share_1);
                comm.mNext.asyncSendCopy(final_share[1].data(), final_share[1].size());
                comm.mNext.recv(other_beta_cx_share.data(), other_beta_cx_share.size());
                final_share[1]+=other_beta_cx_share;
            }
            else
            {
                i64Matrix alpha_cx_share_1, alpha_cx_share_2;
                alpha_cx_share_1.resizeLike(cx_share);
                alpha_cx_share_2.resizeLike(cx_share);
                for (u64 i = 0; i<alpha_cx_share_1.size(); ++i)
                  alpha_cx_share_1(i) = mShareGen.getShare(0, 1, 0);
                
                for (u64 i = 0; i<alpha_cx_share_2.size(); ++i)
                  alpha_cx_share_2(i) = mShareGen.getShare(0, 0, 1);

                for(u64 i = 0; i<final_share.size(); ++i)
                  final_share[0](i) = mShareGen.getShare(0, 0, 1);

                final_share[0]+=alpha_cx_share_1;
                final_share[1] = alpha_cx_share_1 + alpha_cx_share_2 + cx_share;               

                i64Matrix other_beta_cx_share;
                other_beta_cx_share.resizeLike(alpha_cx_share_1);
                comm.mPrev.asyncSendCopy(final_share[1].data(), final_share[1].size());
                comm.mPrev.recv(other_beta_cx_share.data(), other_beta_cx_share.size());
                final_share[1]+=other_beta_cx_share;
            }

            }).getClosure();
    }

    Sh3Task AstraSh3Evaluator::astra_bit_injection_online_evaluator(Sh3Task dep, si64Matrix& final_share)
    {
        return dep.then([this, &final_share](CommPkg& comm, Sh3Task& self) {
            si64Matrix cx_share_1(final_share.rows(), final_share.cols()), cx_share_2(final_share.rows(), final_share.cols());

            for(u64 i = 0; i < cx_share_1.size(); ++i)
            {
              cx_share_1[0](i) = mShareGen.getShare(0, 1, 0);
              cx_share_2[0](i) = mShareGen.getShare(0, 0, 1);
            }
            for(u64 i = 0; i < cx_share_1.size(); ++i)
            {
              cx_share_2[1](i) = mShareGen.getShare(1, 0, 0);
              cx_share_1[1](i) = mShareGen.getShare(0, 0, 1);
            }

            final_share[0] = cx_share_1[0] + cx_share_2[0];
            final_share[1] = cx_share_1[1] + cx_share_2[1];
          }).getClosure();
    }
}

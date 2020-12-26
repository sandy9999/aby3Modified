#include "AstraSh3Encryptor.h"
#include <libOTe/Tools/Tools.h>

#include "cryptoTools/Common/Log.h"

namespace aby3
{

	Sh3Task AstraSh3Encryptor::astra_share_preprocess_distributor(Sh3Task dep, si64& alpha_X)
	{

		return dep.then([this, &alpha_X](CommPkg& comm, Sh3Task& self) {
        alpha_X[0] = mShareGen.getShare(0,1,0);
        alpha_X[1] = mShareGen.getShare(1,0,0);

				comm.mNext.asyncSendCopy(alpha_X[0]);
				comm.mPrev.asyncSendCopy(alpha_X[1]);

		}).getClosure();

	}

	Sh3Task AstraSh3Encryptor::astra_share_preprocess_evaluator(Sh3Task dep, i64& alpha_X_share)
	{
		return dep.then([this, &alpha_X_share](CommPkg& comm, Sh3Task& self) {
          if(self.mRuntime->mPartyIdx == 1)
          {
            alpha_X_share = mShareGen.getShare(1, 0, 0);
          }
          else
          {
            alpha_X_share = mShareGen.getShare(0, 1, 0);
          }
				}).getClosure();
	}

	Sh3Task AstraSh3Encryptor::astra_share_online_distributor(Sh3Task dep, i64& X, si64& alpha_X)
	{
		return dep.then([this, &X, &alpha_X](CommPkg& comm, Sh3Task& self) {
        i64 beta_X;
        beta_X = X + alpha_X[0] + alpha_X[1];
				comm.mNext.asyncSendCopy(beta_X);
				comm.mPrev.asyncSendCopy(beta_X);
		}).getClosure();
	}

	Sh3Task AstraSh3Encryptor::astra_share_online_evaluator(Sh3Task dep, i64 & beta_X, u64& partyIdx)
	{
		return dep.then([this, &beta_X, &partyIdx](CommPkg& comm, Sh3Task& self) {

        auto fu = (partyIdx == ((self.mRuntime->mPartyIdx + 2)%3)) ? comm.mPrev.asyncRecv(beta_X) : comm.mNext.asyncRecv(beta_X);
				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
				}).getClosure();
	}

	Sh3Task AstraSh3Encryptor::astra_share_matrix_preprocess_distributor(Sh3Task dep, si64Matrix & alpha_X)
	{

		return dep.then([this, &alpha_X](CommPkg& comm, Sh3Task& self) {
				for (u64 i = 0; i < alpha_X.mShares[0].size(); ++i)
        {
				  alpha_X.mShares[0](i) = mShareGen.getShare(0, 1, 0);
          alpha_X.mShares[1](i) = mShareGen.getShare(1, 0, 0);
        }

		}).getClosure();

	}

	Sh3Task AstraSh3Encryptor::astra_share_matrix_preprocess_evaluator(Sh3Task dep, i64Matrix & alpha_X_share)
	{
		return dep.then([this, &alpha_X_share](CommPkg& comm, Sh3Task& self) {
        if(self.mRuntime->mPartyIdx == 1)
        {
          for (u64 i = 0; i < alpha_X_share.size(); ++i)
          {
            alpha_X_share(i) = mShareGen.getShare(1, 0, 0);
          }
        }
        else
        {
          for (u64 i = 0; i < alpha_X_share.size(); ++i)
          {
            alpha_X_share(i) = mShareGen.getShare(0, 1, 0);
          }
        }

				}).getClosure();
	}

	Sh3Task AstraSh3Encryptor::astra_share_matrix_online_distributor(Sh3Task dep, i64Matrix& X, si64Matrix& alpha_X)
	{

		return dep.then([this, &X, &alpha_X](CommPkg& comm, Sh3Task& self) {
				auto b0 = alpha_X.cols() != static_cast<u64>(X.cols());
				auto b1 = alpha_X.size() != static_cast<u64>(X.size());
				if (b0 || b1)
    				throw std::runtime_error(LOCATION);

        i64Matrix beta_X(X.rows(), X.cols());

				for (u64 i = 0; i < alpha_X.size(); ++i)
        {
				  beta_X(i) = X(i) + alpha_X(i)[0] + alpha_X(i)[1];
        }
				comm.mNext.asyncSendCopy(beta_X.data(), beta_X.size());
				comm.mPrev.asyncSendCopy(beta_X.data(), beta_X.size());
		}).getClosure();

	}

	Sh3Task AstraSh3Encryptor::astra_share_matrix_online_evaluator(Sh3Task dep, i64Matrix & beta_X, u64& partyIdx)
	{
		return dep.then([this, &beta_X, &partyIdx](CommPkg& comm, Sh3Task& self) {

        auto fu = (partyIdx == ((self.mRuntime->mPartyIdx + 2)%3)) ? comm.mPrev.asyncRecv(beta_X.data(), beta_X.size()) : comm.mNext.asyncRecv(beta_X.data(), beta_X.size());
				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
				}).getClosure();
	}

  Sh3Task AstraSh3Encryptor::astra_revealAll(Sh3Task dep, const si64Matrix& shared_X, i64Matrix& X)
  {
      astra_reveal(dep, 0, shared_X).get();
      return astra_reveal(dep, shared_X, X);
  }

	Sh3Task AstraSh3Encryptor::astra_reveal(Sh3Task dep, const si64Matrix& shared_X, i64Matrix& X)
	{
		return dep.then([&shared_X, &X](CommPkg& comm, Sh3Task& self) {

        i64Matrix recv_val;
        recv_val.resizeLike(shared_X[0]);
        comm.mNext.recv(recv_val.data(), recv_val.size());
        if(self.mRuntime->mPartyIdx == 0)
          X = recv_val - (shared_X[0] + shared_X[1]);
        else
          X = shared_X[1] - shared_X[0] - recv_val;
				}).getClosure();
	}

	Sh3Task AstraSh3Encryptor::astra_reveal(Sh3Task dep, u64 partyIdx, const si64Matrix& shared_X)
	{
		return dep.then([&partyIdx, &shared_X](CommPkg& comm, Sh3Task& self) {
        if(partyIdx == ((self.mRuntime->mPartyIdx + 2)%3))
          comm.mPrev.asyncSendCopy(shared_X[1].data(), shared_X[1].size());
        else
          comm.mPrev.asyncSendCopy(shared_X[0].data(), shared_X[0].size());

				}).getClosure();
	}
}


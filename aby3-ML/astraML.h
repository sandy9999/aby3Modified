#pragma once
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/Channel.h>
#include <aby3/Common/Defines.h>
#include <aby3/sh3/Sh3FixedPoint.h>
#include <aby3/sh3/AstraSh3Encryptor.h>
#include <aby3/sh3/AstraSh3Evaluator.h>
//#include <aby3/sh3/Sh3Piecewise.h>

namespace aby3
{
	class astraML
	{
	public:
		oc::Channel mPreproNext, mPreproPrev, mNext, mPrev;
		AstraSh3Encryptor mEnc;
		AstraSh3Evaluator mEval;
		Sh3Runtime mRt;
		bool mPrint = true;

		u64 partyIdx()
		{
			return mRt.mPartyIdx;
		}

		void init(u64 partyIdx, oc::Session& prev, oc::Session& next, oc::block seed);

		template<Decimal D>
		void astra_share_online_distributor(f64<D>& X, sf64<D>& alpha_X)
		{
			mEnc.astra_fixed_share_online_distributor(mRt.noDependencies(), X, alpha_X).get();
		}

		template<Decimal D>
		f64<D> astra_share_online_evaluator(u64 partyIdx)
		{
			f64<D> beta_X;
			mEnc.astra_fixed_share_online_evaluator(mRt.noDependencies(), beta_X, partyIdx).get();
			return beta_X;
		}

		template<Decimal D>
		sf64<D> astra_share_preprocess_distributor(const f64<D>& X)
		{
			sf64<D> alpha_X;
			mEnc.astra_fixed_share_preprocess_distributor(mRt.noDependencies(), alpha_X).get();
			return alpha_X;
		}

		template<Decimal D>
		f64<D> astra_share_preprocess_evaluator(u64 partyIdx)
		{
			f64<D> alpha_X_share;
			mEnc.astra_fixed_share_preprocess_evaluator(mRt.noDependencies(), alpha_X_share).get();
			return alpha_X_share;
		}
    

		template<Decimal D>
		void astra_share_matrix_online_distributor(f64Matrix<D>& X, sf64Matrix<D>& alpha_X)
		{
			std::array<u64, 2> size{ X.rows(), X.cols() };
			mNext.asyncSendCopy(size);
			mPrev.asyncSendCopy(size);
			mEnc.astra_fixed_share_matrix_online_distributor(mRt.noDependencies(), X, alpha_X).get();
		}

		template<Decimal D>
		f64Matrix<D> astra_share_matrix_online_evaluator(u64 partyIdx)
		{
			std::array<u64, 2> size;
			if (partyIdx == (mRt.mPartyIdx + 1) % 3)
				mNext.recv(size);
			else if (partyIdx == (mRt.mPartyIdx + 2) % 3)
				mPrev.recv(size);
			else
				throw RTE_LOC;

			f64Matrix<D> beta_X(size[0], size[1]);
			mEnc.astra_fixed_share_matrix_online_evaluator(mRt.noDependencies(), beta_X, partyIdx).get();
			return beta_X;
		}

		template<Decimal D>
		sf64Matrix<D> astra_share_matrix_preprocess_distributor(const f64Matrix<D>& X)
		{
			std::array<u64, 2> size{ X.rows(), X.cols() };
			mNext.asyncSendCopy(size);
			mPrev.asyncSendCopy(size);
			sf64Matrix<D> alpha_X(size[0],size[1]);
			mEnc.astra_fixed_share_matrix_preprocess_distributor(mRt.noDependencies(), alpha_X).get();
			return alpha_X;
		}

		template<Decimal D>
		f64Matrix<D> astra_share_matrix_preprocess_evaluator(u64 partyIdx)
		{
			std::array<u64, 2> size;
			if (partyIdx == (mRt.mPartyIdx + 1) % 3)
				mNext.recv(size);
			else if (partyIdx == (mRt.mPartyIdx + 2) % 3)
				mPrev.recv(size);
			else
				throw RTE_LOC;

			f64Matrix<D> alpha_X_share(size[0], size[1]);
			mEnc.astra_fixed_share_matrix_preprocess_evaluator(mRt.noDependencies(), alpha_X_share).get();
			return alpha_X_share;
		}

		template<Decimal D>
		sf64Matrix<D> astra_share_matrix_preprocess_distributor(const eMatrix<double>& vals)
		{
			f64Matrix<D> v2(vals.rows(), vals.cols());
			for (u64 i = 0; i < vals.size(); ++i)
				v2(i) = vals(i);

			return astra_share_matrix_preprocess_distributor(v2);
		}

		template<Decimal D>
		sf64<D> astra_share_preprocess_distributor(double& vals)
		{
			f64<D> v2 = vals;
			return astra_share_preprocess_distributor(v2);
		}
    
		template<Decimal D>
		void astra_share_matrix_online_distributor(const eMatrix<double>& vals, sf64Matrix<D>& alpha_X)
		{
			f64Matrix<D> X(vals.rows(), vals.cols());
			for (u64 i = 0; i < vals.size(); ++i)
				X(i) = vals(i);
      
			astra_share_matrix_online_distributor(X, alpha_X);
		}

		template<Decimal D>
		void astra_share_online_distributor(double& vals, sf64<D>& alpha_X)
		{
			f64<D> X = vals;
			astra_share_online_distributor(X, alpha_X);
		}

		template<Decimal D>
		eMatrix<double> reveal(const sf64Matrix<D>& shared_X)
		{
			f64Matrix<D> X (shared_X[0].rows(), shared_X[0].cols());
			mEnc.astra_revealAll(mRt.noDependencies(), shared_X, X).get();
      eMatrix<double> vals(X.rows(), X.cols());
			for (u64 i = 0; i < vals.size(); ++i)
				vals(i) = static_cast<double>(X(i));
			return vals;
		}

		template<Decimal D>
		sf64Matrix<D> add(const sf64Matrix<D>& left, const sf64Matrix<D>& right)
		{
			if (left.rows() != right.rows() || left.cols() != right.cols())
				throw RTE_LOC;

			sf64Matrix<D> sum(left.rows(), left.cols());
      sum = left + right;
			return sum;
		}

		template<Decimal D>
		sf64Matrix<D> mul(const sf64Matrix<D>& left, const sf64Matrix<D>& right)
		{
			sf64Matrix<D> product_share(left.rows(), right.cols());
			if (mRt.mPartyIdx == 0)
      {
          mEval.astra_asyncMul_preprocess_distributor(mRt.noDependencies(), left, right, product_share).get();
      }
      else
      {
          f64Matrix<D> product_alpha_share(left.rows(), right.cols()), alpha_left_alpha_right_share(left.rows(), right.cols());
          mEval.astra_asyncMul_preprocess_evaluator(mRt.noDependencies(), product_alpha_share, alpha_left_alpha_right_share).get();
    			mEval.astra_asyncMul_online(mRt.noDependencies(), left, right, product_share, alpha_left_alpha_right_share, product_alpha_share).get();
      }
      for(u64 i = 0; i<product_share[0].size(); ++i)
      {
        product_share[0](i)>>=D;
        product_share[1](i)>>=D;
      }
			return product_share;
		}

	};

}

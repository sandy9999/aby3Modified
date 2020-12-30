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
			mEnc.astra_share_online_distributor(mRt.noDependencies(), X, alpha_X).get();
		}

		template<Decimal D>
		f64<D> astra_share_online_evaluator(u64 partyIdx)
		{
			f64<D> beta_X;
			mEnc.astra_share_online_evaluator(mRt.noDependencies(), beta_X, partyIdx).get();
			return beta_X;
		}

		template<Decimal D>
		sf64<D> astra_share_preprocess_distributor(const f64<D>& X)
		{
			sf64<D> alpha_X;
			mEnc.astra_share_preprocess_distributor(mRt.noDependencies(), alpha_X).get();
			return alpha_X;
		}

		template<Decimal D>
		f64<D> astra_share_preprocess_evaluator(u64 partyIdx)
		{
			f64<D> alpha_X_share;
			mEnc.astra_share_preprocess_evaluator(mRt.noDependencies(), alpha_X_share).get();
			return alpha_X_share;
		}
    
		template<Decimal D>
		void astra_share_matrix_online_distributor(f64Matrix<D>& X, sf64Matrix<D>& alpha_X)
		{
			std::array<u64, 2> size{ X.rows(), X.cols() };
			mNext.asyncSendCopy(size);
			mPrev.asyncSendCopy(size);
			mEnc.astra_share_matrix_online_distributor(mRt.noDependencies(), X, alpha_X).get();
		}

		/*template<Decimal D>
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
			mEnc.astra_share_matrix_online_evaluator(mRt.noDependencies(), beta_X, partyIdx).get();
			return beta_X;
		}*/

		i64Matrix astra_share_matrix_online_evaluator(u64 partyIdx)
		{
			std::array<u64, 2> size;
			if (partyIdx == (mRt.mPartyIdx + 1) % 3)
				mNext.recv(size);
			else if (partyIdx == (mRt.mPartyIdx + 2) % 3)
				mPrev.recv(size);
			else
				throw RTE_LOC;

			i64Matrix beta_X(size[0], size[1]);
			mEnc.astra_share_matrix_online_evaluator(mRt.noDependencies(), beta_X, partyIdx).get();
			return beta_X;
		}

		template<Decimal D>
		sf64Matrix<D> astra_share_matrix_preprocess_distributor(const f64Matrix<D>& X)
		{
			std::array<u64, 2> size{ X.rows(), X.cols() };
			mNext.asyncSendCopy(size);
			mPrev.asyncSendCopy(size);
			sf64Matrix<D> alpha_X(size[0],size[1]);
			mEnc.astra_share_matrix_preprocess_distributor(mRt.noDependencies(), alpha_X).get();
			return alpha_X;
		}

		/*template<Decimal D>
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
			mEnc.astra_share_matrix_preprocess_evaluator(mRt.noDependencies(), alpha_X_share).get();
			return alpha_X_share;
		}*/

		i64Matrix astra_share_matrix_preprocess_evaluator(u64 partyIdx)
		{
			std::array<u64, 2> size;
			if (partyIdx == (mRt.mPartyIdx + 1) % 3)
				mNext.recv(size);
			else if (partyIdx == (mRt.mPartyIdx + 2) % 3)
				mPrev.recv(size);
			else
				throw RTE_LOC;

			i64Matrix alpha_X_share(size[0], size[1]);
			mEnc.astra_share_matrix_preprocess_evaluator(mRt.noDependencies(), alpha_X_share).get();
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

		i64Matrix reveal(const si64Matrix& shared_X)
		{
			i64Matrix X (shared_X[0].rows(), shared_X[0].cols());
			mEnc.astra_revealAll(mRt.noDependencies(), shared_X, X).get();
			return X;
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
      std::chrono::time_point<std::chrono::system_clock>
        addStop,
        addStart = std::chrono::system_clock::now();
			if (left.rows() != right.rows() || left.cols() != right.cols())
				throw RTE_LOC;

			sf64Matrix<D> sum(left.rows(), left.cols());
      sum = left + right;
      addStop = std::chrono::system_clock::now();
      auto addMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(addStop - addStart).count();
      std::cout<<"Time for Local Addition in microseconds: "<<addMicroSeconds<<std::endl;
			return sum;
		}

		template<Decimal D>
		sf64Matrix<D> subtract(const sf64Matrix<D>& left, const sf64Matrix<D>& right)
		{
      std::chrono::time_point<std::chrono::system_clock>
        subtractStop,
        subtractStart = std::chrono::system_clock::now();
			if (left.rows() != right.rows() || left.cols() != right.cols())
				throw RTE_LOC;

			sf64Matrix<D> diff(left.rows(), left.cols());
      diff = left - right;
      subtractStop = std::chrono::system_clock::now();
      auto subtractMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(subtractStop - subtractStart).count();
      std::cout<<"Time for Local Subtraction in microseconds: "<<subtractMicroSeconds<<std::endl;
			return diff;
		}

		template<Decimal D>
		sf64Matrix<D> mul(const sf64Matrix<D>& left, const sf64Matrix<D>& right)
		{
      std::chrono::time_point<std::chrono::system_clock>
        matrixMultiplicationStop,
        matrixMultiplicationStart = std::chrono::system_clock::now();

      sf64Matrix<D> ans(left.rows(), right.cols());
			if (mRt.mPartyIdx == 0)
      {
        for(u64 i = 0; i<left.rows(); ++i)
        {
			    sf64<D> product_share;
          sf64Matrix<D> currentRow(1, right.rows());
          currentRow.row(0) = left.row(i);
          mEval.astra_asyncMul_preprocess_distributor(mRt.noDependencies(), currentRow, right, product_share).get();
          ans[0](i) = product_share[0];
          ans[1](i) = product_share[1];
        }
      }
      else
      {
          for(u64 i = 0; i<left.rows(); ++i)
          {
            f64<D> product_alpha_share, alpha_left_alpha_right_share;
			      sf64<D> product_share;
            sf64Matrix<D> currentRow(1, right.rows());
            currentRow.row(0) = left.row(i);
            mEval.astra_asyncMul_preprocess_evaluator(mRt.noDependencies(), product_alpha_share, alpha_left_alpha_right_share).get();
            mEval.astra_asyncMul_online(mRt.noDependencies(), currentRow, right, product_share, alpha_left_alpha_right_share, product_alpha_share).get();
            ans[0](i) = product_share[0];
            ans[1](i) = product_share[1];
          }
      }
      for(u64 i = 0; i<ans.size(); ++i)
      {
        ans[0](i)>>=D;
        ans[1](i)>>=D;
      }
      matrixMultiplicationStop = std::chrono::system_clock::now();
      auto matrixMultiplicationMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(matrixMultiplicationStop - matrixMultiplicationStart).count();
      std::cout<<"Time for Matrix Multiplication in microseconds: "<<matrixMultiplicationMicroSeconds<<std::endl;
			return ans;
		}

    template<Decimal D>
    sf64Matrix<D> add_const(const sf64Matrix<D>& shared_X, const f64<D> val)
   {
        std::chrono::time_point<std::chrono::system_clock>
          addConstStop,
          addConstStart = std::chrono::system_clock::now();

        sf64Matrix<D> ans(shared_X[0].rows(), shared_X[0].cols());
        if(mRt.mPartyIdx == 0)
        {
          ans = shared_X;
        }
        else
        {
          ans[0] = shared_X[0];
          for (u64 i = 0; i<shared_X[1].size(); ++i)
            ans[1](i) = shared_X[1](i) + val.mValue;
        }
        addConstStop = std::chrono::system_clock::now();
        auto addConstMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(addConstStop - addConstStart).count();
        std::cout<<"Time for Adding a constant to matrix in microseconds: "<<addConstMicroSeconds<<std::endl;

        return ans;
    }

    si64Matrix bit_extraction(eMatrix<double> val)
    {
        std::chrono::time_point<std::chrono::system_clock>
          bitExtractionStop,
          bitExtractionStart = std::chrono::system_clock::now();

        si64Matrix ret(val.rows(), val.cols());
        for (u64 i = 0; i<val.size(); ++i)
          if(val(i) < 0)
          {
            if (mRt.mPartyIdx == 0)
            {
              ret.mShares[0](i) = 0;
              ret.mShares[1](i) = 0;
            }
            else
            {
              ret.mShares[0](i) = 0;
              ret.mShares[1](i) = 1;
            }

          }
          else
          {
            ret.mShares[0](i) = 0;
            ret.mShares[1](i) = 0;
          }

        bitExtractionStop = std::chrono::system_clock::now();
        auto bitExtractionMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(bitExtractionStop - bitExtractionStart).count();
        std::cout<<"Time for Bit Extraction in microseconds: "<<bitExtractionMicroSeconds<<std::endl;
        return ret;
    }

    template<Decimal D>
    sf64Matrix<D> bit_injection(si64Matrix c, sf64Matrix<D> x)
    {
        std::chrono::time_point<std::chrono::system_clock>
          bitInjectionStop,
          bitInjectionStart = std::chrono::system_clock::now();
        sf64Matrix<D> final_share(x.rows(), x.cols());
        if (mRt.mPartyIdx == 0)
        {
            mEval.astra_bit_injection_preprocess_distributor(mRt.noDependencies(), c, x).get();
            mEval.astra_bit_injection_online_evaluator(mRt.noDependencies(), final_share).get();
        }
        else
        {
            i64Matrix alpha_c_share(x.rows(), x.cols()), alpha_left_alpha_right_share(x.rows(), x.cols());
            mEval.astra_bit_injection_preprocess_evaluator(mRt.noDependencies(), alpha_c_share, alpha_left_alpha_right_share).get();
            mEval.astra_bit_injection_online(mRt.noDependencies(), c, x, alpha_c_share, alpha_left_alpha_right_share, final_share).get();
        }
        bitInjectionStop = std::chrono::system_clock::now();
        auto bitInjectionMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(bitInjectionStop - bitInjectionStart).count();
        std::cout<<"Time for Bit Injection in microseconds: "<<bitInjectionMicroSeconds<<std::endl;
        return final_share;
    }

	};

}

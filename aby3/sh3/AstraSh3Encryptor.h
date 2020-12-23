#pragma once
#include "Sh3Types.h"
#include "AstraSh3ShareGen.h"
#include "Sh3Runtime.h"
#include <cryptoTools/Common/MatrixView.h>
#include "Sh3FixedPoint.h"

namespace aby3
{
    class AstraSh3Encryptor
    {
    public:

        //Sh3Task init(Sh3Task& dep);
        void init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize = 256) { mShareGen.init(prevSeed, nextSeed, buffSize); mPartyIdx = partyIdx; }
        void init(u64 partyIdx, CommPkg& comm, block seed, u64 buffSize = 256) { mShareGen.init(comm, seed, buffSize); mPartyIdx = partyIdx; }
        

	      Sh3Task astra_int_share_preprocess_distributor(Sh3Task dep, si64 & alpha_X);
	      Sh3Task astra_int_share_preprocess_evaluator(Sh3Task dep, i64& alpha_X_share);

	      Sh3Task astra_int_share_online_distributor(Sh3Task dep, i64& X, si64& alpha_X);
      	Sh3Task astra_int_share_online_evaluator(Sh3Task dep, i64& beta_X, u64& partyIdx);


	      Sh3Task astra_int_share_matrix_preprocess_distributor(Sh3Task dep, si64Matrix & alpha_X);
	      Sh3Task astra_int_share_matrix_preprocess_evaluator(Sh3Task dep, i64Matrix & alpha_X_share);

	      Sh3Task astra_int_share_matrix_online_distributor(Sh3Task dep, i64Matrix& X, si64Matrix& alpha_X);
      	Sh3Task astra_int_share_matrix_online_evaluator(Sh3Task dep, i64Matrix & beta_X, u64& partyIdx);

    Sh3Task astra_revealAll(Sh3Task dep, const si64Matrix& shared_X, i64Matrix& X);
  	Sh3Task astra_reveal(Sh3Task dep, const si64Matrix& shared_X, i64Matrix& X);
	  Sh3Task astra_reveal(Sh3Task dep, u64 partyIdx, const si64Matrix& shared_X);

    template<Decimal D>
  	Sh3Task astra_revealAll(Sh3Task dep, const sf64Matrix<D>& shared_X, f64Matrix<D>& X);

    template<Decimal D>
    Sh3Task astra_fixed_share_preprocess_distributor(Sh3Task dep, sf64<D>& alpha_X);

    template<Decimal D>
    Sh3Task astra_fixed_share_preprocess_evaluator(Sh3Task dep, f64<D>& alpha_X_share);

    template<Decimal D>
    Sh3Task astra_fixed_share_online_distributor(Sh3Task dep, f64<D>& X, sf64<D>& alpha_X);

    template<Decimal D>
    Sh3Task astra_fixed_share_online_evaluator(Sh3Task dep, f64<D>& beta_X, u64 partyIdx);

    template<Decimal D>
    Sh3Task astra_fixed_share_matrix_preprocess_distributor(Sh3Task dep, sf64Matrix<D>& alpha_X);

    template<Decimal D>
    Sh3Task astra_fixed_share_matrix_preprocess_evaluator(Sh3Task dep, f64Matrix<D>& alpha_X_share);

    template<Decimal D>
    Sh3Task astra_fixed_share_matrix_online_distributor(Sh3Task dep, f64Matrix<D>& X, sf64Matrix<D>& alpha_X);

    template<Decimal D>
    Sh3Task astra_fixed_share_matrix_online_evaluator(Sh3Task dep, f64Matrix<D>& beta_X, u64 partyIdx);

        u64 mPartyIdx = -1;
        AstraSh3ShareGen mShareGen;
    };

    template<Decimal D>
  	Sh3Task AstraSh3Encryptor::astra_revealAll(Sh3Task dep, const sf64Matrix<D>& shared_X, f64Matrix<D>& X)
    {
        
      static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix), "assumption for this cast.");
      auto& shared_X_cast = (si64Matrix&)shared_X;

      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& X_cast = (i64Matrix&)X;
      return astra_revealAll(dep, shared_X_cast, X_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_preprocess_distributor(Sh3Task dep, sf64<D>& alpha_X)
    {
      static_assert(sizeof(sf64<D>) == sizeof(si64), "assumption for this cast.");
      auto& alpha_X_cast = (si64&)alpha_X;

      return astra_int_share_preprocess_distributor(dep, alpha_X_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_preprocess_evaluator(Sh3Task dep, f64<D>& alpha_X_share)
    {

      static_assert(sizeof(f64<D>) == sizeof(i64), "assumption for this cast.");
      auto& alpha_X_share_cast = (i64&)alpha_X_share;

      return astra_int_share_preprocess_evaluator(dep, alpha_X_share_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_online_distributor(Sh3Task dep, f64<D>& X, sf64<D>& alpha_X)
    {
      static_assert(sizeof(sf64<D>) == sizeof(si64), "assumption for this cast.");
      auto& alpha_X_cast = (si64&)alpha_X;
      
      static_assert(sizeof(f64<D>) == sizeof(i64), "assumption for this cast.");
      auto& X_cast = (i64&)X;

      return astra_int_share_online_distributor(dep, X_cast, alpha_X_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_online_evaluator(Sh3Task dep, f64<D>& beta_X, u64 partyIdx)
    {

      static_assert(sizeof(f64<D>) == sizeof(i64), "assumption for this cast.");
      auto& beta_X_cast = (i64&)beta_X;

      return astra_int_share_online_evaluator(dep, beta_X_cast, partyIdx);
    }


    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_matrix_preprocess_distributor(Sh3Task dep, sf64Matrix<D>& alpha_X)
    {
      static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix), "assumption for this cast.");
      auto& alpha_X_cast = (si64Matrix&)alpha_X;

      return astra_int_share_matrix_preprocess_distributor(dep, alpha_X_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_matrix_preprocess_evaluator(Sh3Task dep, f64Matrix<D>& alpha_X_share)
    {

      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& alpha_X_share_cast = (i64Matrix&)alpha_X_share;

      return astra_int_share_matrix_preprocess_evaluator(dep, alpha_X_share_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_matrix_online_distributor(Sh3Task dep, f64Matrix<D>& X, sf64Matrix<D>& alpha_X)
    {
      static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix), "assumption for this cast.");
      auto& alpha_X_cast = (si64Matrix&)alpha_X;
      
      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& X_cast = (i64Matrix&)X;

      return astra_int_share_matrix_online_distributor(dep, X_cast, alpha_X_cast);
    }

    template<Decimal D>
    Sh3Task AstraSh3Encryptor::astra_fixed_share_matrix_online_evaluator(Sh3Task dep, f64Matrix<D>& beta_X, u64 partyIdx)
    {

      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& beta_X_cast = (i64Matrix&)beta_X;

      return astra_int_share_matrix_online_evaluator(dep, beta_X_cast, partyIdx);
    }
}

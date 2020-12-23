#pragma once
#include "Sh3Types.h"
#include "AstraSh3ShareGen.h"
#include "Sh3Runtime.h"
#include "Sh3FixedPoint.h"
#include "aby3/OT/SharedOT.h"

namespace aby3
{

    class AstraSh3Evaluator
    {
    public:

		void init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize = 256);
		void init(u64 partyIdx, CommPkg& comm, block seed, u64 buffSize = 256);

		bool DEBUG_disable_randomization = false;

    Sh3Task astra_asyncMul_preprocess_distributor(Sh3Task dep, const si64Matrix& left, const si64Matrix& right, si64Matrix& product_share);

    Sh3Task astra_asyncMul_preprocess_evaluator(Sh3Task dep, i64Matrix& product_alpha_share, i64Matrix& alpha_left_alpha_right_share);

    Sh3Task astra_asyncMul_online(Sh3Task dep, const si64Matrix& left , const si64Matrix& right, si64Matrix& product_share, i64Matrix& alpha_left_alpha_right_share, i64Matrix& product_alpha_share);

    template<Decimal D>
		Sh3Task astra_asyncMul_preprocess_distributor(
			Sh3Task dep,
			const sf64Matrix<D>& left,
			const sf64Matrix<D>& right,
			sf64Matrix<D>& product_share);

		template<Decimal D>
		Sh3Task astra_asyncMul_preprocess_evaluator(
			Sh3Task dep,
			const f64Matrix<D>& product_alpha_share,
			const f64Matrix<D>& alpha_left_alpha_right_share);

		template<Decimal D>
		Sh3Task astra_asyncMul_online(
			Sh3Task dep,
			const sf64Matrix<D>& left,
			const sf64Matrix<D>& right,
      sf64Matrix<D>& product_share,
      f64Matrix<D>& alpha_left_alpha_right_share,
      f64Matrix<D>& product_alpha_share);

    u64 mPartyIdx = -1;
    AstraSh3ShareGen mShareGen;

    };
    
    template<Decimal D>
		Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_distributor(
			Sh3Task dep,
			const sf64Matrix<D>& left,
			const sf64Matrix<D>& right,
			sf64Matrix<D>& product_share)
		{

      static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix), "assumption for this cast.");
      auto& left_cast = (si64Matrix&)left;
      auto& right_cast = (si64Matrix&)right;
      auto& product_share_cast = (si64Matrix&)product_share;
			return astra_asyncMul_preprocess_distributor(dep, left_cast, right_cast, product_share_cast);
		}

		template<Decimal D>
		Sh3Task AstraSh3Evaluator::astra_asyncMul_preprocess_evaluator(
			Sh3Task dep,
			const f64Matrix<D>& product_alpha_share,
			const f64Matrix<D>& alpha_left_alpha_right_share)
		{
      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& product_alpha_share_cast = (i64Matrix&)product_alpha_share;
      auto& alpha_left_alpha_right_share_cast = (i64Matrix&)alpha_left_alpha_right_share;
			return astra_asyncMul_preprocess_evaluator(dep, product_alpha_share_cast, alpha_left_alpha_right_share_cast);
		}

		template<Decimal D>
		Sh3Task AstraSh3Evaluator::astra_asyncMul_online(
			Sh3Task dep,
			const sf64Matrix<D>& left,
			const sf64Matrix<D>& right,
      sf64Matrix<D>& product_share,
      f64Matrix<D>& alpha_left_alpha_right_share,
      f64Matrix<D>& product_alpha_share)
		{
      static_assert(sizeof(sf64Matrix<D>) == sizeof(si64Matrix), "assumption for this cast.");
      auto& left_cast = (si64Matrix&)left;
      auto& right_cast = (si64Matrix&)right;
      auto& product_share_cast = (si64Matrix&)product_share;
      static_assert(sizeof(f64Matrix<D>) == sizeof(i64Matrix), "assumption for this cast.");
      auto& product_alpha_share_cast = (i64Matrix&)product_alpha_share;
      auto& alpha_left_alpha_right_share_cast = (i64Matrix&)alpha_left_alpha_right_share;
			return astra_asyncMul_online(dep, left_cast, right_cast, product_share_cast, alpha_left_alpha_right_share_cast, product_alpha_share_cast);
		}


}

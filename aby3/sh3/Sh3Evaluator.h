#pragma once
#include "Sh3Types.h"
#include "Sh3ShareGen.h"
#include "Sh3Runtime.h"
#include "Sh3FixedPoint.h"
#include "aby3/OT/SharedOT.h"

namespace aby3
{

    struct TruncationPair
    {
        // the share that should be added before the value being trucnated is revealed.
        i64Matrix mR;

        // the share that thsould be subtracted after the value has been truncated.
        si64Matrix mRTrunc;
    };


    class Sh3Evaluator
    {
    public:

		void init(u64 partyIdx, block prevSeed, block nextSeed, u64 buffSize = 256);
		void init(u64 partyIdx, CommPkg& comm, block seed, u64 buffSize = 256);
    /*void astra_preprocess_mult_step1_0(CommPkg& comm);
    void astra_binary_preprocess_mult_step1_0(CommPkg& comm);
    i64 astra_preprocess_mult_step1(CommPkg& comm, int partyIdx);
    void astra_preprocess_mult_step2_0(CommPkg& comm, si64 share1, si64 share2);
    i64 astra_preprocess_mult_step2(CommPkg& comm, int partyIdx); 
    void astra_binary_preprocess_mult_step2_0_0(CommPkg& comm, sb64 share1);
    void astra_binary_preprocess_mult_step2_0_1(CommPkg& comm, sb64 share1, sb64 share2);

    void astra_preprocess_mult_matrix_step2_0(CommPkg& comm, si64Matrix share1, si64Matrix share2);
    i64 astra_preprocess_mult_matrix_step2(CommPkg& comm, int partyIdx);
    i64 astra_bit2a_online_mult(CommPkg& comm, si64 share1, si64 share2, i64 extra_term, i64 alpha_prod_share, int partyIdx);
    void astra_online_bit_injection(CommPkg& comm, si64 b_Shares, si64 a_Shares, i64 alpha_b_share, i64 alpha_b_alpha_x_share, int partyIdx);

    i64 astra_online_mult_matrix(CommPkg& comm, si64Matrix share1, si64Matrix share2, i64 extra_term, i64 alpha_prod_share, int partyIdx);
    i64 astra_reveal_mult_matrix(CommPkg& comm, int partyIdx, i64 beta_prod, i64 alpha_prod_share, si64 bias);
    i64 astra_bit2a_reveal(CommPkg& comm, int partyIdx, i64 beta_prod_share, i64 alpha_prod_share, i64 alpha_share, i64 beta_share);*/

		bool DEBUG_disable_randomization = false;

        //void mul(
        //    CommPkg& comm,
        //    const si64Matrix & A,
        //    const si64Matrix & B,
        //    si64Matrix& C);

        //CompletionHandle asyncMul(
        //    CommPkg& comm,
        //    const si64Matrix& A,
        //    const si64Matrix& B,
        //    si64Matrix& C);

		Sh3Task asyncMul(
			Sh3Task dependency,
			const si64& A,
			const si64& B,
			si64& C);

        Sh3Task asyncMul(
            Sh3Task dependency,
            const si64Matrix& A,
            const si64Matrix& B,
            si64Matrix& C);


		Sh3Task asyncMul(
			Sh3Task dependency,
			const si64Matrix& A,
			const si64Matrix& B,
			si64Matrix& C,
			u64 shift);


		Sh3Task asyncMul(
			Sh3Task dependency,
			const si64& A,
			const si64& B,
			si64& C,
			u64 shift);

        template<Decimal D>
		Sh3Task asyncMul(
			Sh3Task dependency,
			const sf64<D>& A,
			const sf64<D>& B,
			sf64<D>& C)
		{
			return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D);
		}

		template<Decimal D>
		Sh3Task asyncMul(
			Sh3Task dependency,
			const sf64Matrix<D>& A,
			const sf64Matrix<D>& B,
			sf64Matrix<D>& C,
			u64 shift)
		{
			return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D + shift);
		}

		template<Decimal D>
		Sh3Task asyncMul(
			Sh3Task dependency,
			const sf64Matrix<D>& A,
			const sf64Matrix<D>& B,
			sf64Matrix<D>& C)
		{
			return asyncMul(dependency, A.i64Cast(), B.i64Cast(), C.i64Cast(), D);
		}


		Sh3Task asyncMul(
			Sh3Task dep,
			const si64Matrix& A,
			const sbMatrix& B,
			si64Matrix& C);

		Sh3Task asyncMul(
			Sh3Task dep,
			const i64& a,
			const sbMatrix& B,
			si64Matrix& C);

        TruncationPair getTruncationTuple(u64 xSize, u64 ySize, u64 d);

        u64 mPartyIdx = -1, mTruncationIdx = 0;
        Sh3ShareGen mShareGen;
		SharedOT mOtPrev, mOtNext;
    };




}

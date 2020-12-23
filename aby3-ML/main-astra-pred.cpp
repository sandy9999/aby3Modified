#include "main-astra-pred.h"
#include "astraML.h"
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Network/IOService.h>

#include "aby3/sh3/Sh3Runtime.h"
#include "aby3/sh3/Sh3Types.h"
#include "aby3/sh3/Sh3FixedPoint.h"

using namespace aby3;
using namespace std;
using namespace oc;

//const Decimal Dec(Decimal::D8);

int astra_linear_reg_inference(int N, int Dim, int pIdx, bool print, CLP& cmd, Session& chlPrev, Session& chlNext, bool logistic)
{
  PRNG prng(toBlock(1));

	eMatrix<double> W(N, Dim), X(Dim, 1);
  eMatrix<double> B(N, 1);

	for (u64 i = 0; i < W.size(); ++i)
	{
		  W(i) = (prng.get<int>() % 10 )/ double(10);
	}
	
  for (u64 i = 0; i < X.size(); ++i)
	{
		  X(i) = (prng.get<int>() % 10 )/ double(10);
	}
  
  std::cout<<"Actual product of matrices: "<<W*X<<"\n";

  for (u64 i = 0; i < B.size(); ++i)
	{
		  B(i) = (prng.get<int>() % 10 )/ double(10);
	}

  std::cout<<"W: "<<W<<std::endl;
  std::cout<<"X: "<<X<<std::endl;
  std::cout<<"B: "<<B<<std::endl;
  const Decimal D = D16;
  astraML p;

  p.init(pIdx, chlPrev, chlNext, toBlock(pIdx));
  
  sf64Matrix<D> shared_W(N, Dim), shared_X(Dim, 1), shared_B(N, 1);

	if (pIdx == 0)
	{
		shared_W  = p.astra_share_matrix_preprocess_distributor<D>(W);
		shared_X = p.astra_share_matrix_preprocess_distributor<D>(X);
    shared_B = p.astra_share_matrix_preprocess_distributor<D>(B);

    p.astra_share_matrix_online_distributor<D>(W, shared_W);
    p.astra_share_matrix_online_distributor<D>(X, shared_X);
    p.astra_share_matrix_online_distributor<D>(B, shared_B);

	}
	else
	{
    f64Matrix<D> alpha_X_share(Dim, 1), alpha_W_share(N, Dim), beta_X(Dim, 1), beta_W(N, Dim), alpha_B_share(N, 1), beta_B(N, 1);

		alpha_W_share  = p.astra_share_matrix_preprocess_evaluator<D>(0);
		alpha_X_share = p.astra_share_matrix_preprocess_evaluator<D>(0);
    alpha_B_share = p.astra_share_matrix_preprocess_evaluator<D>(0);

    beta_W = p.astra_share_matrix_online_evaluator<D>(0);
    beta_X = p.astra_share_matrix_online_evaluator<D>(0);
    beta_B = p.astra_share_matrix_online_evaluator<D>(0);

    shared_W[0] = (i64Matrix&)alpha_W_share;
    shared_W[1] = (i64Matrix&)beta_W;
    shared_X[0] = (i64Matrix&)alpha_X_share;
    shared_X[1] = (i64Matrix&)beta_X;
    shared_B[0] = (i64Matrix&)alpha_B_share;
    shared_B[1] = (i64Matrix&)beta_B;
	}

  sf64Matrix<D> shared_W_times_X (N, 1),shared_W_times_X_plus_B (N, 1);
  eMatrix<double> ans (N, 1);
  shared_W_times_X = p.mul(shared_W, shared_X);
  shared_W_times_X_plus_B = p.add(shared_W_times_X, shared_B);
  ans = p.reveal(shared_W_times_X_plus_B);
  std::cout<<"Ans: "<<ans<<std::endl;
	return 0;
}

int astra_logistic_reg_inference(int N, int Dim, int pIdx, bool print, CLP& cmd, Session& chlPrev, Session& chlNext, bool logistic)
{
  PRNG prng(toBlock(1));

	eMatrix<double> W(N, Dim), X(Dim, 1);

	for (u64 i = 0; i < W.size(); ++i)
	{
		  W(i) = (prng.get<int>() % 10 )/ double(10);
	}
	
  for (u64 i = 0; i < X.size(); ++i)
	{
		  X(i) = (prng.get<int>() % 10 )/ double(10);
	}
  
  std::cout<<"Actual product of matrices: "<<W*X<<"\n";

  std::cout<<"W: "<<W<<std::endl;
  std::cout<<"X: "<<X<<std::endl;
  const Decimal D = D16;
  astraML p;

  p.init(pIdx, chlPrev, chlNext, toBlock(pIdx));
  
  sf64Matrix<D> shared_W(N, Dim), shared_X(Dim, 1);

	if (pIdx == 0)
	{
		shared_W  = p.astra_share_matrix_preprocess_distributor<D>(W);
		shared_X = p.astra_share_matrix_preprocess_distributor<D>(X);

    p.astra_share_matrix_online_distributor<D>(W, shared_W);
    p.astra_share_matrix_online_distributor<D>(X, shared_X);
	}
	else
	{
    f64Matrix<D> alpha_X_share(Dim, 1), alpha_W_share(N, Dim), beta_X(Dim, 1), beta_W(N, Dim);

		alpha_W_share  = p.astra_share_matrix_preprocess_evaluator<D>(0);
		alpha_X_share = p.astra_share_matrix_preprocess_evaluator<D>(0);

    beta_W = p.astra_share_matrix_online_evaluator<D>(0);
    beta_X = p.astra_share_matrix_online_evaluator<D>(0);

    shared_W[0] = (i64Matrix&)alpha_W_share;
    shared_W[1] = (i64Matrix&)beta_W;
    shared_X[0] = (i64Matrix&)alpha_X_share;
    shared_X[1] = (i64Matrix&)beta_X;
	}

  sf64Matrix<D> shared_W_times_X (N, 1);
  eMatrix<double> ans (N, 1);
  shared_W_times_X = p.mul(shared_W, shared_X);

  f64<D> half = 0.5, minus_half = -0.5 one = 1.0;
  

  ans = p.reveal(shared_W_times_X);
  std::cout<<"Ans: "<<ans<<std::endl;
	return 0;
}

int astra_pred_inference_sh(oc::CLP& cmd)
{
  auto N = cmd.getManyOr<int>("N", {2});
  auto D = cmd.getManyOr<int>("D", {2});
  IOService ios(cmd.isSet("p") ? 3 : 7);
  std::vector<std::thread> thrds;
  for(u64 i=0; i<3; ++i)
  {
		if (cmd.isSet("p") == false || cmd.get<int>("p") == i)
		{
        thrds.emplace_back(std::thread([i, N, D, &cmd, &ios]() {
            auto next = (i+1) %3;
            auto prev = (i+2) %3;

            auto cNameNext = std::to_string(std::min(i, next)) + std::to_string(std::max(i, next));
            auto cNamePrev = std::to_string(std::min(i, prev)) + std::to_string(std::max(i, prev));

            auto modeNext = i < next ? SessionMode::Server : SessionMode::Client;
            auto modePrev = i < prev ? SessionMode::Server : SessionMode::Client;

            auto portNext = 1212 + std::min(i, next);
            auto portPrev = 1212 + std::min(i, prev);

            Session epNext(ios, "127.0.0.1", portNext, modeNext, cNameNext);
            Session epPrev(ios, "127.0.0.1", portPrev, modePrev, cNamePrev);
            std::cout<<"party "<<i<<" next "<<portNext<<" mode=server?:"<<(modeNext == SessionMode::Server) << " name " << cNameNext << std::endl;
            std::cout<<"party "<<i<<" prev "<<portPrev<<" mode=server?:"<<(modePrev == SessionMode::Server) << " name " << cNamePrev << std::endl;
            auto chlNext = epNext.addChannel();
            auto chlPrev = epPrev.addChannel();

            chlNext.waitForConnection();
            chlPrev.waitForConnection();

            chlNext.send(i);
            chlPrev.send(i);

            u64 prevAct, nextAct;
            chlNext.recv(nextAct);
            chlPrev.recv(prevAct);

            if (next != nextAct)
              std::cout<< "bad next party idx, act: " << nextAct << " exp: "<<next<<std::endl;
            if (prev != prevAct)
              std::cout<< "bad prev party idx, act: " << prevAct << " exp: "<<prev<<std::endl;

            ostreamLock(std::cout)<<"party "<<i<<" start"<<std::endl;
            auto print = cmd.isSet("p") || i == 0;
            for(auto n: N)
            {
              for(auto d: D)
              {
                astra_linear_reg_inference(n, d, i, print, cmd, epPrev, epNext, 0);
              }
            }
        }));
    }
  }

  for (auto& t : thrds)
    t.join();
  
  return 0;
}

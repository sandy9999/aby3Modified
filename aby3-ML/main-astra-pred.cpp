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

int astra_linear_reg_inference(int N, int Dim, int pIdx, bool print, CLP& cmd, Session& chlPrev, Session& chlNext)
{
  PRNG prng(toBlock(1));

	eMatrix<double> W(N, Dim), X(Dim, 1);
  eMatrix<double> B(N, 1);
  //std::cout<<"PRNG value: "<<prng.get<int>()<<std::endl;
	for (u64 i = 0; i < W.size(); ++i)
	{
		  W(i) = (prng.get<int>()%10000000)/ double(1000);
	}
	
  for (u64 i = 0; i < X.size(); ++i)
	{
		  X(i) = (prng.get<int>()%10000000)/ double(1000);
	}

  for (u64 i = 0; i < B.size(); ++i)
	{
		  B(i) = (prng.get<int>()%10000000)/ double(1000);
	}
  std::cout<<fixed;  

  std::cout<<"W: "<<W<<std::endl;
  std::cout<<"X: "<<X<<std::endl;
  std::cout<<"B: "<<B<<std::endl;
  std::cout<<"Actual product of matrices + bias: "<<W*X+B<<"\n";
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
		shared_W[0]  = p.astra_share_matrix_preprocess_evaluator(0);
		shared_X[0] = p.astra_share_matrix_preprocess_evaluator(0);
    shared_B[0] = p.astra_share_matrix_preprocess_evaluator(0);

    shared_W[1] = p.astra_share_matrix_online_evaluator(0);
    shared_X[1] = p.astra_share_matrix_online_evaluator(0);
    shared_B[1] = p.astra_share_matrix_online_evaluator(0);
	}

  sf64Matrix<D> shared_W_times_X,shared_W_times_X_plus_B;
  std::array<f64Matrix<D>, 2> intermediate_shares;
  intermediate_shares[0].resize(N, 1);
  intermediate_shares[1].resize(N, 1);
  eMatrix<double> ans (N, 1);
  std::chrono::time_point<std::chrono::system_clock>
    linearRegStop,
    linearRegStart = std::chrono::system_clock::now();

  if(pIdx == 0)
  {
      shared_W_times_X = p.mul_preprocess_distributor(shared_W, shared_X);
  }
  else
  {
      intermediate_shares = p.mul_preprocess_evaluator(shared_W, shared_X);
      shared_W_times_X = p.mul_online(shared_W, shared_X, intermediate_shares); 
  }

shared_W_times_X = p.mul(shared_W, shared_X);


  shared_W_times_X_plus_B = p.add(shared_W_times_X, shared_B);

  linearRegStop = std::chrono::system_clock::now();
  auto linearRegMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(linearRegStop - linearRegStart).count();
  std::cout<<"Time for Linear Regression in microseconds: "<<linearRegMicroSeconds<<std::endl;
  ans = p.reveal(shared_W_times_X_plus_B);
  std::cout<<"Ans: "<<ans<<std::endl;
	return 0;
}

int astra_logistic_reg_inference(int N, int Dim, int pIdx, bool print, CLP& cmd, Session& chlPrev, Session& chlNext)
{
  PRNG prng(toBlock(1));

	eMatrix<double> W(N, Dim), X(Dim, 1);

	for (u64 i = 0; i < W.size(); ++i)
	{
		  W(i) = (prng.get<int>()%10000000)/ double(1000);
	}
	
  for (u64 i = 0; i < X.size(); ++i)
	{
		  X(i) = (prng.get<int>()%10000000)/ double(1000);
	}
 
  std::cout<<fixed;
  std::cout<<"W: "<<W<<std::endl;
  std::cout<<"X: "<<X<<std::endl;
  std::cout<<"Actual product of matrices: "<<W*X<<"\n";
  const Decimal D = D16;
  astraML p;

  p.init(pIdx, chlPrev, chlNext, toBlock(pIdx));
  
  sf64Matrix<D> shared_W, shared_X;

	if (pIdx == 0)
	{
		shared_W  = p.astra_share_matrix_preprocess_distributor<D>(W);
		shared_X = p.astra_share_matrix_preprocess_distributor<D>(X);

    p.astra_share_matrix_online_distributor<D>(W, shared_W);
    p.astra_share_matrix_online_distributor<D>(X, shared_X);
	}
	else
	{
		shared_W[0]  = p.astra_share_matrix_preprocess_evaluator(0);
		shared_X[0] = p.astra_share_matrix_preprocess_evaluator(0);

    shared_W[1] = p.astra_share_matrix_online_evaluator(0);
    shared_X[1] = p.astra_share_matrix_online_evaluator(0);
	}

  sf64Matrix<D> shared_W_times_X(N, 1);
  eMatrix<double> ans (N, 1);
  f64<D> half = 0.5, minus_half = -0.5;
  sf64Matrix<D> val1, val2, shared_one;

  shared_one[0].setZero(shared_W_times_X.rows(), shared_W_times_X.cols());
  shared_one[1].setZero(shared_W_times_X.rows(), shared_W_times_X.cols());
  if(pIdx != 0)
  {
    shared_one[1].fill(1<<16);
  }

  std::chrono::time_point<std::chrono::system_clock>
    logisticRegStop,
    logisticRegStart = std::chrono::system_clock::now();

  shared_W_times_X = p.mul(shared_W, shared_X);

  val1 = p.add_const(shared_W_times_X, half);
  val2 = p.add_const(shared_W_times_X, minus_half);

  si64Matrix bool_shared_b1, bool_shared_b2;
  bool_shared_b1 = p.bit_extraction(p.reveal(val1));
  bool_shared_b2 = p.bit_extraction(p.reveal(val2));
  
  sf64Matrix<D> shared_b1, shared_b2;
  shared_b1 = p.bit_injection(bool_shared_b1, val1);
  shared_b2 = p.bit_injection(bool_shared_b2, val2);
 
  sf64Matrix<D> shared_b2_minus_b1, shared_sigmoid;
  shared_b2_minus_b1 = p.subtract(shared_b2, shared_b1);
  shared_sigmoid = p.add(shared_b2_minus_b1, shared_one);

  logisticRegStop = std::chrono::system_clock::now();
  auto logisticRegMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(logisticRegStop - logisticRegStart).count();
  std::cout<<"Time for Logistic Regression in microseconds: "<<logisticRegMicroSeconds<<std::endl;
  
  ans = p.reveal(shared_W_times_X);
  std::cout<<"Ans: "<<ans<<std::endl;
  /*
  ans = p.reveal(val1);
  std::cout<<"val1: "<<ans<<std::endl;
  ans = p.reveal(val2);
  std::cout<<"val2: "<<ans<<std::endl;
  i64Matrix ans1;
  ans1 = p.reveal(bool_shared_b1);
  std::cout<<"b1: "<<ans1<<std::endl;
  ans1 = p.reveal(bool_shared_b2);
  std::cout<<"b2: "<<ans1<<std::endl;
  ans = p.reveal(shared_b1);
  std::cout<<"b1: "<<ans<<std::endl;
  ans = p.reveal(shared_b2);
  std::cout<<"b2: "<<ans<<std::endl;
  */
  ans = p.reveal(shared_sigmoid);
  std::cout<<"sigmoid: "<<ans<<std::endl;
	return 0;
}

int astra_pred_inference_sh(oc::CLP& cmd)
{
  auto N = cmd.getManyOr<int>("N", {10});
  auto D = cmd.getManyOr<int>("D", {10});
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
              { if (cmd.isSet("log-reg"))
                  astra_logistic_reg_inference(n, d, i, print, cmd, epPrev, epNext);
                else if (cmd.isSet("lin-reg"))
                  astra_linear_reg_inference(n, d, i, print, cmd, epPrev, epNext);
              }
            }
        }));
    }
  }

  for (auto& t : thrds)
    t.join();
  
  return 0;
}

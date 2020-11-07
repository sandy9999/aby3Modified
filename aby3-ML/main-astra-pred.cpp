#include "main-astra-pred.h"
#include "aby3/sh3/Sh3Encryptor.h"
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Common/CLP.h>

using namespace aby3;
using namespace oc;

//W is weight vector, X is x coordinates vector, b is bias
//Aim: Compute W.X + b
void astra_linear_regression_inference(oc::CLP& cmd, i64Matrix W, i64Matrix X, i64 b, int no_of_cols)
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();

	CommPkg comms[3];
	comms[0] = { chl02, chl01 };
	comms[1] = { chl10, chl12 };
	comms[2] = { chl21, chl20 };

	Sh3Encryptor encs[3];
	encs[0].init(0, toBlock(0, 0), toBlock(0, 1));
	encs[1].init(1, toBlock(0, 1), toBlock(0, 2));
	encs[2].init(2, toBlock(0, 2), toBlock(0, 0));

	auto t0 = std::thread([&]() {
			auto i = 0;
			auto& enc = encs[i];
			auto& comm = comms[i];
      
      i64 alpha_w_times_alpha_x = 0, revealed_val;
      i64Matrix revealed_matrix(1, no_of_cols);
      si64Matrix shared_w(1, no_of_cols), shared_x(1, no_of_cols);
      si64 shared_b, shared_wx, shared_alpha_w_times_alpha_x, shared_wx_plus_b;

      //Generating Input Shares Preprocess
      enc.astra_share_matrix_preprocess_distributor(comm, shared_w, 0);
      enc.astra_share_matrix_preprocess_distributor(comm, shared_x, 0);
      shared_b = enc.astra_share_preprocess_distributor(comm, 0);

      //Generating Input Shares Online
      enc.astra_share_matrix_online_distributor(comm, W, shared_w, 0);
      enc.astra_share_matrix_online_distributor(comm, X, shared_x, 0);
      enc.astra_share_online_distributor(comm, b, shared_b, 0);

      //Preprocess phase
      shared_wx = enc.astra_share_preprocess_distributor(comm, 0);
      for(u64 k=0; k<shared_w.size(); k++)
        alpha_w_times_alpha_x+= (shared_w.mShares[0](k) + shared_w.mShares[1](k))*(shared_x.mShares[0](k) + shared_x.mShares[1](k));
      shared_alpha_w_times_alpha_x = enc.astra_additive_share_distributor(comm, alpha_w_times_alpha_x, 0, true);

      //Local Addition with Bias Shares
      shared_wx_plus_b[0] = shared_wx[0] + shared_b[0];
      shared_wx_plus_b[1] = shared_wx[1] + shared_b[1];

      //Reveal actual value to check
      enc.astra_share_reveal_sender(comm, shared_wx_plus_b, 0);
      revealed_val = enc.astra_share_reveal_receiver(comm, shared_wx_plus_b, 0);
      ostreamLock(std::cout)<<revealed_val<<std::endl;

	});

	auto rr = [&](int i) {
		auto& enc = encs[i];
		auto& comm = comms[i];
    
    i64 alpha_w_times_alpha_x_share, beta_wx_1, beta_wx_2, revealed_val;
    i64Matrix revealed_matrix(1, no_of_cols);
    si64Matrix shared_w(1, no_of_cols), shared_x(1, no_of_cols);
    si64 shared_b, shared_wx, shared_wx_plus_b;

    //Generating Input Shares Preprocess
    enc.astra_share_matrix_preprocess_evaluator(comm, shared_w.mShares[0], i);
    enc.astra_share_matrix_preprocess_evaluator(comm, shared_x.mShares[0], i);
    shared_b[0] = enc.astra_share_preprocess_evaluator(comm, i);
    
    //Generating Input Shares Online
    enc.astra_share_matrix_online_evaluator(comm, shared_w.mShares[1], i);
    enc.astra_share_matrix_online_evaluator(comm, shared_x.mShares[1], i);
    shared_b[1] = enc.astra_share_online_evaluator(comm, i);

    //Preprocess phase
    shared_wx[0] = enc.astra_share_preprocess_evaluator(comm, i);
    alpha_w_times_alpha_x_share = enc.astra_additive_share_evaluator(comm, i);

    //Online phase
    if (i == 1)
    {
        beta_wx_1 = (shared_w.mShares[1]*shared_x.mShares[1].transpose())(0) - (shared_w.mShares[1]*shared_x.mShares[0].transpose())(0) - (shared_x.mShares[1]*shared_w.mShares[0].transpose())(0) + alpha_w_times_alpha_x_share + shared_wx[0];
        comm.mNext.asyncSendCopy(beta_wx_1);
        comm.mNext.recv(beta_wx_2);
    } 
    else if(i == 2)
    {

        beta_wx_2 = - (shared_w.mShares[1]*shared_x.mShares[0].transpose())(0) - (shared_x.mShares[1]*shared_w.mShares[0].transpose())(0) + alpha_w_times_alpha_x_share + shared_wx[0];
        comm.mPrev.asyncSendCopy(beta_wx_2);
        comm.mPrev.recv(beta_wx_1);
    }
    shared_wx[1] = beta_wx_1 + beta_wx_2;

    //Local Addition with Bias Shares
    shared_wx_plus_b[0] = shared_wx[0] + shared_b[0];
    shared_wx_plus_b[1] = shared_wx[1] + shared_b[1];

    enc.astra_share_reveal_sender(comm, shared_wx_plus_b, i);
    revealed_val = enc.astra_share_reveal_receiver(comm, shared_wx_plus_b, i);
    ostreamLock(std::cout)<<revealed_val<<std::endl;
	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();
}

int astra_linear_regression_inference_sh(oc::CLP& cmd)
{
  i64Matrix W(1, 3),X(1, 3);
  i64 b = 5;
  for(u64 i=0; i<W.size(); ++i)
    W(i) = i;

  for(u64 i=0; i<X.size(); ++i)
    X(i) = i+3;
  ostreamLock(std::cout)<<"W: "<<W<<std::endl;
  ostreamLock(std::cout)<<"X: "<<X<<std::endl;
  ostreamLock(std::cout)<<"b: "<<b<<std::endl;
  astra_linear_regression_inference(cmd, W, X, b, 3);
  return 0;
}

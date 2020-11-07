#include "main-astra-logistic-pred.h"
#include "aby3/sh3/Sh3Encryptor.h"
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Common/CLP.h>

using namespace aby3;
using namespace oc;

//W is weight vector, X is x coordinates vector, b is bias
//Aim: Compute W.X + b
void astra_logistic_regression_inference(oc::CLP& cmd, i64Matrix W, i64Matrix X, i64 b, int no_of_cols)
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
      
      i64 bit1 = 0, bit2 = 0, revealed_val;
      sb64 binary_shared_bit1, binary_shared_bit2, binary_shared_not_bit2, binary_shared_bit1_and_not_bit2;

      //Generating Input Shares Preprocess
      binary_shared_bit1 = enc.astra_binary_share_preprocess_distributor(comm, 0);
      binary_shared_bit2 = enc.astra_binary_share_preprocess_distributor(comm, 0);

      //Generating Input Shares Online
      enc.astra_binary_share_online_distributor(comm, bit1, binary_shared_bit1, 0);
      enc.astra_binary_share_online_distributor(comm, bit2, binary_shared_bit2, 0);

      //Generate shares of not_bit2
      binary_shared_not_bit2 = binary_shared_bit2;

      //Preprocess phase
      binary_shared_bit1_and_not_bit2 = enc.astra_binary_share_preprocess_distributor(comm, 0);
      enc.astra_binary_additive_share_distributor(comm, (binary_shared_bit1[0] ^ binary_shared_not_bit2[0])&(binary_shared_not_bit2[0]^binary_shared_not_bit2[1]), 0, true);

      //Reveal actual value to check
      enc.astra_binary_share_reveal_sender(comm, binary_shared_bit1_and_not_bit2, 0);
      revealed_val = enc.astra_binary_share_reveal_receiver(comm, binary_shared_bit1_and_not_bit2, 0);
      ostreamLock(std::cout)<<revealed_val<<std::endl;

	});

	auto rr = [&](int i) {
		auto& enc = encs[i];
		auto& comm = comms[i];
    
    i64 alpha_bit1_and_alpha_not_bit2_share, beta_bit1_and_not_bit2_1, beta_bit1_and_not_bit2_2, revealed_val;
    sb64 binary_shared_bit1, binary_shared_bit2, binary_shared_not_bit2, binary_shared_bit1_and_not_bit2;

    //Generating Input Shares Preprocess
    binary_shared_bit1[0] = enc.astra_binary_share_preprocess_evaluator(comm, i);
    binary_shared_bit2[0] = enc.astra_binary_share_preprocess_evaluator(comm, i);
    
    //Generating Input Shares Online
    binary_shared_bit1[1] = enc.astra_binary_share_online_evaluator(comm, i);
    binary_shared_bit2[1] = enc.astra_binary_share_online_evaluator(comm, i);

    //Generate shares of not_bit2
    binary_shared_not_bit2[0] = binary_shared_bit2[0];
    binary_shared_not_bit2[1] = 1^binary_shared_bit2[1];

    //Preprocess phase
    binary_shared_bit1_and_not_bit2[0] = enc.astra_binary_share_preprocess_evaluator(comm, i); 
    alpha_bit1_and_alpha_not_bit2_share = enc.astra_binary_additive_share_evaluator(comm, i);

    //Online phase
    if (i == 1)
    {
        beta_bit1_and_not_bit2_1 = (binary_shared_bit1[1] & binary_shared_not_bit2[1]) ^ (binary_shared_bit1[1] & binary_shared_not_bit2[0]) ^ (binary_shared_not_bit2[1] & binary_shared_bit1[0]) ^ alpha_bit1_and_alpha_not_bit2_share ^ binary_shared_bit1_and_not_bit2[0];
        comm.mNext.asyncSendCopy(beta_bit1_and_not_bit2_1);
        comm.mNext.recv(beta_bit1_and_not_bit2_2);
    } 
    else if(i == 2)
    {

        beta_bit1_and_not_bit2_2 = (binary_shared_bit1[1] & binary_shared_not_bit2[0]) ^ (binary_shared_not_bit2[1] & binary_shared_bit1[0]) ^ alpha_bit1_and_alpha_not_bit2_share ^ binary_shared_bit1_and_not_bit2[0];
        comm.mPrev.asyncSendCopy(beta_bit1_and_not_bit2_2);
        comm.mPrev.recv(beta_bit1_and_not_bit2_1);
    }
    binary_shared_bit1_and_not_bit2[1] = beta_bit1_and_not_bit2_1 ^ beta_bit1_and_not_bit2_2;

    enc.astra_binary_share_reveal_sender(comm, binary_shared_bit1_and_not_bit2, i);
    revealed_val = enc.astra_binary_share_reveal_receiver(comm, binary_shared_bit1_and_not_bit2, i);
    ostreamLock(std::cout)<<revealed_val<<std::endl;
	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();
}

int astra_logistic_regression_inference_sh(oc::CLP& cmd)
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
  astra_logistic_regression_inference(cmd, W, X, b, 3);
  return 0;
}

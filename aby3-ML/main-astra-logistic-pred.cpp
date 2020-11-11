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
void astra_logistic_regression_inference(oc::CLP& cmd, i64Matrix W, i64Matrix X, i64 b, int no_of_cols, i64 a)
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
      
      i64 bit1 = 1, bit2 = 0, revealed_val;
      sb64 binary_shared_bit1, binary_shared_bit2, binary_shared_not_bit2, binary_shared_c;//Let c = bit1.(not_bit2) in boolean sharing

      si64 shared_a, shared_ca_1, shared_ca_2, shared_ca;

      si64 shared_bit2, shared_bit2_1, shared_bit2_2;

      si64 shared_sigmoid;

      //Generating Input Shares Preprocess
      binary_shared_bit1 = enc.astra_binary_share_preprocess_distributor(comm, 0);
      binary_shared_bit2 = enc.astra_binary_share_preprocess_distributor(comm, 0);
      shared_a = enc.astra_share_preprocess_distributor(comm, 0);

      //Generating Input Shares Online
      enc.astra_binary_share_online_distributor(comm, bit1, binary_shared_bit1, 0);
      enc.astra_binary_share_online_distributor(comm, bit2, binary_shared_bit2, 0);
      enc.astra_share_online_distributor(comm, a, shared_a, 0);

      //Generate shares of not_bit2
      binary_shared_not_bit2 = binary_shared_bit2;

      //Preprocess phase of bit1 and not_bit2
      binary_shared_c = enc.astra_binary_share_preprocess_distributor(comm, 0);
      enc.astra_binary_additive_share_distributor(comm, (binary_shared_bit1[0] ^ binary_shared_bit1[1])&(binary_shared_not_bit2[0]^binary_shared_not_bit2[1]), 0, true);

      //Reveal actual value to check
      enc.astra_binary_share_reveal_sender(comm, binary_shared_c, 0);
      revealed_val = enc.astra_binary_share_reveal_receiver(comm, binary_shared_c, 0);
      ostreamLock(std::cout)<<revealed_val<<std::endl;


      //Now that have c, let's convert [[c]]^B [[a]] to [[ca]]
      //Bit Injection Preprocess
      enc.astra_additive_share_distributor(comm, (binary_shared_c[0]^binary_shared_c[1]), 0, true);
      enc.astra_additive_share_distributor(comm, ((binary_shared_c[0]^binary_shared_c[1])*(shared_a[0] + shared_a[1])), 0, true);
    
      //Preprocess - Generate Astra shares of Bit Injection Local shares
      shared_ca_1 = enc.astra_share_preprocess_evaluator_notP0(comm, 0, 1);
      shared_ca_2 = enc.astra_share_preprocess_evaluator_notP0(comm, 0, 2);
      
      //Local addition of Astra shares to get [[ca]]
      shared_ca[0] = shared_ca_1[0] + shared_ca_2[0];
      shared_ca[1] = shared_ca_1[1] + shared_ca_2[1];

      //Reveal actual value to check
      enc.astra_share_reveal_sender(comm, shared_ca, 0);
      revealed_val = enc.astra_share_reveal_receiver(comm, shared_ca, 0);
      ostreamLock(std::cout)<<revealed_val<<std::endl;

      //Bit2A preprocessing
      enc.astra_additive_share_distributor(comm, binary_shared_bit2[0]^binary_shared_bit2[1], 0, true);
      shared_bit2_1 = enc.astra_share_preprocess_evaluator_notP0(comm, 0, 1);
      shared_bit2_2 = enc.astra_share_preprocess_evaluator_notP0(comm, 0, 2);
      shared_bit2[0] = shared_bit2_1[0] + shared_bit2_2[0];
      shared_bit2[1] = shared_bit2_1[1] + shared_bit2_2[1];

      //Addition of local shares obtained from bit injection and bit2A to get final protocol
      shared_sigmoid[0] = shared_ca[0] + shared_bit2[0];
      shared_sigmoid[1] = shared_ca[1] + shared_bit2[1];

      //Reveal actual value to check
      enc.astra_share_reveal_sender(comm, shared_sigmoid, 0);
      revealed_val = enc.astra_share_reveal_receiver(comm, shared_sigmoid, 0);
      ostreamLock(std::cout)<<revealed_val<<std::endl;
      
	});

	auto rr = [&](int i) {
		auto& enc = encs[i];
		auto& comm = comms[i];
    
    i64 binary_alpha_c_share, beta_c_1, beta_c_2, revealed_val;
    sb64 binary_shared_bit1, binary_shared_bit2, binary_shared_not_bit2, binary_shared_c;

    i64 alpha_c_share, alpha_c_times_alpha_a_share, ca_share, alpha_bit2_share;
    si64 shared_a, shared_ca, shared_ca_1, shared_ca_2, alpha_ca_share;

    i64 bit2_share;
    si64 shared_bit2, shared_bit2_1, shared_bit2_2, alpha_bit2_arith_share;

    si64 shared_sigmoid;

    //Generating Input Shares Preprocess
    binary_shared_bit1[0] = enc.astra_binary_share_preprocess_evaluator(comm, i);
    binary_shared_bit2[0] = enc.astra_binary_share_preprocess_evaluator(comm, i);
    shared_a[0] = enc.astra_share_preprocess_evaluator(comm, i);
    
    //Generating Input Shares Online
    binary_shared_bit1[1] = enc.astra_binary_share_online_evaluator(comm, i);
    binary_shared_bit2[1] = enc.astra_binary_share_online_evaluator(comm, i);
    shared_a[1] = enc.astra_share_online_evaluator(comm, i);

    //Generate shares of not_bit2
    binary_shared_not_bit2[0] = binary_shared_bit2[0];
    binary_shared_not_bit2[1] = 1^binary_shared_bit2[1];

    //Preprocess phase of boolean multiplication of bit1 and not_bit2
    binary_shared_c[0] = enc.astra_binary_share_preprocess_evaluator(comm, i); 
    binary_alpha_c_share = enc.astra_binary_additive_share_evaluator(comm, i);

    //Online phase of boolean multiplication of bit1 and not_bit2
    if (i == 1)
    {
        beta_c_1 = (binary_shared_bit1[1] & binary_shared_not_bit2[1]) ^ (binary_shared_bit1[1] & binary_shared_not_bit2[0]) ^ (binary_shared_not_bit2[1] & binary_shared_bit1[0]) ^ binary_alpha_c_share ^ binary_shared_c[0];
        comm.mNext.asyncSendCopy(beta_c_1);
        comm.mNext.recv(beta_c_2);
    } 
    else if(i == 2)
    {
        beta_c_2 = (binary_shared_bit1[1] & binary_shared_not_bit2[0]) ^ (binary_shared_not_bit2[1] & binary_shared_bit1[0]) ^ binary_alpha_c_share ^ binary_shared_c[0];
        comm.mPrev.asyncSendCopy(beta_c_2);
        comm.mPrev.recv(beta_c_1);
    }
    binary_shared_c[1] = beta_c_1 ^ beta_c_2;
  
    //Reveal actual value to check
    enc.astra_binary_share_reveal_sender(comm, binary_shared_c, i);
    revealed_val = enc.astra_binary_share_reveal_receiver(comm, binary_shared_c, i);
    ostreamLock(std::cout)<<revealed_val<<std::endl;
    
    //Bit Injection Preprocess
    alpha_c_share = enc.astra_additive_share_evaluator(comm, i);
    alpha_c_times_alpha_a_share = enc.astra_additive_share_evaluator(comm, i);
    
    //Bit Injection Online
    if (i == 1)
    {
        ca_share = (binary_shared_c[1]*shared_a[1]) - (binary_shared_c[1]*shared_a[0]) + (alpha_c_share*shared_a[1]) - alpha_c_times_alpha_a_share - (2*binary_shared_c[1]*alpha_c_share*shared_a[1]) + (2*binary_shared_c[1]*alpha_c_times_alpha_a_share);
    //Preprocess - Generate Astra shares of Bit Injection Local shares
    alpha_ca_share = enc.astra_share_preprocess_distributor(comm, i);
    shared_ca_2[0] = enc.astra_share_preprocess_evaluator(comm, i, 2);

    //Online - Generate Astra shares of Bit Injection Local shares
    shared_ca_1[1] = enc.astra_share_online_distributor_notP0(comm, ca_share, alpha_ca_share, i);
    shared_ca_1[0] = alpha_ca_share[0];
    shared_ca_2[1] = enc.astra_share_online_evaluator(comm, i, 2);

    }
    else if(i == 2)
    {
        ca_share = 0 - (binary_shared_c[1]*shared_a[0]) + (alpha_c_share*shared_a[1]) - alpha_c_times_alpha_a_share - (2*binary_shared_c[1]*alpha_c_share*shared_a[1]) + (2*binary_shared_c[1]*alpha_c_times_alpha_a_share);
    //Preprocess - Generate Astra shares of Bit Injection Local shares
    alpha_ca_share = enc.astra_share_preprocess_distributor(comm, i);
    shared_ca_1[0] = enc.astra_share_preprocess_evaluator(comm, i, 1);

    //Online - Generate Astra shares of Bit Injection Local shares
    shared_ca_2[1] = enc.astra_share_online_distributor_notP0(comm, ca_share, alpha_ca_share, i);
    shared_ca_2[0] = alpha_ca_share[0];
    shared_ca_1[1] = enc.astra_share_online_evaluator(comm, i, 1);

    }
    
    //Local addition of Astra shares to get [[ca]]
    shared_ca[0] = shared_ca_1[0] + shared_ca_2[0];
    shared_ca[1] = shared_ca_1[1] + shared_ca_2[1];

    //Reveal actual value to check
    enc.astra_share_reveal_sender(comm, shared_ca, i);
    revealed_val = enc.astra_share_reveal_receiver(comm, shared_ca, i);
    ostreamLock(std::cout)<<revealed_val<<std::endl;
    
    //Bit2A Preprocessing
    alpha_bit2_share = enc.astra_additive_share_evaluator(comm, i);

    //Bit2A Online
    if (i == 1)
    {
        bit2_share = binary_shared_bit2[1] + alpha_bit2_share - 2*binary_shared_bit2[1]*alpha_bit2_share;
    //Preprocess - Generate Astra shares of Bit2A Local shares
    alpha_bit2_arith_share = enc.astra_share_preprocess_distributor(comm, i);
    shared_bit2_2[0] = enc.astra_share_preprocess_evaluator(comm, i, 2);

    //Online - Generate Astra shares of Bit2A Local shares
    shared_bit2_1[1] = enc.astra_share_online_distributor_notP0(comm, bit2_share, alpha_bit2_arith_share, i);
    shared_bit2_1[0] = alpha_bit2_arith_share[0];
    shared_bit2_2[1] = enc.astra_share_online_evaluator(comm, i, 2);

    }
    else if(i == 2)
    {

        bit2_share = alpha_bit2_share - 2*binary_shared_bit2[1]*alpha_bit2_share;
    //Preprocess - Generate Astra shares of Bit2A Local shares
    alpha_bit2_arith_share = enc.astra_share_preprocess_distributor(comm, i);
    shared_bit2_1[0] = enc.astra_share_preprocess_evaluator(comm, i, 1);

    //Online - Generate Astra shares of Bit2A Local shares
    shared_bit2_2[1] = enc.astra_share_online_distributor_notP0(comm, bit2_share, alpha_bit2_arith_share, i);
    shared_bit2_2[0] = alpha_bit2_arith_share[0];
    shared_bit2_1[1] = enc.astra_share_online_evaluator(comm, i, 1);
    
    }
    shared_bit2[0] = shared_bit2_1[0] + shared_bit2_2[0];
    shared_bit2[1] = shared_bit2_1[1] + shared_bit2_2[1];

    //Addition of local shares obtained from bit injection and bit2A to get final protocol
    shared_sigmoid[0] = shared_ca[0] + shared_bit2[0];
    shared_sigmoid[1] = shared_ca[1] + shared_bit2[1];
    
    //Reveal actual value to check
    enc.astra_share_reveal_sender(comm, shared_sigmoid, i);
    revealed_val = enc.astra_share_reveal_receiver(comm, shared_sigmoid, i);
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
  astra_logistic_regression_inference(cmd, W, X, b, 3, 5);
  return 0;
}

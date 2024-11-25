#include "main-astra-pred.h"
#include "astraML.h"
#include <cryptoTools/Common/CLP.h>

using namespace aby3;
using namespace std;
using namespace oc;

//W is weight vector, X is x coordinates vector, b is bias
//Aim: Compute W.X + b
share_value astra_linear_reg_inference(oc::CLP& cmd, vector<vector<share_value>> shared_W, vector<vector<share_value>> shared_X, share_value shared_b, CommPkg comms[3], astraML& a)
{

  share_value shared_W_dot_X = a.dot_product(shared_W, shared_X, comms);
  share_value shared_W_dot_X_plus_b = a.local_add(shared_W_dot_X, shared_b);
  
  ostreamLock(std::cout)<<shared_W_dot_X_plus_b.beta - (shared_W_dot_X_plus_b.alpha_1 + shared_W_dot_X_plus_b.alpha_2)<<std::endl;

  return shared_W_dot_X_plus_b;

}

//W is weight vector, X is coordinates vector, b is bas
//Aim: Compute sigmoid(W.X + b)
share_value astra_logistic_reg_inference(oc::CLP& cmd, vector<vector<share_value>> shared_W, vector<vector<share_value>> shared_X, share_value shared_b, CommPkg comms[3], astraML& a)
{
  
  share_value shared_W_dot_X = a.dot_product(shared_W, shared_X, comms);

  //share_value share_minus_one = a.generate_shares(-1);
  //share_value share_100 = a.generate_shares(100);
  //share_value val_test = a.dot_product(share_minus_one, share_100, comms);

  share_value val1 = a.add_const(shared_W_dot_X, 1);
  share_value val2 = a.add_const(shared_W_dot_X, -1);

  //share_value val1 = a.add_const(val_test, 1);
  //share_value val2 = a.add_const(val_test, -1);

  bool_share_value b1 = a.bit_extraction(val1);
  bool_share_value b2 = a.bit_extraction(val2);
  bool_share_value not_b2 = a.not_bool_share(b2);
  bool_share_value not_b1 = a.not_bool_share(b1);
  bool_share_value c = a.bit_multiplication(not_b1, b2, comms);
  share_value cx = a.bit_injection(c, val1, comms);
  share_value shared_not_b2 = a.bit2A(not_b2, comms);
  share_value sigmoid_shared_W_dot_X = a.local_add(cx, shared_not_b2);
   
  ostreamLock(std::cout)<<(sigmoid_shared_W_dot_X.beta - (sigmoid_shared_W_dot_X.alpha_1 + sigmoid_shared_W_dot_X.alpha_2))<<std::endl;

  return sigmoid_shared_W_dot_X;

}

int astra_pred_inference_sh(oc::CLP& cmd)
{

  oc::IOService ios;

  auto chl01 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Server, "01").addChannel();
  auto chl10 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Client, "01").addChannel();
  auto chl02 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Server, "02").addChannel();
  auto chl20 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Client, "02").addChannel();
  auto chl12 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Server, "12").addChannel();
  auto chl21 = oc::Session(ios, "127.0.0.1:1313", oc::SessionMode::Client, "12").addChannel();

  CommPkg comms[3];
  comms[0] = { chl02, chl01 };
  comms[1] = { chl10, chl12 };
  comms[2] = { chl21, chl20 };

  //f64<D8> fixedInt = 34.1234;
  //std::cout<<fixedInt<<std::endl;

  //share_value shared_fixedInt; 

  astraML a;
  a.key_channel_setup();

  //shared_fixedInt = a.generate_shares(fixedInt.mValue);
  //f64<D8> reconstructed_val;
  //reconstructed_val.mValue = shared_fixedInt.beta - (shared_fixedInt.alpha_1 + shared_fixedInt.alpha_2);
  //std::cout<<reconstructed_val<<std::endl;
  //f64<D8> aa = 2/double(3);
  //std::cout<<aa<<std::endl;
   
  vector<vector<i64>> W, X;
  i64 b = 5;
  for(u64 i=0; i<1; ++i)
  {
    vector<i64> row;
    for(u64 j=0; j<10; ++j)
        row.push_back(j+1);
    W.push_back(row);
  }

  for(u64 i=0; i<1; ++i)
  {
    vector<i64> row;
    for(u64 j=0; j<10; ++j)
        row.push_back(j+10);
    X.push_back(row);
  }
  
  ostreamLock(std::cout)<<"W: ";
  for(u64 i=0; i<W.size(); ++i)
  {
    for(u64 j=0; j<W[0].size(); ++j)
      ostreamLock(std::cout)<<W[i][j]<<' ';
    ostreamLock(std::cout)<<std::endl;
  }
  
  ostreamLock(std::cout)<<"X: ";
  for(u64 i=0; i<X.size(); ++i)
  {
    for(u64 j=0; j<X[0].size(); ++j)
      ostreamLock(std::cout)<<X[i][j]<<' ';
    ostreamLock(std::cout)<<std::endl;
  }
  
  ostreamLock(std::cout)<<"b: ";
  ostreamLock(std::cout)<<b<<std::endl;

  vector<vector<share_value>> shared_W, shared_X;
  share_value shared_b;
  shared_W = a.generate_shares(W);
  shared_X = a.generate_shares(X);
  shared_b = a.generate_shares(b);

  if (cmd.isSet("linear-reg"))
  {
    astra_linear_reg_inference(cmd, shared_W, shared_X, shared_b, comms, a);
  }
  else if (cmd.isSet("logistic-reg"))
  {
    astra_logistic_reg_inference(cmd, shared_W, shared_X, shared_b, comms, a);
  }

  return 0;
}

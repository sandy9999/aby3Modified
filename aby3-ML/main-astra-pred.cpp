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
  //f64<D16> val1 = 5.3;
  //share_value share_5 = a.generate_shares(val1);

  //f64<D16> val2 = 7.5;
  //share_value share_7 = a.generate_shares(val2);

  //share_value prod_val1_val2 = a.dot_product(share_5, share_7, comms);
  //f64<D16> tempans;
  //tempans.mValue = (prod_val1_val2.beta - (prod_val1_val2.alpha_1 + prod_val1_val2.alpha_2))>>16;
  //std::cout<<"Temp ans: "<<tempans<<std::endl;

  std::chrono::time_point<std::chrono::system_clock>
    linearRegStop,
    linearRegStart = std::chrono::system_clock::now();

  share_value shared_W_dot_X = a.dot_product(shared_W, shared_X, comms);
  share_value shared_W_dot_X_plus_b = a.local_add(shared_W_dot_X, shared_b);
  
  linearRegStop = std::chrono::system_clock::now();

  auto linearRegSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(linearRegStop - linearRegStart).count() / 1000.0;
  ostreamLock(std::cout)<<"Time for Linear Regression Inference in seconds: "<<linearRegSeconds<<std::endl;

  f64<D16> ans;
  ans.mValue = shared_W_dot_X_plus_b.beta - (shared_W_dot_X_plus_b.alpha_1 + shared_W_dot_X_plus_b.alpha_2);
  ostreamLock(std::cout)<<ans<<std::endl;

  return shared_W_dot_X_plus_b;
  //return shared_b;
}

//W is weight vector, X is coordinates vector, b is bas
//Aim: Compute sigmoid(W.X + b)
share_value astra_logistic_reg_inference(oc::CLP& cmd, vector<vector<share_value>> shared_W, vector<vector<share_value>> shared_X, share_value shared_b, CommPkg comms[3], astraML& a)
{
  
  share_value shared_W_dot_X = a.dot_product(shared_W, shared_X, comms);
  
  f64<D16> half = 0.5, minus_half = -0.5, one = 1.0;
  
  share_value shared_one;
  shared_one.alpha_1 = 0;
  shared_one.alpha_2 = 0;
  shared_one.beta = one.mValue;

  share_value val1 = a.add_const(shared_W_dot_X, half);
  share_value val2 = a.add_const(shared_W_dot_X, minus_half);

  bool_share_value b1 = a.bit_extraction(val1);
  bool_share_value b2 = a.bit_extraction(val2);

  share_value b1_val1 = a.bit_injection(b1, val1, comms);
  share_value b2_val2 = a.bit_injection(b2, val2, comms);

  share_value one_plus_b2_val2 = a.local_add(shared_one, b2_val2);
  share_value sigmoid_shared_W_dot_X = a.local_subtract(one_plus_b2_val2, b1_val1);

  //bool_share_value not_b2 = a.not_bool_share(b2);
  //bool_share_value not_b1 = a.not_bool_share(b1);
  //bool_share_value c = a.bit_multiplication(not_b1, b2, comms);
  //share_value cx = a.bit_injection(c, val1, comms);
  //share_value shared_not_b2 = a.bit2A(not_b2, comms);
  //share_value sigmoid_shared_W_dot_X = a.local_add(cx, shared_not_b2);
   f64<D16> ans;
   ans.mValue = sigmoid_shared_W_dot_X.beta - (sigmoid_shared_W_dot_X.alpha_1 + sigmoid_shared_W_dot_X.alpha_2);
  ostreamLock(std::cout)<<ans<<std::endl;

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

  astraML a;
  a.key_channel_setup();
   
  vector<vector<f64<D16>>> W, X;
  f64<D16> b = 5;
  for(u64 i=0; i<1; ++i)
  {
    vector<f64<D16>> row;
    for(u64 j=0; j<10; ++j)
        row.push_back((j-j)/double(3));
    W.push_back(row);
  }

  for(u64 i=0; i<1; ++i)
  {
    vector<f64<D16>> row;
    for(u64 j=0; j<10; ++j)
        row.push_back((j+10)/double(3));
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

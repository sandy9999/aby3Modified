#include "main-astra-pred.h"
#include "astraML.h"
#include <cryptoTools/Common/CLP.h>

using namespace aby3;
using namespace std;
using namespace oc;

//W is weight vector, X is x coordinates vector, b is bias
//Aim: Compute W.X + b
share_value astra_linear_reg_inference(oc::CLP& cmd, vector<vector<i64>> W, vector<vector<i64>> X, i64 b)
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

  vector<vector<share_value>> shared_W, shared_X;
  share_value shared_b;
  shared_W = a.generate_shares(W);
  shared_X = a.generate_shares(X);
  shared_b = a.generate_shares(b);
  
  share_value shared_W_dot_X = a.dot_product(shared_W, shared_X, comms);
  share_value shared_W_dot_X_plus_b = a.local_add(shared_W_dot_X, shared_b);
  
  ostreamLock(std::cout)<<shared_W_dot_X_plus_b.beta - (shared_W_dot_X_plus_b.alpha_1 + shared_W_dot_X_plus_b.alpha_2)<<std::endl;

  return shared_W_dot_X_plus_b;

}

int astra_linear_reg_inference_sh(oc::CLP& cmd)
{
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

  astra_linear_reg_inference(cmd, W, X, b);
  return 0;
}

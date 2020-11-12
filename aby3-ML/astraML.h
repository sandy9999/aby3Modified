#pragma once
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/Channel.h>
#include <aby3/Common/Defines.h>
#include <aby3/sh3/Sh3FixedPoint.h>
#include <aby3/sh3/Sh3Encryptor.h>
#include <aby3/sh3/Sh3Evaluator.h>
#include <aby3/sh3/AstraSh3ShareGen.h>

namespace aby3
{
  struct share_value {
    i64 alpha_1, alpha_2, beta;
  };

	class astraML
	{
	public:
    //Sh3Encryptor encs[3];
    AstraSh3ShareGen shareGen;

    void key_channel_setup()
    {
       shareGen.init();//Key setup
    }
    
    share_value generate_shares(i64 x)
    {
      share_value ret;
      ret.alpha_1 = shareGen.getShare(0, 1, -1, 0);
      shareGen.getShare(0, 1, -1, 1);
      ret.alpha_2 = shareGen.getShare(0, 2, -1, 0);
      shareGen.getShare(0, 2, -1, 2);
      ret.beta = x + ret.alpha_1 + ret.alpha_2;
      return ret;
    }

    std::vector<std::vector<share_value>> generate_shares(std::vector<std::vector<i64>> x)
    {
      std::vector<std::vector<share_value>> ret;
      for(u64 i=0; i<x.size(); ++i)
      {
        std::vector<share_value> row;
        for(u64 j=0; j<x[0].size(); ++j)
        {
          share_value val;
          val.alpha_1 = shareGen.getShare(0, 1, -1, 0);
          shareGen.getShare(0, 1, -1, 1);
          val.alpha_2 = shareGen.getShare(0, 2, -1, 0);
          shareGen.getShare(0, 2, -1, 2);
          val.beta = x[i][j] + val.alpha_1 + val.alpha_2;
          row.push_back(val);
        }
        ret.push_back(row);
      }
      return ret;
    }
    
    si64 get_allocated_share(share_value x, int partyIdx)
    {
        si64 ret;
        if(partyIdx!=2)
          ret[0] = x.alpha_1;
        else
          ret[0] = x.alpha_2;

        if(partyIdx!=0)
          ret[1] = x.beta;
        else
          ret[1] = x.alpha_2;
        return ret;
    }

    void get_allocated_share(std::vector<std::vector<share_value>> x, int partyIdx, si64Matrix& ret)
    {
        if (ret.mShares[0].cols() != static_cast<u64>(x[0].size()) || ret.mShares[0].rows() != static_cast<u64>(x.size()))
            throw std::runtime_error(LOCATION);
        for(u64 i=0, k=0; i<x.size(); ++i)
        {
          for(u64 j=0; j<x[0].size(); ++j, ++k)
          {
            if(partyIdx!=2)
              ret.mShares[0](k) = x[i][j].alpha_1;
            else
              ret.mShares[0](k) = x[i][j].alpha_2;

            if(partyIdx!=0)
              ret.mShares[1](k) = x[i][j].beta;
            else
              ret.mShares[1](k) = x[i][j].alpha_2;
          }
        }
    }

    share_value dot_product(share_value x, share_value y, CommPkg comms[3])
    {
      share_value z;
      //Consider z = x.y
      auto t0 = std::thread([&]() {
          auto i = 0;
          //auto& enc = encs[i];
          auto& comm = comms[i];
          
          si64 shared_x = get_allocated_share(x, 0);
          si64 shared_y = get_allocated_share(y, 0);
          si64 shared_z;

          //Preprocessing Phase
          i64 alpha_z_1 = shareGen.getShare(0, 1, -1, 0), alpha_z_2 = shareGen.getShare(0, 2, -1, 0);
          i64 alpha_x_alpha_y_1 = shareGen.getShare(0, 1, -1, 0), alpha_x_alpha_y_2 = (shared_x[0]+shared_x[1])*(shared_y[0] + shared_y[1]) - alpha_x_alpha_y_1;
          comm.mPrev.asyncSendCopy(alpha_x_alpha_y_2);

          shared_z[0] = alpha_z_1;
          shared_z[1] = alpha_z_2;

          z.alpha_1 = shared_z[0];
          z.alpha_2 = shared_z[1];
      });

      auto rr = [&](int i) {
        //auto& enc = encs[i];
        auto& comm = comms[i];
        
        si64 shared_x = get_allocated_share(x, i);
        si64 shared_y = get_allocated_share(y, i);
        si64 shared_z;
        
        //Preprocessing Phase
        i64 alpha_z_share = shareGen.getShare(0, i, -1, i);
        i64 alpha_x_alpha_y_share;
        if(i == 1)
            alpha_x_alpha_y_share = shareGen.getShare(0, 1, -1, 1);
        else if(i == 2)
            comm.mNext.recv(alpha_x_alpha_y_share);

        //Online phase
        i64 beta_z_1, beta_z_2;
        if (i == 1)
        {
            beta_z_1 = (shared_x[1]*shared_y[1]) - (shared_x[1]*shared_y[0]) - (shared_y[1]*shared_x[0]) + alpha_x_alpha_y_share + alpha_z_share;
            comm.mNext.asyncSendCopy(beta_z_1);
            comm.mNext.recv(beta_z_2);
        } 
        else if(i == 2)
        {

            beta_z_2 = 0 - (shared_x[1]*shared_y[0]) - (shared_y[1]*shared_x[0]) + alpha_x_alpha_y_share + alpha_z_share;
            comm.mPrev.asyncSendCopy(beta_z_2);
            comm.mPrev.recv(beta_z_1);
        }
        shared_z[1] = beta_z_1 + beta_z_2;
        shared_z[0] = alpha_z_share;

        z.beta = shared_z[1];
  	};
  
	  auto t1 = std::thread(rr, 1);
  	auto t2 = std::thread(rr, 2);
  
	  t0.join();
  	t1.join();
  	t2.join();
    return z;

    }
    
    share_value dot_product(std::vector<std::vector<share_value>> x, std::vector<std::vector<share_value>> y, CommPkg comms[3])
    {
      share_value z;
      //Consider z = x.y
      auto t0 = std::thread([&]() {
          auto i = 0;
          //auto& enc = encs[i];
          auto& comm = comms[i];
          
          si64Matrix shared_x(x.size(), x[0].size()), shared_y(y.size(), y[0].size());
          get_allocated_share(x, 0, shared_x);
          get_allocated_share(y, 0, shared_y);
          si64 shared_z;

          //Preprocessing Phase
          i64 alpha_z_1 = shareGen.getShare(0, 1, -1, 0), alpha_z_2 = shareGen.getShare(0, 2, -1, 0);
          i64 alpha_x_alpha_y_2 = 0;

          for(u64 k = 0; k<shared_x.size(); ++k)
            alpha_x_alpha_y_2 += ((shared_x.mShares[0](k) + shared_x.mShares[1](k))*(shared_y.mShares[0](k) + shared_y.mShares[1](k)));

          i64 alpha_x_alpha_y_1 = shareGen.getShare(0, 1, -1, 0);
          alpha_x_alpha_y_2-=alpha_x_alpha_y_1;
          comm.mPrev.asyncSendCopy(alpha_x_alpha_y_2);

          shared_z[0] = alpha_z_1;
          shared_z[1] = alpha_z_2;

          z.alpha_1 = shared_z[0];
          z.alpha_2 = shared_z[1];
      });

      auto rr = [&](int i) {
        //auto& enc = encs[i];
        auto& comm = comms[i];
        
        si64Matrix shared_x(x.size(), x[0].size()), shared_y(y.size(), y[0].size());
        si64 shared_z;

        get_allocated_share(x, i, shared_x);
        get_allocated_share(y, i, shared_y);
        
        //Preprocessing Phase
        i64 alpha_z_share = shareGen.getShare(0, i, -1, i);
        
        i64 alpha_x_alpha_y_share;
        if(i == 1)
            alpha_x_alpha_y_share = shareGen.getShare(0, 1, -1, 1);
        else if(i == 2)
            comm.mNext.recv(alpha_x_alpha_y_share);

        //Online phase
        i64 beta_z_1, beta_z_2;
        if (i == 1)
        {
            beta_z_1 = (shared_x.mShares[1]*shared_y.mShares[1].transpose())(0) - (shared_x.mShares[1]*shared_y.mShares[0].transpose())(0) - (shared_y.mShares[1]*shared_x.mShares[0].transpose())(0) + alpha_x_alpha_y_share + alpha_z_share;
            comm.mNext.asyncSendCopy(beta_z_1);
            comm.mNext.recv(beta_z_2);
        } 
        else if(i == 2)
        {

            beta_z_2 = 0 - (shared_x.mShares[1]*shared_y.mShares[0].transpose())(0) - (shared_y.mShares[1]*shared_x.mShares[0].transpose())(0) + alpha_x_alpha_y_share + alpha_z_share;
            comm.mPrev.asyncSendCopy(beta_z_2);
            comm.mPrev.recv(beta_z_1);
        }
        shared_z[1] = beta_z_1 + beta_z_2;
        shared_z[0] = alpha_z_share;

        z.beta = shared_z[1];
  	};
  
	  auto t1 = std::thread(rr, 1);
  	auto t2 = std::thread(rr, 2);
  
	  t0.join();
  	t1.join();
  	t2.join();
    return z;

    }
    
    share_value local_add(share_value x, share_value y)
    {
        share_value z;
        z.alpha_1 = x.alpha_1 + y.alpha_1;
        z.alpha_2 = x.alpha_2 + y.alpha_2;
        z.beta = x.beta + y.beta;

        return z;
    }

	};

}

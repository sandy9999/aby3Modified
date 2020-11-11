#include "Sh3Encryptor.h"
#include <libOTe/Tools/Tools.h>

#include "cryptoTools/Common/Log.h"

namespace aby3
{

	//Sh3Task Sh3Encryptor::init(Sh3Runtime& rt)
	//{
	//    auto task = rt.nullTask();
	//    task.then
	//}

	void Sh3Encryptor::complateSharing(CommPkg& comm, span<i64> send, span<i64> recv)
	{
		comm.mNext.asyncSendCopy(send);
		comm.mPrev.recv(recv);
	}

  /*Consider P0, P1, P2 follow the astra sharing protocol. WLOG, let P0 be the distributor, and P1 and P2 the evaluators.
  An astra arithmetic share of x is represented as: [[ beta_x, [alpha_x] ]] where
    P0 holds alpha_x_1 and alpha_x_2
    P1 holds alpha_x_1 and beta_x
    P2 holds alpha_x_2 and beta_x
  */

  //Astra sharing of a value Preprocess Phase begins
	si64 Sh3Encryptor::astra_share_preprocess_distributor(CommPkg& comm, int partyIdx)
	{
    if(partyIdx == 0)
    {
        si64 alpha_x;
        alpha_x[0] = mShareGen.getShare();
        alpha_x[1] = mShareGen.getShare();
        comm.mNext.asyncSendCopy(alpha_x[0]);
        comm.mPrev.asyncSendCopy(alpha_x[1]);
        return alpha_x;
    }
    else if(partyIdx == 1)
    {
        si64 alpha_x;
        alpha_x[0] = mShareGen.getShare();
        alpha_x[1] = mShareGen.getShare();
        comm.mPrev.asyncSendCopy(alpha_x[0]);//alpha_x_1
        comm.mPrev.asyncSendCopy(alpha_x[1]);//alpha_x_2
        comm.mNext.asyncSendCopy(alpha_x[1]);//alpha_x_2
        return alpha_x;
    }
    else if(partyIdx == 2)
    {
        si64 alpha_x;
        alpha_x[0] = mShareGen.getShare();
        alpha_x[1] = mShareGen.getShare();
        comm.mNext.asyncSendCopy(alpha_x[0]);//alpha_x_2
        comm.mNext.asyncSendCopy(alpha_x[1]);//alpha_x_1
        comm.mPrev.asyncSendCopy(alpha_x[1]);//alpha_x_1
        return alpha_x;
    }
	}

  i64 Sh3Encryptor::astra_share_preprocess_evaluator(CommPkg& comm, int partyIdx, int senderPartyIdx)
  {
      if(partyIdx == 1)
      {
          if(senderPartyIdx == 0)
          {
            i64 alpha_x_1;
            comm.mPrev.recv(alpha_x_1);
            return alpha_x_1;
          }
          else if(senderPartyIdx == 2)
          {
            i64 alpha_x_1;
            comm.mNext.recv(alpha_x_1);
            return alpha_x_1;
          }
      }
      else if(partyIdx == 2)
      {
          if(senderPartyIdx == 0)
          {
            i64 alpha_x_2;
            comm.mNext.recv(alpha_x_2);
            return alpha_x_2;
          }
          else if(senderPartyIdx == 1)
          {
            i64 alpha_x_2;
            comm.mPrev.recv(alpha_x_2);
            return alpha_x_2;
          }
      }
  }

  si64 Sh3Encryptor::astra_share_preprocess_evaluator_notP0(CommPkg& comm, int partyIdx, int senderPartyIdx)
  {
      
      if(partyIdx == 0)
      {
        si64 alpha_x;
        if(senderPartyIdx == 1)
        {
            comm.mNext.recv(alpha_x[0]);
            comm.mNext.recv(alpha_x[1]);
        }
        else if(senderPartyIdx == 2)
        {
          comm.mPrev.recv(alpha_x[1]);
          comm.mPrev.recv(alpha_x[0]);
        }
        return alpha_x;
      }
  }
  //Astra sharing of a value Preprocess Phase ends

  //Astra sharing of a value Online Phase begins
  void Sh3Encryptor::astra_share_online_distributor(CommPkg& comm, i64 x, si64 alpha_x, int partyIdx)
  {
    if(partyIdx == 0)
    {
      i64 beta_x = alpha_x[0] + alpha_x[1] + x;
      comm.mNext.asyncSendCopy(beta_x);
      comm.mPrev.asyncSendCopy(beta_x);
    }
  }

  i64 Sh3Encryptor::astra_share_online_distributor_notP0(CommPkg& comm, i64 x, si64 alpha_x, int partyIdx)
  {
    if(partyIdx == 1)
    {
      i64 beta_x = alpha_x[0] + alpha_x[1] + x;
      comm.mNext.asyncSendCopy(beta_x);
      return beta_x;
    }
    else if(partyIdx == 2)
    {
      i64 beta_x = alpha_x[0] + alpha_x[1] + x;
      comm.mPrev.asyncSendCopy(beta_x);
      return beta_x;
    }
  }

  i64 Sh3Encryptor::astra_share_online_evaluator(CommPkg& comm, int partyIdx, int senderPartyIdx)
  {
      i64 beta_x;
      if(partyIdx == 1)
      {
          if(senderPartyIdx == 0)
          {
            comm.mPrev.recv(beta_x);
          }
          else if(senderPartyIdx == 2)
          {
            comm.mNext.recv(beta_x);
          }
      }
      else if(partyIdx == 2)
      {
          if(senderPartyIdx == 0)
          {
            comm.mNext.recv(beta_x);
          }
          else if(senderPartyIdx == 1)
          {
            comm.mPrev.recv(beta_x);
          }
      }
      return beta_x;
  }
  //Astra sharing of a value Online Phase ends

  //Astra sharing of a value Reveal Phase begins
  i64 Sh3Encryptor::astra_share_reveal_receiver(CommPkg& comm, si64 share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (other_share - (share[0] + share[1]));
    }
    else if(partyIdx == 1)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (share[1] - (other_share + share[0]));
    }
    else if(partyIdx == 2)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (share[1] - (other_share + share[0]));
    }
  }

  void Sh3Encryptor::astra_share_reveal_sender(CommPkg& comm, si64 share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      comm.mNext.asyncSendCopy(share[1]);
    }
    else if(partyIdx == 1)
    {
      comm.mNext.asyncSendCopy(share[0]);
    }
    else if(partyIdx == 2)
    {
      comm.mNext.asyncSendCopy(share[1]);
    }
  }
  //Astra sharing of a value Reveal Phase ends

  /*
  An astra boolean share of x is represented as: [[ beta_b, [alpha_b]^B ]]^B where
    P0 holds alpha_b_1 and alpha_b_2
    P1 holds alpha_b_1 and beta_b
    P2 holds alpha_b_2 and beta_b
  */

  //Astra binary sharing of a value Preprocessing Phase begins
	sb64 Sh3Encryptor::astra_binary_share_preprocess_distributor(CommPkg& comm, int partyIdx)
	{
    if(partyIdx == 0)
    {
        sb64 alpha_b;
        alpha_b[0] = (mShareGen.getBinaryShare())&1;
        alpha_b[1] = (mShareGen.getBinaryShare())&1;
        comm.mNext.asyncSendCopy(alpha_b[0]);
        comm.mPrev.asyncSendCopy(alpha_b[1]);
        return alpha_b;
    }
	}
	
  i64 Sh3Encryptor::astra_binary_share_preprocess_evaluator(CommPkg& comm, int partyIdx)
  {
      if(partyIdx == 1)
      {
          i64 alpha_b_1;
          comm.mPrev.recv(alpha_b_1);
          return alpha_b_1;
      }
      else if(partyIdx == 2)
      {
          i64 alpha_b_2;
          comm.mNext.recv(alpha_b_2);
          return alpha_b_2;
      }
  }
  //Astra binary sharing of a value Preprocessing Phase ends

  //Astra binary sharing of a value Online Phase begins
  void Sh3Encryptor::astra_binary_share_online_distributor(CommPkg& comm, i64 x, sb64 alpha_b, int partyIdx)
  {
    if(partyIdx == 0)
    {
      i64 beta_b = alpha_b[0] ^ alpha_b[1] ^ x;
      comm.mNext.asyncSendCopy(beta_b);
      comm.mPrev.asyncSendCopy(beta_b);
    }
  }
	
  i64 Sh3Encryptor::astra_binary_share_online_evaluator(CommPkg& comm, int partyIdx)
  {
      i64 beta_b;
      if(partyIdx == 1)
      {
          comm.mPrev.recv(beta_b);
      }
      else if(partyIdx == 2)
      {
          comm.mNext.recv(beta_b);
      }
      return beta_b;
  }
  //Astra binary sharing of a value Online Phase ends


  //Astra binary sharing of a value Reveal Phase begins
  i64 Sh3Encryptor::astra_binary_share_reveal_receiver(CommPkg& comm, sb64 share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (other_share ^ (share[0] ^ share[1]));
    }
    else if(partyIdx == 1)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (share[1] ^ (other_share ^ share[0]));
    }
    else if(partyIdx == 2)
    {
      i64 other_share;
      comm.mPrev.recv(other_share);
      return (share[1] ^ (other_share ^ share[0]));
    }
  }

  void Sh3Encryptor::astra_binary_share_reveal_sender(CommPkg& comm, sb64 share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      comm.mNext.asyncSendCopy(share[1]);
    }
    else if(partyIdx == 1)
    {
      comm.mNext.asyncSendCopy(share[0]);
    }
    else if(partyIdx == 2)
    {
      comm.mNext.asyncSendCopy(share[1]);
    }
  }
  //Astra binary sharing of a value Reveal Phase ends

  //Astra sharing of a matrix X Preprocess Phase begins
	void Sh3Encryptor::astra_share_matrix_preprocess_distributor(CommPkg& comm, si64Matrix& alpha_X, int partyIdx)
	{
    if(partyIdx == 0)
    {
        
        for(u64 i=0; i< alpha_X.mShares[0].size(); ++i)
        {
            alpha_X.mShares[0](i) = mShareGen.getShare();
            alpha_X.mShares[1](i) = mShareGen.getShare();
        }
        comm.mNext.asyncSendCopy(alpha_X.mShares[0].data(), alpha_X.mShares[0].size());
        comm.mPrev.asyncSendCopy(alpha_X.mShares[1].data(), alpha_X.mShares[1].size());
    }
	}
	
  void Sh3Encryptor::astra_share_matrix_preprocess_evaluator(CommPkg& comm, i64Matrix& alpha_X_share, int partyIdx)
  {
      if(partyIdx == 1)
      {
          comm.mPrev.recv(alpha_X_share.data(), alpha_X_share.size()); //Receives matrix alpha_X_1
      }
      else if(partyIdx == 2)
      {
          comm.mNext.recv(alpha_X_share.data(), alpha_X_share.size()); //Receives matrix alpha_X_2
      }
  }
  //Astra sharing of a matrix X Preprocess Phase ends

  //Astra sharing of a matrix X Online Phase begins
  void Sh3Encryptor::astra_share_matrix_online_distributor(CommPkg& comm, i64Matrix X, si64Matrix alpha_X, int partyIdx)
  {
    if(partyIdx == 0)
    {
      if (alpha_X.cols() != static_cast<u64>(X.cols()) || alpha_X.size() != static_cast<u64>(X.size()))
            throw std::runtime_error(LOCATION);
      i64Matrix beta_X;
      beta_X.resizeLike(X);
      for(u64 i = 0; i<X.size(); ++i)
        beta_X(i) = X(i) + alpha_X.mShares[0](i) + alpha_X.mShares[1](i);
      comm.mNext.asyncSendCopy(beta_X.data(), beta_X.size());
      comm.mPrev.asyncSendCopy(beta_X.data(), beta_X.size());
    }
  }

  void Sh3Encryptor::astra_share_matrix_online_evaluator(CommPkg& comm, i64Matrix& beta_X, int partyIdx)
  {
      if(partyIdx == 1)
      {
          comm.mPrev.recv(beta_X.data(), beta_X.size());
      }
      else if(partyIdx == 2)
      {
          comm.mNext.recv(beta_X.data(), beta_X.size());
      }
  }
  //Astra sharing of a matrix X Online Phase ends

  //Astra sharing of a matrix X Reveal Phase begins
  void Sh3Encryptor::astra_share_matrix_reveal_receiver(CommPkg& comm, si64Matrix share, i64Matrix& actual_matrix, int partyIdx)
  {
    if (actual_matrix.rows() != static_cast<i64>(share.rows()) || actual_matrix.cols() != static_cast<i64>(share.cols()))
            throw std::runtime_error(LOCATION);
    if(partyIdx == 0)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = actual_matrix(i) - (share.mShares[0](i) + share.mShares[1](i));
    }
    else if(partyIdx == 1)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = share.mShares[1](i) - (share.mShares[0](i) + actual_matrix(i));
    }
    else if(partyIdx == 2)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = share.mShares[1](i) - (share.mShares[0](i) + actual_matrix(i));
    }
  }

  void Sh3Encryptor::astra_share_matrix_reveal_sender(CommPkg& comm, si64Matrix share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      comm.mNext.asyncSendCopy(share.mShares[1].data(), share.mShares[1].size());
    }
    else if(partyIdx == 1)
    {
      comm.mNext.asyncSendCopy(share.mShares[0].data(), share.mShares[0].size());
    }
    else if(partyIdx == 2)
    {
      comm.mNext.asyncSendCopy(share.mShares[1].data(), share.mShares[1].size());
    }
  }
  //Astra sharing of a matrix X Reveal Phase ends

  //Astra binary sharing of a matrix B Preprocess Phase begins
	void Sh3Encryptor::astra_binary_share_matrix_preprocess_distributor(CommPkg& comm, sbMatrix& alpha_B, int partyIdx)
	{
    if(partyIdx == 0)
    {
        
        for(u64 i=0; i< alpha_B.mShares[0].size(); ++i)
        {
            alpha_B.mShares[0](i) = (mShareGen.getBinaryShare())&1;
            alpha_B.mShares[1](i) = (mShareGen.getBinaryShare())&1;
        }
        comm.mNext.asyncSendCopy(alpha_B.mShares[0].data(), alpha_B.mShares[0].size());
        comm.mPrev.asyncSendCopy(alpha_B.mShares[1].data(), alpha_B.mShares[1].size());
    }
	}
	
  void Sh3Encryptor::astra_binary_share_matrix_preprocess_evaluator(CommPkg& comm, i64Matrix& alpha_B_share, int partyIdx)
  {
      if(partyIdx == 1)
      {
          comm.mPrev.recv(alpha_B_share.data(), alpha_B_share.size()); //Receives matrix alpha_B_1
      }
      else if(partyIdx == 2)
      {
          comm.mNext.recv(alpha_B_share.data(), alpha_B_share.size()); //Receives matrix alpha_B_2
      }
  }
  //Astra binary sharing of a matrix B Preprocess Phase ends

  //Astra binary sharing of a matrix B Online Phase begins
  void Sh3Encryptor::astra_binary_share_matrix_online_distributor(CommPkg& comm, i64Matrix B, sbMatrix alpha_B, int partyIdx)
  {
    if(partyIdx == 0)
    {
      if (alpha_B.mShares[0].cols() != static_cast<u64>(B.cols()) || alpha_B.mShares[0].size() != static_cast<u64>(B.size()))
            throw std::runtime_error(LOCATION);
      i64Matrix beta_B;
      beta_B.resizeLike(B);
      for(u64 i = 0; i<B.size(); ++i)
        beta_B(i) = B(i) ^ alpha_B.mShares[0](i) ^ alpha_B.mShares[1](i);
      comm.mNext.asyncSendCopy(beta_B.data(), beta_B.size());
      comm.mPrev.asyncSendCopy(beta_B.data(), beta_B.size());
    }
  }

  void Sh3Encryptor::astra_binary_share_matrix_online_evaluator(CommPkg& comm, i64Matrix& beta_B, int partyIdx)
  {
      if(partyIdx == 1)
      {
          comm.mPrev.recv(beta_B.data(), beta_B.size());
      }
      else if(partyIdx == 2)
      {
          comm.mNext.recv(beta_B.data(), beta_B.size());
      }
  }
  //Astra binary sharing of a matrix B Online Phase ends

  //Astra binary sharing of a matrix B Reveal Phase begins
  void Sh3Encryptor::astra_binary_share_matrix_reveal_receiver(CommPkg& comm, sbMatrix share, i64Matrix& actual_matrix, int partyIdx)
  {
    if (actual_matrix.rows() != static_cast<i64>(share.mShares[0].rows()) || actual_matrix.cols() != static_cast<i64>(share.mShares[0].cols()))
            throw std::runtime_error(LOCATION);
    if(partyIdx == 0)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = actual_matrix(i) ^ (share.mShares[0](i) ^ share.mShares[1](i));
    }
    else if(partyIdx == 1)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = share.mShares[1](i) ^ (share.mShares[0](i) ^ actual_matrix(i));
    }
    else if(partyIdx == 2)
    {
      comm.mPrev.recv(actual_matrix.data(), actual_matrix.size());
      for(u64 i=0; i<actual_matrix.size(); ++i)
        actual_matrix(i) = share.mShares[1](i) ^ (share.mShares[0](i) ^ actual_matrix(i));
    }
  }

  void Sh3Encryptor::astra_binary_share_matrix_reveal_sender(CommPkg& comm, si64Matrix share, int partyIdx)
  {
    if(partyIdx == 0)
    {
      comm.mNext.asyncSendCopy(share.mShares[1].data(), share.mShares[1].size());
    }
    else if(partyIdx == 1)
    {
      comm.mNext.asyncSendCopy(share.mShares[0].data(), share.mShares[0].size());
    }
    else if(partyIdx == 2)
    {
      comm.mNext.asyncSendCopy(share.mShares[1].data(), share.mShares[1].size());
    }
  }
  //Astra binary sharing of a matrix B Reveal Phase ends

  //Additive sharing of value x begins
  si64 Sh3Encryptor::astra_additive_share_distributor(CommPkg& comm, i64 x, int partyIdx, bool value_given)
  {
    if(partyIdx == 0)
    {
      si64 shared_x;
      shared_x[0] = mShareGen.getShare();
      if(value_given)
        shared_x[1] = x - shared_x[0];
      else
        shared_x[1] = mShareGen.getShare();
      comm.mNext.asyncSendCopy(shared_x[0]);
      comm.mPrev.asyncSendCopy(shared_x[1]);
      return shared_x;
    }
  }

  i64 Sh3Encryptor::astra_additive_share_evaluator(CommPkg& comm, int partyIdx)
  {
    if(partyIdx == 1)
    {
      i64 x_1;
      comm.mPrev.recv(x_1);
      return x_1;
    }
    else if(partyIdx == 2)
    {
      i64 x_2;
      comm.mNext.recv(x_2);
      return x_2;
    }
  }
  //Additive sharing of value x ends

  //Additive sharing of matrix X begins
  void Sh3Encryptor::astra_additive_share_matrix_distributor(CommPkg& comm, i64Matrix X, si64Matrix& shared_X, int partyIdx, bool matrix_given)
  {
    if(partyIdx == 0)
    {
      for(u64 i = 0; i<shared_X.size(); ++i)
      {
        shared_X.mShares[0](i) = mShareGen.getShare();
        if(matrix_given)
          shared_X.mShares[1](i) = X(i) - shared_X.mShares[0](i);
        else
          shared_X.mShares[1](i) = mShareGen.getShare();
      }
      comm.mNext.asyncSendCopy(shared_X.mShares[0].data(), shared_X.mShares[0].size());
      comm.mPrev.asyncSendCopy(shared_X.mShares[1].data(), shared_X.mShares[1].size());
    }
  }

  void Sh3Encryptor::astra_additive_share_matrix_evaluator(CommPkg& comm, int partyIdx, i64Matrix& X_share)
  {
    if(partyIdx == 1)
    {
      comm.mPrev.recv(X_share.data(), X_share.size());
    }
    else if(partyIdx == 2)
    {
      comm.mNext.recv(X_share.data(), X_share.size());
    }
  }
  //Additive sharing of matrix X ends


  //Binary Additive sharing of value b begins
  sb64 Sh3Encryptor::astra_binary_additive_share_distributor(CommPkg& comm, i64 b, int partyIdx, bool value_given)
  {
    if(partyIdx == 0)
    {
      sb64 shared_b;
      shared_b[0] = (mShareGen.getBinaryShare())&1;
      if(value_given)
        shared_b[1] = b ^ shared_b[0];
      else
        shared_b[1] = (mShareGen.getBinaryShare())&1;
      comm.mNext.asyncSendCopy(shared_b[0]);
      comm.mPrev.asyncSendCopy(shared_b[1]);
      return shared_b;
    }
  }

  i64 Sh3Encryptor::astra_binary_additive_share_evaluator(CommPkg& comm, int partyIdx)
  {
    if(partyIdx == 1)
    {
      i64 b_1;
      comm.mPrev.recv(b_1);
      return b_1;
    }
    else if(partyIdx == 2)
    {
      i64 b_2;
      comm.mNext.recv(b_2);
      return b_2;
    }
  }
  //Binary Additive sharing of value b ends

  //Binary Additive sharing of matrix B begins
  void Sh3Encryptor::astra_binary_additive_share_matrix_distributor(CommPkg& comm, i64Matrix B, sbMatrix& shared_B, int partyIdx, bool matrix_given)
  {
    if(partyIdx == 0)
    {
      for(u64 i = 0; i<B.size(); ++i)
      {
        shared_B.mShares[0](i) = (mShareGen.getBinaryShare())&1;
        if(matrix_given)
          shared_B.mShares[1](i) = B(i) ^ shared_B.mShares[0](i);
        else
          shared_B.mShares[1](i) = (mShareGen.getBinaryShare())&1;
      }
      comm.mNext.asyncSendCopy(shared_B.mShares[0].data(), shared_B.mShares[0].size());
      comm.mPrev.asyncSendCopy(shared_B.mShares[1].data(), shared_B.mShares[1].size());
    }
  }

  void Sh3Encryptor::astra_binary_additive_share_matrix_evaluator(CommPkg& comm, int partyIdx, i64Matrix& B_share)
  {
    if(partyIdx == 1)
    {
      comm.mPrev.recv(B_share.data(), B_share.size());
    }
    else if(partyIdx == 2)
    {
      comm.mNext.recv(B_share.data(), B_share.size());
    }
  }
  //Additive sharing of matrix X ends
    /*si64 Sh3Encryptor::astra_bit2a_online_0(CommPkg& comm)
    {
        si64 ret;
        comm.mPrev.recv(ret.mData[0]);
        comm.mNext.recv(ret.mData[1]);
    }

    si64 Sh3Encryptor::astra_bit2a_online(CommPkg& comm, i64 val, int partyIdx)
    {
       si64 ret;
       if(partyIdx == 1)
       {
          ret.mData[0] = mShareGen.getShare();
          ret.mData[1] = ret.mData[0] + val;
          comm.mPrev.asyncSendCopy(ret.mData[0]);
       }
       else if(partyIdx == 2)
       {
          ret.mData[0] = mShareGen.getShare();
          ret.mData[1] = ret.mData[0] + val;
          comm.mNext.asyncSendCopy(ret.mData[0]);
       }
       return ret;
    }*/

	si64 Sh3Encryptor::localInt(CommPkg & comm, i64 val)
	{
		si64 ret;
		ret[0] = mShareGen.getShare() + val;


		comm.mNext.asyncSendCopy(ret[0]);
		comm.mPrev.recv(ret[1]);
		oc::lout<<"Next party: "<<comm.mNext<<" Previous party: "<<comm.mPrev<<" val: "<<val<<" ret[0]: "<<ret[0]<<" ret[1]: "<<ret[1]<<std::endl;
		return ret;
	}

	si64 Sh3Encryptor::remoteInt(CommPkg & comm)
	{
		return localInt(comm, 0);
	}

	Sh3Task Sh3Encryptor::localInt(Sh3Task dep, i64 val, si64 & dest)
	{
		return dep.then([this, val, &dest](CommPkg& comm, Sh3Task& self) {

				//si64 ret;
				dest[0] = mShareGen.getShare() + val;

				comm.mNext.asyncSendCopy(dest[0]);
				auto fu = comm.mPrev.asyncRecv(dest[1]);

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
				}).getClosure();
	}

	Sh3Task Sh3Encryptor::remoteInt(Sh3Task dep, si64 & dest)
	{
		return localInt(dep, 0, dest);
	}

	sb64 Sh3Encryptor::localBinary(CommPkg & comm, i64 val)
	{
		sb64 ret;
		ret[0] = mShareGen.getBinaryShare() ^ val;

		comm.mNext.asyncSendCopy(ret[0]);
		comm.mPrev.recv(ret[1]);

		return ret;
	}

	sb64 Sh3Encryptor::remoteBinary(CommPkg & comm)
	{
		return localBinary(comm, 0);
	}

	Sh3Task Sh3Encryptor::localBinary(Sh3Task dep, i64 val, sb64 & ret)
	{
		return dep.then([this, val, &ret](CommPkg& comm, Sh3Task& self) {
				ret[0] = mShareGen.getBinaryShare() ^ val;

				comm.mNext.asyncSendCopy(ret[0]);
				auto fu = comm.mPrev.asyncRecv(ret[1]);

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});

				}).getClosure();
	}

	Sh3Task Sh3Encryptor::remoteBinary(Sh3Task dep, sb64 & dest)
	{
		return localBinary(dep, 0, dest);
	}

	void Sh3Encryptor::localIntMatrix(CommPkg & comm, const i64Matrix & m, si64Matrix & ret)
	{
		if (ret.cols() != static_cast<u64>(m.cols()) ||
				ret.size() != static_cast<u64>(m.size()))
			throw std::runtime_error(LOCATION);
		for (i64 i = 0; i < ret.mShares[0].size(); ++i)
			ret.mShares[0](i) = mShareGen.getShare() + m(i);

		comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
		comm.mPrev.recv(ret.mShares[1].data(), ret.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::localIntMatrix(Sh3Task dep, const i64Matrix & m, si64Matrix & ret)
	{

		return dep.then([this, &m, &ret](CommPkg& comm, Sh3Task& self) {

				//oc::lout << self.mRuntime->mPartyIdx << " localIntMatrix" << std::endl;

				if (ret.cols() != static_cast<u64>(m.cols()) ||
						ret.size() != static_cast<u64>(m.size()))
				throw std::runtime_error(LOCATION);
				//oc::lout<<"no. of rows in m: "<<m.size()<<" no. of columns in m: "<<m.cols()<<std::endl;

				/*for(i64 i=0; i<m.size(); i++)
				  {
				  for(i64 j=0; j<m.cols(); j++)
				  oc::lout<<"m ["<<i<<"]["<<j<<"]: "<<m(i, j)<<std::endl;
				  oc::lout<<std::endl;
				  oc::lout<<std::endl;
				  }*/
				for (i64 i = 0; i < ret.mShares[0].size(); ++i)
				ret.mShares[0](i) = mShareGen.getShare() + m(i);

				comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(ret.mShares[1].data(), ret.mShares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self)mutable{
						//oc::lout << self.mRuntime->mPartyIdx << " localIntMatrix 2" << std::endl;
						fu.get();
						});
		}).getClosure();

	}

	void Sh3Encryptor::remoteIntMatrix(CommPkg & comm, si64Matrix & ret)
	{

		for (i64 i = 0; i < ret.mShares[0].size(); ++i)
			ret.mShares[0](i) = mShareGen.getShare();

		comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
		comm.mPrev.recv(ret.mShares[1].data(), ret.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::remoteIntMatrix(Sh3Task dep, si64Matrix & ret)
	{
		return dep.then([this, &ret](CommPkg& comm, Sh3Task& self) {
				//oc::lout << self.mRuntime->mPartyIdx << " remoteIntMatrix 1" << std::endl;

				for (i64 i = 0; i < ret.mShares[0].size(); ++i)
				ret.mShares[0](i) = mShareGen.getShare();

				comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(ret.mShares[1].data(), ret.mShares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						//oc::lout << self.mRuntime->mPartyIdx << " remoteIntMatrix 2" << std::endl;
						fu.get();
						});
				}).getClosure();
	}


	void Sh3Encryptor::localBinMatrix(CommPkg & comm, const i64Matrix & m, sbMatrix & ret)
	{
		auto b0 = ret.i64Cols() != static_cast<u64>(m.cols());
		auto b1 = ret.i64Size() != static_cast<u64>(m.size());
		if (b0 || b1)
			throw std::runtime_error(LOCATION);

		for (u64 i = 0; i < ret.mShares[0].size(); ++i)
			ret.mShares[0](i) = mShareGen.getBinaryShare() ^ m(i);

		comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
		comm.mPrev.recv(ret.mShares[1].data(), ret.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::localBinMatrix(Sh3Task dep, const i64Matrix & m, sbMatrix & ret)
	{
		return dep.then([this, &m, &ret](CommPkg& comm, Sh3Task self) {

				auto b0 = ret.i64Cols() != static_cast<u64>(m.cols());
				auto b1 = ret.i64Size() != static_cast<u64>(m.size());
				if (b0 || b1)
				throw std::runtime_error(LOCATION);

				for (u64 i = 0; i < ret.mShares[0].size(); ++i)
				ret.mShares[0](i) = mShareGen.getBinaryShare() ^ m(i);

				comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(ret.mShares[1].data(), ret.mShares[1].size());

				self.then([fu = std::move(fu)](CommPkg&, Sh3Task& self) mutable {
						fu.get();
						});
				}).getClosure();
	}

	void Sh3Encryptor::remoteBinMatrix(CommPkg & comm, sbMatrix & ret)
	{
		for (u64 i = 0; i < ret.mShares[0].size(); ++i)
			ret.mShares[0](i) = mShareGen.getBinaryShare();

		comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
		comm.mPrev.recv(ret.mShares[1].data(), ret.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::remoteBinMatrix(Sh3Task dep, sbMatrix & ret)
	{
		return dep.then([this, &ret](CommPkg& comm, Sh3Task& self) mutable {

				for (u64 i = 0; i < ret.mShares[0].size(); ++i)
				ret.mShares[0](i) = mShareGen.getBinaryShare();

				comm.mNext.asyncSendCopy(ret.mShares[0].data(), ret.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(ret.mShares[1].data(), ret.mShares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
				}).getClosure();
	}

	void Sh3Encryptor::localPackedBinary(CommPkg & comm, const i64Matrix& m, sPackedBin & dest)
	{
		if (dest.bitCount() != m.cols() * sizeof(i64) * 8)
			throw std::runtime_error(LOCATION);
		if (dest.shareCount() != static_cast<u64>(m.rows()))
			throw std::runtime_error(LOCATION);

		auto bits = sizeof(i64) * 8;
		auto outRows = dest.bitCount();
		auto outCols = (dest.shareCount() + bits - 1) / bits;
		oc::MatrixView<u8> in((u8*)m.data(), m.rows(), m.cols() * sizeof(i64));
		oc::MatrixView<u8> out((u8*)dest.mShares[0].data(), outRows, outCols * sizeof(i64));
		oc::transpose(in, out);

		for (u64 i = 0; i < dest.mShares[0].size(); ++i)
			dest.mShares[0](i) = dest.mShares[0](i) ^ mShareGen.getBinaryShare();

		comm.mNext.asyncSendCopy(dest.mShares[0].data(), dest.mShares[0].size());
		comm.mPrev.recv(dest.mShares[1].data(), dest.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::localPackedBinary(Sh3Task dep, const i64Matrix & m, sPackedBin & dest)
	{

		oc::MatrixView<u8> mm((u8*)m.data(), m.rows(), m.cols() * sizeof(i64));
		return localPackedBinary(dep, mm, dest, true);
	}

	Sh3Task Sh3Encryptor::localPackedBinary(Sh3Task dep, oc::MatrixView<u8> m, sPackedBin & dest, bool transpose)
	{
		return dep.then([this, m, &dest, transpose](CommPkg& comm, Sh3Task& self) {

				if (dest.bitCount() != m.cols() * 8)
				throw std::runtime_error(LOCATION);
				if (dest.shareCount() != m.rows())
				throw std::runtime_error(LOCATION);

				auto bits = sizeof(i64) * 8;
				auto outRows = dest.bitCount();
				auto outCols = (dest.shareCount() + bits - 1) / bits;
				oc::MatrixView<u8> out((u8*)dest.mShares[0].data(), outRows, outCols * sizeof(i64));

				if (transpose)
				oc::transpose(m, out);
				else
				memcpy(out.data(), m.data(), m.size());

				for (u64 i = 0; i < dest.mShares[0].size(); ++i)
				dest.mShares[0](i) = dest.mShares[0](i) ^ ((mShareGen.getBinaryShare())&1);

				comm.mNext.asyncSendCopy(dest.mShares[0].data(), dest.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(dest.mShares[1].data(), dest.mShares[1].size());

				self.then([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
						fu.get();
						});
		}).getClosure();
	}

	void Sh3Encryptor::remotePackedBinary(CommPkg & comm, sPackedBin & dest)
	{
		for (u64 i = 0; i < dest.mShares[0].size(); ++i)
			dest.mShares[0](i) = (mShareGen.getBinaryShare())&1;

		comm.mNext.asyncSendCopy(dest.mShares[0].data(), dest.mShares[0].size());
		comm.mPrev.recv(dest.mShares[1].data(), dest.mShares[1].size());
	}

	Sh3Task Sh3Encryptor::remotePackedBinary(Sh3Task dep, sPackedBin & dest)
	{
		return dep.then([this, &dest](CommPkg& comm, Sh3Task& self)
				{

				for (u64 i = 0; i < dest.mShares[0].size(); ++i)
				dest.mShares[0](i) = (mShareGen.getBinaryShare())&1;

				comm.mNext.asyncSendCopy(dest.mShares[0].data(), dest.mShares[0].size());
				auto fu = comm.mPrev.asyncRecv(dest.mShares[1].data(), dest.mShares[1].size());

				self.then(std::move([fu = std::move(fu)](CommPkg& comm, Sh3Task& self) mutable {
							fu.get();
							}));
				}).getClosure();
	}

	i64 Sh3Encryptor::reveal(CommPkg & comm, const si64 & x)
	{
		i64 s;
		comm.mNext.recv(s);
		std::cout<<"s: "<<s<<" x[0]: "<<x[0]<<" x[1]: "<<x[1]<<" Revealed val: "<<s+x[0]+x[1]<<std::endl;
		return s + x[0] + x[1];
	}

	i64 Sh3Encryptor::revealAll(CommPkg & comm, const si64 & x)
	{
		reveal(comm, (mPartyIdx + 2) % 3, x);
		return reveal(comm, x);
	}

	void Sh3Encryptor::reveal(CommPkg & comm, u64 partyIdx, const si64 & x)
	{
		auto p = ((mPartyIdx + 2)) % 3;
		if (p == partyIdx)
			comm.mPrev.asyncSendCopy(x[0]);
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task  dep, const si64 & x, i64 & dest)
	{
		return dep.then([&x, &dest](CommPkg& comm, Sh3Task& self) {
				comm.mNext.recv(dest);
				dest += x[0] + x[1];
				});
	}


	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const si64& x, i64& dest)
	{
		reveal(dep, (mPartyIdx + 2) % 3, x);
		return reveal(dep, x, dest);
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const si64& x)
	{
		//TODO("decide if we can move the if outside the call to then(...)");
		bool send = ((mPartyIdx + 2) % 3) == partyIdx;
		return dep.then([send, &x](CommPkg& comm, Sh3Task&) {
				if (send)
				comm.mPrev.asyncSendCopy(x[0]);
				});
	}


	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sb64& x, i64& dest)
	{
		return dep.then([&x, &dest](CommPkg& comm, Sh3Task& self) {
				comm.mNext.recv(dest);
				dest ^= x[0] ^ x[1];
				});
	}

	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sb64& x, i64& dest)
	{
		reveal(dep, (mPartyIdx + 2) % 3, x);
		return reveal(dep, x, dest);
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const sb64& x)
	{
		//TODO("decide if we can move the if outside the call to then(...)");
		bool send = ((mPartyIdx + 2) % 3) == partyIdx;
		return dep.then([send, &x](CommPkg& comm, Sh3Task&) {
				if (send)
				comm.mPrev.asyncSendCopy(x[0]);
				});
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const si64Matrix& x, i64Matrix& dest)
	{
		return dep.then([&x, &dest](CommPkg& comm, Sh3Task& self) {
				//oc::lout << self.mRuntime->mPartyIdx << " reveal recv" << std::endl;
				dest.resize(x.rows(), x.cols());
				comm.mNext.recv(dest.data(), dest.size());
				dest += x.mShares[0];
				dest += x.mShares[1];
				});
	}

	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const si64Matrix& x, i64Matrix& dest)
	{
		reveal(dep, (mPartyIdx + 2) % 3, x);
		return reveal(dep, x, dest);
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const si64Matrix& x)
	{
		//TODO("decide if we can move the if outside the call to then(...)");
		bool send = ((mPartyIdx + 2) % 3) == partyIdx;
		return dep.then([send, &x](CommPkg& comm, Sh3Task& self) {

				//oc::lout << self.mRuntime->mPartyIdx << " reveal Send" << std::endl;

				if (send)
				comm.mPrev.asyncSendCopy(x.mShares[0].data(), x.mShares[0].size());
				});
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sbMatrix& x, i64Matrix& dest)
	{
		return dep.then([&x, &dest](CommPkg& comm, Sh3Task& self) {
				comm.mNext.recv(dest.data(), dest.size());
				for (i32 i = 0; i < dest.size(); ++i)
				{
				dest(i) ^= x.mShares[0](i);
				dest(i) ^= x.mShares[1](i);
				}
				});
	}
	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sbMatrix& x, i64Matrix& dest)
	{
		reveal(dep, (mPartyIdx + 2) % 3, x);
		return reveal(dep, x, dest);
	}
	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const sbMatrix& x)
	{
		//TODO("decide if we can move the if outside the call to then(...)");
		bool send = ((mPartyIdx + 2) % 3) == partyIdx;
		return dep.then([send, &x](CommPkg& comm, Sh3Task& self) {
				if (send)
				comm.mPrev.asyncSendCopy(x.mShares[0].data(), x.mShares[0].size());
				});
	}








	i64 Sh3Encryptor::reveal(CommPkg & comm, const sb64 & x)
	{
		i64 s;
		comm.mNext.recv(s);
		return s ^ x[0] ^ x[1];
	}

	i64 Sh3Encryptor::revealAll(CommPkg & comm, const sb64 & x)
	{
		reveal(comm, (mPartyIdx + 2) % 3, x);
		return reveal(comm, x);
	}

	void Sh3Encryptor::reveal(CommPkg & comm, u64 partyIdx, const sb64 & x)
	{
		if ((mPartyIdx + 2) % 3 == partyIdx)
			comm.mPrev.asyncSendCopy(x[0]);
	}

	void Sh3Encryptor::reveal(CommPkg & comm, const si64Matrix & x, i64Matrix & dest)
	{
		if (dest.rows() != static_cast<i64>(x.rows()) || dest.cols() != static_cast<i64>(x.cols()))
			throw std::runtime_error(LOCATION);

		comm.mNext.recv(dest.data(), dest.size());
		for (i64 i = 0; i < dest.size(); ++i)
		{
			dest(i) += x.mShares[0](i) + x.mShares[1](i);
		}
	}

	void Sh3Encryptor::revealAll(CommPkg & comm, const si64Matrix & x, i64Matrix & dest)
	{
		reveal(comm, (mPartyIdx + 2) % 3, x);
		reveal(comm, x, dest);
	}

	void Sh3Encryptor::reveal(CommPkg & comm, u64 partyIdx, const si64Matrix & x)
	{
		if ((mPartyIdx + 2) % 3 == partyIdx)
			comm.mPrev.asyncSendCopy(x.mShares[0].data(), x.mShares[0].size());
	}

	void Sh3Encryptor::reveal(CommPkg & comm, const sbMatrix & x, i64Matrix & dest)
	{
		if (dest.rows() != static_cast<i64>(x.rows()) || dest.cols() != static_cast<i64>(x.i64Cols()))
			throw std::runtime_error(LOCATION);

		comm.mNext.recv(dest.data(), dest.size());
		for (i64 i = 0; i < dest.size(); ++i)
		{
			dest(i) ^= x.mShares[0](i) ^ x.mShares[1](i);
		}
	}

	void Sh3Encryptor::revealAll(CommPkg & comm, const sbMatrix & x, i64Matrix & dest)
	{
		reveal(comm, (mPartyIdx + 2) % 3, x);
		reveal(comm, x, dest);
	}

	void Sh3Encryptor::reveal(CommPkg & comm, u64 partyIdx, const sbMatrix & x)
	{
		if ((mPartyIdx + 2) % 3 == partyIdx)
			comm.mPrev.asyncSendCopy(x.mShares[0].data(), x.mShares[0].size());
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sPackedBin& A, i64Matrix& r)
	{
		return dep.then([&A, &r](CommPkg& comm, Sh3Task&  self)
				{
				auto wordWidth = (A.bitCount() + 8 * sizeof(i64) - 1) / (8 * sizeof(i64));
				i64Matrix buff;
				buff.resize(A.bitCount(), A.simdWidth());
				r.resize(A.mShareCount, wordWidth);

				comm.mNext.recv(buff.data(), buff.size());


				for (i64 i = 0; i < buff.size(); ++i)
				{
				buff(i) ^= A.mShares[0](i);
				buff(i) ^= A.mShares[1](i);
				}

				oc::MatrixView<u8> bb((u8*)buff.data(), A.bitCount(), A.simdWidth() * sizeof(i64));
				oc::MatrixView<u8> rr((u8*)r.data(), r.rows(), r.cols() * sizeof(i64));
				memset(r.data(), 0, r.size() * sizeof(i64));
				transpose(bb, rr);
				});
	}
	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sPackedBin& A, i64Matrix& r)
	{

		reveal(dep, (mPartyIdx + 2) % 3, A);
		return reveal(dep, A, r);
	}

	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, u64 partyIdx, const sPackedBin& A)
	{
		//TODO("decide if we can move the if outside the call to then(...)"); 
		bool send = (mPartyIdx + 2) % 3 == partyIdx;
		return dep.then([send, &A](CommPkg& comm, Sh3Task& self) {
				if (send)
				comm.mPrev.asyncSendCopy(A.mShares[0].data(), A.mShares[0].size());
				});
	}



	void Sh3Encryptor::reveal(CommPkg & comm, const sPackedBin & A, i64Matrix & r)
	{
		auto wordWidth = (A.bitCount() + 8 * sizeof(i64) - 1) / (8 * sizeof(i64));
		i64Matrix buff;
		buff.resize(A.bitCount(), A.simdWidth());
		r.resize(A.mShareCount, wordWidth);

		comm.mNext.recv(buff.data(), buff.size());


		for (i64 i = 0; i < buff.size(); ++i)
		{
			buff(i) = buff(i) ^ A.mShares[0](i) ^ A.mShares[1](i);
		}

		oc::MatrixView<u8> bb((u8*)buff.data(), A.bitCount(), A.simdWidth() * sizeof(i64));
		oc::MatrixView<u8> rr((u8*)r.data(), r.rows(), r.cols() * sizeof(i64));
		memset(r.data(), 0, r.size() * sizeof(i64));
		transpose(bb, rr);
	}

	Sh3Task Sh3Encryptor::revealAll(Sh3Task dep, const sPackedBin& A, PackedBin& r)
	{

		reveal(dep, (mPartyIdx + 2) % 3, A);
		return reveal(dep, A, r);
	}


	Sh3Task Sh3Encryptor::reveal(Sh3Task dep, const sPackedBin & A, PackedBin & r)
	{
		return dep.then([&A, &r](CommPkg& comm, Sh3Task&  self)
				{
				r.resize(A.mShareCount, A.bitCount());

				comm.mNext.recv(r.mData.data(), r.mData.size());


				for (u64 i = 0; i < r.size(); ++i)
				{
				r.mData(i) = r.mData(i) ^ A.mShares[0](i) ^ A.mShares[1](i);
				}
				});
	}

	void Sh3Encryptor::revealAll(CommPkg & comm, const sPackedBin & A, i64Matrix & r)
	{
		reveal(comm, (mPartyIdx + 2) % 3, A);
		reveal(comm, A, r);
	}
	void Sh3Encryptor::reveal(CommPkg & comm, u64 partyIdx, const sPackedBin & A)
	{
		if ((mPartyIdx + 2) % 3 == partyIdx)
			comm.mPrev.asyncSendCopy(A.mShares[0].data(), A.mShares[0].size());
	}

	void Sh3Encryptor::rand(si64Matrix & dest)
	{
		for (u64 i = 0; i < dest.size(); ++i)
		{
			auto s = mShareGen.getRandIntShare();
			dest.mShares[0](i) = s[0];
			dest.mShares[1](i) = s[1];
		}
	}

	void Sh3Encryptor::rand(sbMatrix & dest)
	{
		for (u64 i = 0; i < dest.i64Size(); ++i)
		{
			auto s = mShareGen.getRandBinaryShare();
			dest.mShares[0](i) = s[0];
			dest.mShares[1](i) = s[1];
		}
	}

	void Sh3Encryptor::rand(sPackedBin & dest)
	{
		for (u64 i = 0; i < dest.mShares[0].size(); ++i)
		{
			auto s = mShareGen.getRandBinaryShare();
			dest.mShares[0](i) = s[0];
			dest.mShares[1](i) = s[1];
		}
	}


}


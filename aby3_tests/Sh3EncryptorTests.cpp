#include "Sh3EncryptorTests.h"
#include "aby3/sh3/Sh3Encryptor.h"
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Common/BitVector.h>

using namespace aby3;
using namespace oc;

i64 extract(const sPackedBin& A, u64 share, u64 packIdx, u64 wordIdx)
{

	i64 v = 0;
	BitIterator iter((u8*)&v, 0);

	u64 bitIdx = 64 * wordIdx;
	u64 offset = packIdx / 64;
	u64 mask = 1ull << (packIdx % 64);
	for (u64 i = 0; i < 64; ++i)
	{
		*iter = gsl::narrow<u8>(A.mShares[share](bitIdx, offset) & mask);
		++bitIdx;
		++iter;
	}

	return v;
}

void Sh3_Encryptor_IO_test()
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();


	int trials = 10;
	CommPkg comm[3];
	comm[0] = { chl02, chl01 };
	comm[1] = { chl10, chl12 };
	comm[2] = { chl21, chl20 };

	Sh3Encryptor enc[3];
	enc[0].init(0, toBlock(0, 0), toBlock(1, 1));
	enc[1].init(1, toBlock(1, 1), toBlock(2, 2));
	enc[2].init(2, toBlock(2, 2), toBlock(0, 0));

	bool failed = false;
	auto t0 = std::thread([&]() {
			auto i = 0;
			auto& e = enc[i];
			auto& c = comm[i];
			PRNG prng(ZeroBlock);

			i64 s = 0, x = 0;
			std::vector<i64> vals(trials);
			std::vector<si64> shrs(trials);
			std::vector<sb64> bshrs(trials);
			si64 sum{ { 0,0 } };
			sb64 XOR{ {0,0} };
			ostreamLock(std::cout)<<"Declarations of variables ends here\n";
			for (int i = 0; i < trials; ++i)
			{
			vals[i] = i * 325143121;
			shrs[i] = e.localInt(c, vals[i]);
			ostreamLock(std::cout)<<"Next party: "<<c.mNext<<" Previous party: "<<c.mPrev<<" Value: "<<vals[i]<<" Corresponding share 0: "<<shrs[i].mData[0]<<" Corresponding share 1: "<<shrs[i].mData[1]<<std::endl;
			bshrs[i] = e.localBinary(c, vals[i]);
			sum = sum + shrs[i];
			XOR = XOR ^ bshrs[i];
			s += vals[i];
			x ^= vals[i];
			}

			for (int i = 0; i < trials; ++i)
			{
				std::array<i64, 2> v;
				v[0] = e.reveal(c, shrs[i]);
				v[1] = e.reveal(c, bshrs[i]);
				for (u64 j = 0; j < v.size(); ++j)
				{
					if (v[j] != vals[i]) {
						std::cout << "localInt[" << i << ", "<<j<<"] " << v[j] << " " << vals[i] << " failed" << std::endl;
						failed = true;
					}
				}
			}

			auto v1 = e.reveal(c, sum);
			auto v2 = e.reveal(c, XOR);
			if (v1 != s)
			{
				std::cout << "sum" << std::endl;
				failed = true;
			}

			if (v2 != x)
			{
				std::cout << "xor" << std::endl;
				failed = true;
			}


			i64Matrix m(trials, trials), mm(trials, trials);
			for (u64 i = 0; i < m.size(); ++i)
				m(i) = i;

			si64Matrix mShr(trials, trials);
			e.localIntMatrix(c, m, mShr);
			e.reveal(c, mShr, mm);

			if (mm != m)
			{
				std::cout << "int mtx" << std::endl;
				failed = true;
			}

			sbMatrix bShr(trials, trials * 64);
			e.localBinMatrix(c, m, bShr);
			e.reveal(c, bShr, mm);

			if (mm != m)
			{
				std::cout << "bin mtx" << std::endl;
				failed = true;
			}

			//m.setZero();

			sPackedBin pShr(trials, trials * sizeof(i64) * 8);// , pShr1(trials, trials * sizeof(i64) * 8), pShr2(trials, trials * sizeof(i64) * 8);
			e.localPackedBinary(c, m, pShr);
			e.reveal(c, pShr, mm);

			//c.mNext.recv(pShr1.mShares[0].data(), pShr1.mShares[0].size());
			//c.mNext.recv(pShr1.mShares[1].data(), pShr1.mShares[1].size());
			//c.mPrev.recv(pShr2.mShares[0].data(), pShr2.mShares[0].size());
			//c.mPrev.recv(pShr2.mShares[1].data(), pShr2.mShares[1].size());

			//pShr = pShr ^ pShr1;
			//pShr = pShr ^ pShr2;

			if (mm != m)
			{
				std::cout << "pac mtx" << std::endl;
				std::cout << m << std::endl;
				std::cout << mm << std::endl;


				//for (u64 i = 0; i < 64; ++i)
				//{
				//    auto v = extract(pShr, 0, i, 0);
				//    std::cout<< "s["<<i<<"] " << std::hex << v << std::endl;

				//}

				failed = true;
			}

	});


	auto rr = [&](int i)
	{
		auto& e = enc[i];
		auto& c = comm[i];
		std::vector<si64> shrs(trials);
		std::vector<sb64> bshrs(trials);
		si64 sum{ { 0,0 } };
		sb64 XOR{ { 0,0 } };

		for (int i = 0; i < trials; ++i)
		{
			shrs[i] = e.remoteInt(c);
			bshrs[i] = e.remoteBinary(c);
			sum = sum + shrs[i];
			XOR = XOR ^ bshrs[i];
		}

		for (int i = 0; i < trials; ++i)
		{
			e.reveal(c, 0, shrs[i]);
			e.reveal(c, 0, bshrs[i]);
		}
		e.reveal(c, 0, sum);
		e.reveal(c, 0, XOR);




		si64Matrix mShr(trials, trials);
		e.remoteIntMatrix(c, mShr);
		e.reveal(c, 0, mShr);

		sbMatrix bShr(trials, trials * 64);
		e.remoteBinMatrix(c, bShr);
		e.reveal(c, 0, bShr);

		sPackedBin pShr(trials, trials * sizeof(i64) * 8);
		e.remotePackedBinary(c, pShr);
		e.reveal(c, 0, pShr);

		//Channel chl;
		//if (i == 1)
		//    chl = c.mPrev;
		//else
		//    chl = c.mNext;

		//chl.asyncSend(pShr.mShares[0].data(), pShr.mShares[0].size());
		//chl.asyncSend(pShr.mShares[1].data(), pShr.mShares[1].size());
	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();

	if (failed)
		throw std::runtime_error(LOCATION);
}


void Sh3_Encryptor_asyncIO_test()
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();


	int trials = 1;
	CommPkg comm[3];
	comm[0] = { chl02, chl01 };
	comm[1] = { chl10, chl12 };
	comm[2] = { chl21, chl20 };

	Sh3Encryptor enc[3];
	enc[0].init(0, toBlock(0, 0), toBlock(1, 1));
	enc[1].init(1, toBlock(1, 1), toBlock(2, 2));
	enc[2].init(2, toBlock(2, 2), toBlock(0, 0));

	bool failed = false;
	auto t0 = std::thread([&]() {
			setThreadName("t_0");
			auto i = 0;
			auto& e = enc[i];
			//auto& c = comm[i];
			Sh3Runtime rt(i, comm[i]);
			PRNG prng(ZeroBlock);

			i64 s = 0, x = 0;
			std::vector<i64> vals(trials);
			std::vector<si64> shrs(trials);
			std::vector<sb64> bshrs(trials);
			std::vector<std::array<Sh3Task,2>> tasks(trials);
			si64 sum{ { 0,0 } };
			sb64 XOR{ { 0,0 } };


			e.localInt(rt.noDependencies(), vals[0], shrs[0]).get();

			auto tt = rt.noDependencies();
			for (int i = 0; i < trials; ++i)
			{
				vals[i] = i * 325143121;
				shrs[i] = { {0,0} };
				tasks[i][0] = e.localInt(rt.noDependencies(), vals[i], shrs[i]).then([&sum, &shrs, i](Sh3Task& _) mutable {
						//std::cout <<i<< " (" << shrs[i][0] <<", "<<shrs[i][1] <<")" << std::endl;
						sum = sum + shrs[i];
						});

				tasks[i][1] = e.localBinary(rt.noDependencies(), vals[i], bshrs[i]).then([&XOR, &bshrs, i](Sh3Task& _) mutable {
						XOR = XOR ^ bshrs[i];
						});

				s += vals[i];
				x ^= vals[i];

				tt = tt && tasks[i][0] && tasks[i][1];
			}


			for (int i = 0; i < trials; ++i)
			{
				std::array<i64, 2> v;
				tasks[i][0] = e.reveal(tasks[i][0], shrs[i], v[0]);
				tasks[i][1] = e.reveal(tasks[i][1], bshrs[i], v[1]);

				for (u64 j = 0; j < v.size(); ++j)
				{
					tasks[i][j].get();
					if (v[j] != vals[i]) {
						std::cout << "localInt[" << i << ", " << j << "] " << v[j] << " " << vals[i] << " failed" << std::endl;
						failed = true;
					}
				}
			}

			i64 v1, v2;
			e.reveal(tt, sum, v1).get();
			e.reveal(tt, XOR, v2).get();

			if (v1 != s)
			{
				std::cout << "sum" << std::endl;
				failed = true;
			}

			if (v2 != x)
			{
				std::cout << "xor" << std::endl;
				failed = true;
			}


			i64Matrix m(trials, trials), mm(trials, trials);
			for (u64 i = 0; i < m.size(); ++i)
				m(i) = i;

			si64Matrix mShr(trials, trials);
			Sh3Task task = rt.noDependencies();
			//rt.mPrint = true;

			if (rt.mSched.mTasks.size() || rt.mSched.mReady.size())
			{
				std::cout << "bad runtime 1" << std::endl;
				failed = true;
			}
			task = e.localIntMatrix(task, m, mShr);
			e.reveal(task, mShr, mm).get();

			if (mm != m)
			{
				std::cout << "int mtx" << std::endl;
				failed = true;
			}

			if (rt.mSched.mTasks.size() || rt.mSched.mReady.size())
			{
				std::cout << "bad runtime 2" << std::endl;
				failed = true;
			}

			sbMatrix bShr(trials, trials * 64);
			task = e.localBinMatrix(task, m, bShr);
			e.reveal(task, bShr, mm).get();

			if (mm != m)
			{
				std::cout << "bin mtx" << std::endl;
				failed = true;
			}

			sPackedBin pShr(trials, trials * sizeof(i64) * 8);
			task = e.localPackedBinary(task, m, pShr);
			e.reveal(task, pShr, mm).get();

			if (mm != m)
			{
				std::cout << "pac mtx" << std::endl;
				std::cout << m << std::endl;
				std::cout << mm << std::endl;

				failed = true;
			}

	});


	auto rr = [&](int i)
	{
		setThreadName("t_"+std::to_string(i));
		auto& e = enc[i];
		//auto& c = comm[i];
		Sh3Runtime rt(i, comm[i]);
		std::vector<si64> shrs(trials);
		std::vector<sb64> bshrs(trials);
		std::vector<std::array<Sh3Task, 2>> tasks(trials);
		si64 sum{ { 0,0 } };
		sb64 XOR{ { 0,0 } };

		e.remoteInt(rt.noDependencies(), shrs[0]).get();

		auto tt = rt.noDependencies();

		for (int i = 0; i < trials; ++i)
		{
			tasks[i][0] = e.remoteInt(rt.noDependencies(), shrs[i]).then([&sum, &shrs, i](Sh3Task& _) mutable {
					sum = sum + shrs[i];
					});

			tasks[i][1] = e.remoteBinary(rt.noDependencies(), bshrs[i]).then([&XOR, &bshrs, i](Sh3Task& _) mutable {
					XOR = XOR ^ bshrs[i];
					});

			tt = tt && tasks[i][0] && tasks[i][1];
		}

		for (int i = 0; i < trials; ++i)
		{
			e.reveal(tasks[i][0], 0, shrs[i]).get();
			e.reveal(tasks[i][1], 0, bshrs[i]).get();
		}
		e.reveal(tt, 0, sum).get();
		e.reveal(tt, 0, XOR).get();




		si64Matrix mShr(trials, trials);
		Sh3Task task = rt.noDependencies();
		task = e.remoteIntMatrix(task, mShr);
		e.reveal(task, 0, mShr).get();

		sbMatrix bShr(trials, trials * 64);
		task = e.remoteBinMatrix(task, bShr);
		e.reveal(task, 0, bShr).get();

		sPackedBin pShr(trials, trials * sizeof(i64) * 8);
		task = e.remotePackedBinary(task, pShr);
		e.reveal(task, 0, pShr).get();

	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();

	if (failed)
		throw std::runtime_error(LOCATION);
}

void Sh3_Encryptor_modified_IO_test()
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();


	int trials = 10;
	CommPkg comm[3];
	comm[0] = { chl01, chl02 };
	comm[1] = { chl10, chl12 };
	comm[2] = { chl21, chl20 };

	Sh3Encryptor enc[3];
	enc[0].init(0, toBlock(0, 0), toBlock(1, 1));
	enc[1].init(1, toBlock(1, 1), toBlock(2, 2));
	enc[2].init(2, toBlock(2, 2), toBlock(0, 0));

	bool failed = false;
	auto t0 = std::thread([&]() {
			auto i = 0;
			auto& e = enc[i];
			auto& c = comm[i];
			PRNG prng(ZeroBlock);

			i64 s = 0;
			std::vector<i64> vals(trials);
			std::vector<si64> shrs(trials);
			si64 sum{ { 0,0 } };
			for (int i = 0; i < trials; ++i)
			{
			vals[i] = i * 325143121;
			shrs[i] = e.localInt_modified(c, vals[i]);
			ostreamLock(std::cout)<<"Next party: "<<c.mNext<<" Previous party: "<<c.mPrev<<" Value: "<<vals[i]<<" Corresponding share 0: "<<shrs[i].mData[0]<<" Corresponding share 1: "<<shrs[i].mData[1]<<std::endl;
			sum = sum + shrs[i];
			s += vals[i];
			}

			/*for (int i = 0; i < trials; ++i)
			  {
			  i64 v;
			  v = e.reveal(c, shrs[i]);
			  if (v != vals[i]) {
			  std::cout << "localInt[" << i << "] " << v << " " << vals[i] << " failed" << std::endl;
			  failed = true;
			  }
			  }*/

	});


	auto rr = [&](int i) {
		auto& e = enc[i];
		auto& c = comm[i];
		std::vector<si64> shrs(trials);
		si64 sum{ { 0,0 } };

		for (int j = 0; j < trials; ++j)
		{
			shrs[j] = e.remoteInt_modified(c, i);
			sum = sum + shrs[j];
		}

		/*for (int j = 0; j < trials; ++j)
		  {
		  e.reveal(c, 0, shrs[j]);
		  }*/

	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();

	if (failed)
		throw std::runtime_error(LOCATION);
}

void Astra_basic_shares_gen_test()
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();


	int trials = 10;
	CommPkg comm[3];
	comm[0] = { chl02, chl01 };
	comm[1] = { chl10, chl12 };
	comm[2] = { chl21, chl20 };

	Sh3Encryptor enc[3];
	enc[0].init(0, toBlock(0, 0), toBlock(1, 1));
	enc[1].init(1, toBlock(1, 1), toBlock(2, 2));
	enc[2].init(2, toBlock(2, 2), toBlock(0, 0));

	bool failed = false;
	auto t0 = std::thread([&]() {
			auto i = 0;
			auto& e = enc[i];
			auto& c = comm[i];
			PRNG prng(ZeroBlock);

			std::vector<i64> vals(trials);
			std::vector<si64> shrs(trials);
			for (int i = 0; i < trials; ++i)
			{
			vals[i] = i * 325143121;
			shrs[i] = e.astra_preprocess_0(c);
			e.astra_online_0(c, vals[i], shrs[i]);
			}

	});


	auto rr = [&](int i) {
		auto& e = enc[i];
		auto& c = comm[i];
		std::vector<si64> shrs(trials);

		for (int j = 0; j < trials; ++j)
		{
			shrs[j].mData[0] = e.astra_preprocess(c, i);
			shrs[j].mData[1] = e.astra_online(c, i);
		}
		
		for (int j = 0; j < trials; ++j)
		{
			if(i == 1)
			{	
                i64 recovered_val;
			    recovered_val = e.astra_reveal_1(c, shrs[j]);
                std::cout<<"Recovered value: "<<recovered_val<<std::endl;

            }
            else if(i == 2)
            {
                e.astra_reveal_2(c, shrs[j]);
            }
		}

	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();

	if (failed)
		throw std::runtime_error(LOCATION);
}

void Astra_basic_shares_matrix_gen_test()
{

	IOService ios;
	auto chl01 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "01").addChannel();
	auto chl10 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "01").addChannel();
	auto chl02 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "02").addChannel();
	auto chl20 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "02").addChannel();
	auto chl12 = Session(ios, "127.0.0.1:1313", SessionMode::Server, "12").addChannel();
	auto chl21 = Session(ios, "127.0.0.1:1313", SessionMode::Client, "12").addChannel();


	int trials = 10;
	CommPkg comm[3];
	comm[0] = { chl02, chl01 };
	comm[1] = { chl10, chl12 };
	comm[2] = { chl21, chl20 };

	Sh3Encryptor enc[3];
	enc[0].init(0, toBlock(0, 0), toBlock(1, 1));
	enc[1].init(1, toBlock(1, 1), toBlock(2, 2));
	enc[2].init(2, toBlock(2, 2), toBlock(0, 0));

	bool failed = false;
	auto t0 = std::thread([&]() {
			auto i = 0;
			auto& e = enc[i];
			auto& c = comm[i];
			PRNG prng(ZeroBlock);

            i64Matrix m(trials, trials), mm(trials, trials);
            for (u64 i = 0; i < m.size(); ++i)
                m(i) = i;
            si64Matrix mShr(trials, trials);
            e.astra_preprocess_matrix_0(c, mShr); 
			e.astra_online_matrix_0(c, m, mShr);

            i64Matrix a(1, 3), b(1, 3);
            for (u64 i = 0; i < a.size(); ++i)
                a(i) = i;

            for (u64 i = 0; i < b.size(); ++i)
                b(i) = i+3;
            ostreamLock(std::cout)<<"Matrix a: "<<a<<std::endl;
            ostreamLock(std::cout)<<"Matrix b: "<<b<<std::endl;
            ostreamLock(std::cout)<<"Dot product: "<<a*b.transpose()<<std::endl;

	});


	auto rr = [&](int i) {
		auto& e = enc[i];
		auto& c = comm[i];
        si64Matrix mShr(trials, trials);

	    e.astra_preprocess_matrix(c, i, mShr.mShares[0]);
		e.astra_online_matrix(c, i, mShr.mShares[1]);
		
        if(i == 1)
        {	
            i64Matrix recovered_matrix(trials,trials);
            e.astra_reveal_matrix_1(c, mShr, recovered_matrix);
            std::cout<<"Recovered matrix: "<<recovered_matrix<<std::endl;

        }
        else if(i == 2)
        {
            e.astra_reveal_matrix_2(c, mShr);
        }

	};

	auto t1 = std::thread(rr, 1);
	auto t2 = std::thread(rr, 2);

	t0.join();
	t1.join();
	t2.join();

	if (failed)
		throw std::runtime_error(LOCATION);
}

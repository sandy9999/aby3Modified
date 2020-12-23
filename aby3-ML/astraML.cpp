#include "astraML.h"
#include <cryptoTools/Crypto/PRNG.h>

void aby3::astraML::init(u64 partyIdx, oc::Session& prev, oc::Session& next, oc::block seed)
{
	mPrev = prev.addChannel();
	mNext = next.addChannel();

	CommPkg c{ mPrev, mNext };
	mRt.init(partyIdx, c);

	oc::PRNG prng(seed);
	mEnc.init(partyIdx, c, prng.get<block>());
	mEval.init(partyIdx, c, prng.get<block>());
}

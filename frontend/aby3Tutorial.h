#include <cryptoTools/Network/IOService.h>

#include "aby3/sh3/Sh3Runtime.h"
#include "aby3/sh3/Sh3Encryptor.h"
#include "aby3/sh3/Sh3Evaluator.h"

void setup(aby3::u64 partyIdx, oc::IOService& ios, aby3::Sh3Encryptor& enc, aby3::Sh3Evaluator& eval, aby3::Sh3Runtime& runtime);
void integerOperations(aby3::u64 partyIdx);
void matrixOperations(aby3::u64 partyIdx);
void fixedPointOperations(aby3::u64 partyIdx);

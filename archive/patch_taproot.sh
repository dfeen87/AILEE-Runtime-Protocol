cat include/Global_Seven.h | grep -A 10 "struct AnchorCommitment"
cat src/l1/AnchorCommitment.cpp | grep -A 20 "bool AnchorCommitment::computeTweakedKey"
cat CMakeLists.txt | grep -A 5 "pkg_check_modules(libsecp256k1"

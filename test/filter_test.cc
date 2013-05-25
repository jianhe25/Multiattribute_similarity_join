#include "../src/filter.h"
#include "../src/core.h"
#include "gtest/gtest.h"
namespace {
	class VerifierTest : public ::testing::Test {
		public:
			virtual void SetUp() {
			}
			Verifier verifier_;
	};
	TEST_F(VerifierTest, VerifyED) {
		EXPECT_TRUE(verifier_.filter(Field("sigmod"), Field("sigir"), Similarity(ED, 3)));
		EXPECT_EQ(3, verifier_.edit_distance());

		EXPECT_FALSE(verifier_.filter(Field("sigir"), Field("sigmod"), Similarity(ED, 2)));
	}

	TEST_F(VerifierTest, VerifyOverlap) {
		EXPECT_TRUE(verifier_.filter(Field("sigmod test awesome"), Field("sigmod awesome"), Similarity(JACCARD, 0.5)));
		EXPECT_EQ(2, verifier_.overlap());

		EXPECT_FALSE(verifier_.filter(Field("sigir really"), Field("sigmod"), Similarity(JACCARD, 0.8)));
		EXPECT_EQ(0, verifier_.overlap());
	}
	int main(int argc, char **argv) {
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}
}


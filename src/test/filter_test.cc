#include "../filter.h"
#include "../core.h"
#include "gtest/gtest.h"
namespace {
	class VerifierTest : public ::testing::Test {
		public:
			virtual void SetUp() {
			}
			Verifier verifier_;
	};
	TEST_F(VerifierTest, VerifyED) {
		Field a("sigmod"), b("sigir");
		a.GenerateGrams();
		b.GenerateGrams();
		EXPECT_TRUE(verifier_.filter(a, b, Similarity(ED, 3)));
		EXPECT_EQ(3, verifier_.edit_distance());

		EXPECT_FALSE(verifier_.filter(b, a, Similarity(ED, 2)));
	}

	TEST_F(VerifierTest, VerifyOverlap) {
		Field a("sigmod test awesome"), b("sigmod awesome");
		a.GenerateTokens();
		b.GenerateTokens();
		EXPECT_TRUE(verifier_.filter(a, b, Similarity(JACCARD, 0.5)));
		EXPECT_EQ(2, verifier_.overlap());

		a = Field("sigmod really");
		b = Field("sigmod");
		a.GenerateTokens();
		b.GenerateTokens();
		EXPECT_FALSE(verifier_.filter(a, b, Similarity(JACCARD, 0.8)));
		EXPECT_EQ(1, verifier_.overlap());
	}
	int main(int argc, char **argv) {
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}
}


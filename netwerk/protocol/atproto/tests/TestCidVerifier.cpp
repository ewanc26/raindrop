/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CidVerifier.h"

#include <string>

#include "gtest/gtest.h"
#include "json/json.h"

using namespace mozilla::net::atproto;

namespace {

Json::Value ParseJson(const std::string& aJson) {
  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  EXPECT_TRUE(reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                            &errors))
      << errors;
  return root;
}

void ExpectVerified(const std::string& aJson, const std::string& aCid) {
  EXPECT_EQ(VerifyRecordCid(aCid, ParseJson(aJson)), CidStatus::Verified);
}

}  // namespace

TEST(CidVerifier, VerifiesKnownVectors) {
  ExpectVerified(R"({"$type":"app.bsky.feed.post","text":"hello","replyCount":3})",
                 "bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga");
  ExpectVerified(R"({"$type":"app.bsky.feed.post","text":"Hello, world!"})",
                 "bafyreifffw4fwq54bnrel7salmw5v2mti643pjajbbuo2o3xtwvai3wd54");
  ExpectVerified(
      R"({"text":"unicode: \u00e9\ud83d\ude00","n":-7,"ok":true,"empty":null})",
      "bafyreiardoerdrq6avmqtbkrborcok4er4acqj4leabzi3fkxrgbanqsw4");
  ExpectVerified(R"({"a":[1,2,3],"b":{"z":1,"aa":2}})",
                 "bafyreihb36m2pd3euvztb4kftrwxdvlzlefdca63kdrcv4aau3vwa7ky24");
}

TEST(CidVerifier, DetectsModifiedValue) {
  Json::Value value = ParseJson(R"({"text":"hello"})");
  const std::string cid =
      "bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga";
  EXPECT_EQ(VerifyRecordCid(cid, value), CidStatus::Mismatch);
  value["text"] = "hello!";
  EXPECT_EQ(VerifyRecordCid(cid, value), CidStatus::Mismatch);
}

TEST(CidVerifier, RejectsInvalidCids) {
  Json::Value value = ParseJson(R"({"text":"hello"})");
  EXPECT_EQ(VerifyRecordCid("", value), CidStatus::Absent);
  EXPECT_EQ(VerifyRecordCid("notacid", value), CidStatus::InvalidCid);
  EXPECT_EQ(VerifyRecordCid("bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5g",
                            value),
            CidStatus::InvalidCid);
  EXPECT_EQ(VerifyRecordCid("uafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga",
                            value),
            CidStatus::InvalidCid);
}

TEST(CidVerifier, RejectsUnsupportedFloats) {
  Json::Value value = ParseJson(R"({"n":1.5})");
  const std::string cid =
      "bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga";
  EXPECT_EQ(VerifyRecordCid(cid, value), CidStatus::UnsupportedValue);
}

TEST(CidVerifier, RejectsWrongCodecOrHash) {
  Json::Value value = ParseJson(R"({"text":"hello"})");
  // Valid base32 but codec 0x55 (raw) and sha2-512 multihash length.
  EXPECT_EQ(VerifyRecordCid("bafkreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga",
                            value),
            CidStatus::InvalidCid);
}

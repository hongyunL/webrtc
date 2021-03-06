/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/aec3/suppression_gain.h"

#include "webrtc/base/checks.h"
#include "webrtc/system_wrappers/include/cpu_features_wrapper.h"
#include "webrtc/test/gtest.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace aec3 {

#if RTC_DCHECK_IS_ON && GTEST_HAS_DEATH_TEST && !defined(WEBRTC_ANDROID)

// Verifies that the check for non-null output gains works.
TEST(SuppressionGain, NullOutputGains) {
  std::array<float, kFftLengthBy2Plus1> E2;
  std::array<float, kFftLengthBy2Plus1> R2;
  std::array<float, kFftLengthBy2Plus1> N2;
  E2.fill(0.f);
  R2.fill(0.f);
  N2.fill(0.f);
  float high_bands_gain;
  EXPECT_DEATH(SuppressionGain(DetectOptimization())
                   .GetGain(E2, R2, N2, false,
                            std::vector<std::vector<float>>(
                                3, std::vector<float>(kBlockSize, 0.f)),
                            false, &high_bands_gain, nullptr),
               "");
}

#endif

// Does a sanity check that the gains are correctly computed.
TEST(SuppressionGain, BasicGainComputation) {
  SuppressionGain suppression_gain(DetectOptimization());
  float high_bands_gain;
  std::array<float, kFftLengthBy2Plus1> E2;
  std::array<float, kFftLengthBy2Plus1> R2;
  std::array<float, kFftLengthBy2Plus1> N2;
  std::array<float, kFftLengthBy2Plus1> g;
  std::vector<std::vector<float>> x(1, std::vector<float>(kBlockSize, 0.f));

  // Ensure that a strong noise is detected to mask any echoes.
  E2.fill(10.f);
  R2.fill(0.1f);
  N2.fill(100.f);
  for (int k = 0; k < 10; ++k) {
    suppression_gain.GetGain(E2, R2, N2, false, x, false, &high_bands_gain, &g);
  }
  std::for_each(g.begin(), g.end(),
                [](float a) { EXPECT_NEAR(1.f, a, 0.001); });

  // Ensure that a strong nearend is detected to mask any echoes.
  E2.fill(100.f);
  R2.fill(0.1f);
  N2.fill(0.f);
  for (int k = 0; k < 10; ++k) {
    suppression_gain.GetGain(E2, R2, N2, false, x, false, &high_bands_gain, &g);
  }
  std::for_each(g.begin(), g.end(),
                [](float a) { EXPECT_NEAR(1.f, a, 0.001); });

  // Ensure that a strong echo is suppressed.
  E2.fill(1000000000.f);
  R2.fill(10000000000000.f);
  N2.fill(0.f);
  for (int k = 0; k < 10; ++k) {
    suppression_gain.GetGain(E2, R2, N2, false, x, false, &high_bands_gain, &g);
  }
  std::for_each(g.begin(), g.end(),
                [](float a) { EXPECT_NEAR(0.f, a, 0.001); });

  // Verify the functionality for forcing a zero gain.
  suppression_gain.GetGain(E2, R2, N2, false, x, true, &high_bands_gain, &g);
  std::for_each(g.begin(), g.end(), [](float a) { EXPECT_FLOAT_EQ(0.f, a); });
  EXPECT_FLOAT_EQ(0.f, high_bands_gain);
}

}  // namespace aec3
}  // namespace webrtc

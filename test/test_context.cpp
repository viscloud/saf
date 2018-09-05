// Copyright 2018 The SAF Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include "saf.h"

TEST(CONTEXT_TEST, TEST_BASIC) {
  Context& context = Context::GetContext();

  // Sanaity check
  context.GetString(H264_DECODER_GST_ELEMENT);
  context.GetString(H264_ENCODER_GST_ELEMENT);
}

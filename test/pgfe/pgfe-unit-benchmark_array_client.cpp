// -*- C++ -*-
//
// Copyright 2022 Dmitry Igrishin
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

#include "pgfe-unit-benchmark_array.hpp"

int main(int argc, char* argv[])
try {
  namespace pgfe = dmitigr::pgfe;

  auto [output_file, conn] = pgfe::test::arraybench::prepare(argc, argv);
  conn->execute([output_file = std::move(output_file)](auto&& row)mutable
  {
    using Array = pgfe::Array_optional1<std::string>;
    for (const auto& elem : pgfe::to<Array>(row[0])) {
      if (elem)
        output_file << *elem;
    }
    output_file << "\n";
  }, "select dat from benchmark_test_array");
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown error" << std::endl;
  return 2;
}

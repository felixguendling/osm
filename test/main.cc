#include <filesystem>

#include "gtest/gtest.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  std::clog.rdbuf(std::cout.rdbuf());

  fs::current_path(TEST_DIR);

  ::testing::InitGoogleTest(&argc, argv);
  auto test_result = RUN_ALL_TESTS();

  return test_result;
}
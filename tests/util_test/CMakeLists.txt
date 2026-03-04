gt_register_test(
  NAME util-test
  SRC util-test.cpp
  DEPS
    utils
)

gt_register_test(
  NAME scheduler-test
  SRC scheduler-test.cpp
  DEPS
    utils
)
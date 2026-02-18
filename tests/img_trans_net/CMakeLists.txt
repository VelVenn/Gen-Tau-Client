gt_register_test(
	NAME recv-test
	SRC recv-test.cpp
	DEPS
		img-trans
		utils
)

gt_register_test(
	NAME bind-test 
	SRC bind-test.cpp
	DEPS
		img-trans
		utils
)

gt_register_test(
	NAME frame-data-test 
	SRC frame-data-test.cpp
	DEPS
		img-trans
		utils
)
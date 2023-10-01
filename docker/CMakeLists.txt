cmake_minimum_required (VERSION 3.0)

project(docker)

add_custom_target(
	docker_build_clang
	docker build -f ${CMAKE_CURRENT_SOURCE_DIR}/DockerfileAlpineClang -t genesis_alpine_clang ${CMAKE_SOURCE_DIR}
)

add_custom_target(
	docker_build_gcc
	docker build -f ${CMAKE_CURRENT_SOURCE_DIR}/DockerfileAlpineGCC -t genesis_alpine_gcc ${CMAKE_SOURCE_DIR}
)

add_custom_target(
	docker_test_clang
	docker run -ti --rm genesis_alpine_clang /bin/sh -c "make && ctest"
	DEPENDS docker_build_clang
)

add_custom_target(
	docker_test_gcc
	docker run -ti --rm genesis_alpine_gcc /bin/sh -c "make && ctest"
	DEPENDS docker_build_gcc
)

add_custom_target(
	docker_build
	DEPENDS docker_build_clang docker_build_gcc
)

add_custom_target(
	docker_test
	DEPENDS docker_test_clang docker_test_gcc
)

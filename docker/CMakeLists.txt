cmake_minimum_required (VERSION 3.0)

project(tools)

add_custom_target(
	docker_clang
	docker build -f ${CMAKE_CURRENT_SOURCE_DIR}/DockerfileAlpineClang -t genesis_alpine_clang ${CMAKE_SOURCE_DIR}
)

add_custom_target(
	docker_gcc
	docker build -f ${CMAKE_CURRENT_SOURCE_DIR}/DockerfileAlpineGCC -t genesis_alpine_gcc ${CMAKE_SOURCE_DIR}
)

add_custom_target(
	docker_all
	DEPENDS docker_clang docker_gcc
)

cmake_minimum_required (VERSION 3.10)

project(extra_tools)

function(GET_TARGET_SOURCES TARGET RESULT)
	get_target_property(src ${TARGET} SOURCES)
	get_target_property(dir ${TARGET} SOURCE_DIR)

	set(src_dir)

	foreach(s ${src})
		# not supported yet
		# cmake_path(APPEND s dir)
		list(APPEND src_dir "${dir}/${s}")
	endforeach()

	list(APPEND src_dir ${${RESULT}})
	set(${RESULT} ${src_dir} PARENT_SCOPE)
endfunction()


get_target_sources(${GENESIS_LIB} SRC)
get_target_sources(${GENESIS} SRC)
get_target_sources(${GENESIS_TESTS} SRC)


add_custom_target(
	format
	clang-format -style=file -i ${SRC}
)

add_custom_target(
	tidy
	# todo: may also print '1 warning generated.'
	clang-tidy --quiet -header-filter=.* -p ${CMAKE_BINARY_DIR} ${SRC} |& sed "/\\d* warnings generated\\./d"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

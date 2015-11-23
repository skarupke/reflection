
tup.include(tup.getconfig('COMPILER') .. '.lua')
tup.include(tup.getconfig('VARIANT') .. '.lua')
build_options = tup.getconfig('BUILD_OPTIONS')
if build_options != '' then
    tup.include(build_options .. '.lua')
end

WARNING_FLAGS += '-Werror'
WARNING_FLAGS += '-Wall'
WARNING_FLAGS += '-Wextra'
WARNING_FLAGS += '-Wno-unknown-pragmas'
WARNING_FLAGS += '-Wdeprecated'
WARNING_FLAGS += '-Wno-comment'

--DEFINES += '-DFINAL'
DEFINES += '-D_GLIBCXX_USE_DEPRECATED=0'
DEFINES += '-DBOOST_NO_AUTO_PTR'

-- needed for google benchmark library
DEFINES += '-DHAVE_STD_REGEX'

CPP_FLAGS += WARNING_FLAGS
CPP_FLAGS += '-std=c++14'
--CPP_FLAGS += '-fno-exceptions'
CPP_LINKER = CPP_COMPILER
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/src'
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/libs/gtest'
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/libs/benchmark/include'
INCLUDE_DIRS += '-Isrc/flatbuffers/'
INCLUDE_DIRS += '-Isrc/protobuf/'
INCLUDE_DIRS += '-I/home/malte/workspace/flatbuffers/include'
CPP_FLAGS += INCLUDE_DIRS
CPP_FLAGS += DEFINES
CPP_FLAGS += '-g'

function compile_cpp(source, inputs)
    output = source .. '.o'
    inputs += source
    tup.definerule{ inputs = inputs, command = CPP_COMPILER .. ' ' .. table.concat(CPP_FLAGS, ' ') .. ' -c ' .. source .. ' -o ' .. output, outputs = {output} }
end

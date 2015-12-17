
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

C_CPP_FLAGS += WARNING_FLAGS
CPP_FLAGS += '-std=c++14'
--CPP_FLAGS += '-fno-exceptions'
CPP_LINKER = CPP_COMPILER
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/src'
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/libs/gtest'
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/libs/benchmark/include'
INCLUDE_DIRS += '-I' .. tup.getcwd() .. '/libs/z4'
INCLUDE_DIRS += '-Isrc/flatbuffers/'
INCLUDE_DIRS += '-Isrc/protobuf/'
INCLUDE_DIRS += '-I/home/malte/workspace/flatbuffers/include'
C_CPP_FLAGS += INCLUDE_DIRS
C_CPP_FLAGS += DEFINES
C_CPP_FLAGS += '-g'
C_FLAGS = C_CPP_FLAGS
CPP_FLAGS += C_CPP_FLAGS

function compile_cpp(source, inputs)
    output = source .. '.o'
    inputs += source
    tup.definerule{ inputs = inputs, command = CPP_COMPILER .. ' ' .. table.concat(CPP_FLAGS, ' ') .. ' -c ' .. source .. ' -o ' .. output, outputs = {output} }
end
function compile_c(source, inputs)
    output = source .. '.o'
    inputs += source
    tup.definerule{ inputs = inputs, command = C_COMPILER .. ' ' .. table.concat(C_FLAGS, ' ') .. ' -c ' .. source .. ' -o ' .. output, outputs = {output} }
end

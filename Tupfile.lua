
LINK_FLAGS += '-pthread'
LINK_FLAGS += '-fuse-ld=gold'
-- the next two lines are for incremental linking.
-- they are turned off because they seem to be slower
-- right now. maybe use it when the project gets bigger
--LINK_FLAGS += '-Wl,--incremental'
--LINK_FLAGS += '-fno-use-linker-plugin'

LINK_LIBS += '-lprotobuf'
LINK_LIBS += '-lboost_serialization'

INPUT_FOLDERS = {}
INPUT_FOLDERS += './'
INPUT_FOLDERS += 'src/'
INPUT_FOLDERS += 'src/debug/'
INPUT_FOLDERS += 'src/flatbuffers/'
INPUT_FOLDERS += 'src/meta/'
INPUT_FOLDERS += 'src/meta/serialization/'
INPUT_FOLDERS += 'src/metav3/'
INPUT_FOLDERS += 'src/metav3/serialization/'
INPUT_FOLDERS += 'src/metafast/'
INPUT_FOLDERS += 'src/os/'
INPUT_FOLDERS += 'src/protobuf/'
INPUT_FOLDERS += 'src/util/'

for _, v in ipairs(INPUT_FOLDERS) do
    CPP_SOURCES += tup.glob(v .. '*.cpp')
end

OUTPUT_FOLDERS = INPUT_FOLDERS

CPP_SOURCES += 'libs/gtest/src/gtest-all.cc'
OUTPUT_FOLDERS += 'libs/gtest/src/'
CPP_SOURCES += tup.glob('libs/benchmark/src/*.cc')
OUTPUT_FOLDERS += 'libs/benchmark/src/'
C_SOURCES += tup.glob('libs/z4/*.c')
OUTPUT_FOLDERS += 'libs/z4/'

protobuf_headers = {}
function compile_protobuf()
    protobuf_outputs = {}
    protobuf_inputs = {}
    cpp_outputs = {}
    for _, v in ipairs(tup.glob('src/protobuf/*.proto')) do
        protobuf_inputs += v
        no_ext = v:sub(1, #v - 6)
        cpp_output = no_ext .. '.pb.cc'
        h_output = no_ext .. '.pb.h'
        protobuf_outputs += { cpp_output, h_output }
        cpp_outputs += cpp_output
        protobuf_headers += h_output
    end
    tup.definerule{ inputs = protobuf_inputs, command = 'protoc -Isrc --cpp_out=src ' .. table.concat(protobuf_inputs, ' '), outputs = protobuf_outputs }
    for _, v in ipairs(cpp_outputs) do
        compile_cpp(v, protobuf_headers)
    end
end
compile_protobuf()

flatbuffers_headers = {}
function compile_flatbuffers()
    for _, v in ipairs(tup.glob('src/flatbuffers/*.idl')) do
        no_ext = v:sub(1, #v - 4)
        output = no_ext .. '_generated.h'
        out_folder = 'src/flatbuffers/'
        flatbuffers_headers += output
        tup.definerule{ inputs = v, command = '/home/malte/workspace/flatbuffers/flatc -c -o ' .. out_folder .. ' ' .. v, outputs = { output } }
    end
end
compile_flatbuffers()

generated_headers = protobuf_headers
generated_headers += flatbuffers_headers

for i, v in ipairs(CPP_SOURCES) do
    compile_cpp(v, generated_headers)
end
for i, v in ipairs(C_SOURCES) do
    compile_c(v, generated_headers)
end

executable_name = 'main'

objfiles = {}
for _, v in ipairs(OUTPUT_FOLDERS) do
    objfiles += tup.glob(v .. '*.o')
end
tup.definerule{ inputs = objfiles,
                command = CPP_LINKER .. ' ' .. table.concat(LINK_FLAGS, ' ') .. ' ' .. table.concat(objfiles, ' ') .. ' ' .. table.concat(LINK_LIBS, ' ') .. ' -o ' .. executable_name,
                outputs = {executable_name} }

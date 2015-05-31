
LINK_FLAGS += '-pthread'
LINK_FLAGS += '-fuse-ld=gold'
-- the next two lines are for incremental linking.
-- they are turned off because they seem to be slower
-- right now. maybe use it when the project gets bigger
--LINK_FLAGS += '-Wl,--incremental'
--LINK_FLAGS += '-fno-use-linker-plugin'

LINK_LIBS += ''

SOURCES += tup.glob('*.cpp')
SOURCES += tup.glob('src/*.cpp')
SOURCES += tup.glob('src/debug/*.cpp')
SOURCES += tup.glob('src/meta/*.cpp')
SOURCES += tup.glob('src/meta/serialization/*.cpp')
SOURCES += tup.glob('src/metav3/*.cpp')
SOURCES += tup.glob('src/metav3/serialization/*.cpp')
SOURCES += tup.glob('src/metafast/*.cpp')
SOURCES += tup.glob('src/os/*.cpp')
SOURCES += tup.glob('src/util/*.cpp')

SOURCES += 'libs/gtest/src/gtest-all.cc'
SOURCES += tup.glob('libs/benchmark/src/*.cc')

for i, v in ipairs(SOURCES) do
    compile_cpp(v)
end

executable_name = 'main'

objfiles = tup.glob('*.o')
tup.definerule{ inputs = objfiles,
                command = CPP_LINKER .. ' ' .. table.concat(LINK_FLAGS, ' ') .. ' ' .. table.concat(objfiles, ' ') .. ' ' .. table.concat(LINK_LIBS, ' ') .. ' -o ' .. executable_name,
                outputs = {executable_name} }

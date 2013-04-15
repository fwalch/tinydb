CXX:=g++
ifeq ($(USER),florian)
CXX:=clang++
endif
CXXFLAGS:=-g -std=c++0x -Wall -Wextra -Isrc

DEPTRACKING=-MD -MF $(@:.o=.d)
BUILDEXE=$(CXX) -o$@ $(CXXFLAGS) $(LDFLAGS) $^

ifeq ($(OS),Windows_NT)
nativefile=$(subst /,\\,$(1))
CHECKDIR=@if not exist $(call nativefile,$(dir $@)) mkdir $(call nativefile,$(dir $@))
LDFLAGS:= -Wl,--enable-auto-import
EXEEXT:=.exe
BATEXT:=.cmd
else
CHECKDIR=@mkdir -p $(dir $@)
EXEEXT:=
BATEXT:=
endif

all: bin/admin$(EXEEXT) examples_bin exercises_bin

db:
	cd data && ./loaduni$(BATEXT)

include src/LocalMakefile
include examples/LocalMakefile
include exercises/LocalMakefile

-include bin/*.d bin/*/*.d

bin/%.o: src/%.cpp
	$(CHECKDIR)
	$(CXX) -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<

bin/examples/%.o: examples/%.cpp
	$(CHECKDIR)
	$(CXX) -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<

bin/exercises/%.o: exercises/%.cpp
	$(CHECKDIR)
	$(CXX) -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<

clean:
	find bin -name '*.d' -delete -o -name '*.o' -delete -o '(' -perm -u=x '!' -type d ')' -delete

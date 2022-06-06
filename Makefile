CFLAGS := -O0 # can be set via command-line
CFLAGS += -std=c++20 -Wall -Wextra -Werror -pedantic

h := zest.hh
test-src := t/*.cc
test-bin := t/test

all: test-fail test-pass

test-pass: $(test-bin)
	-$(test-bin) pass

test-fail: $(test-bin)
	-$(test-bin) fail

$(test-bin): $(h) $(test-src)
	$(CXX) $(CFLAGS) -I. $(test-src) -o $@

README.md: $(h) $(test-bin) Makefile
	@printf '> This readme is automatically generated ' >$@
	@printf 'from `$(h)` and `$(test-bin)`. '          >>$@
	@printf 'See `Makefile`.\n\n'                      >>$@
	@grep '^\*\*' $(h) | sed -e 's/** \{0,2\}//'       >>$@
	@printf "\n\n"                                     >>$@
	@echo 'SAMPLE OUTPUT'                              >>$@
	@echo '-------------'                              >>$@
	@echo                                              >>$@
	@echo 'The output of `make test-pass`:'            >>$@
	@echo '```'                                        >>$@
	@-$(test-bin) pass                                 >>$@
	@echo '```'                                        >>$@
	@echo                                              >>$@
	@echo 'The output of `make test-fail`:'            >>$@
	@echo '```'                                        >>$@
	@-$(test-bin) fail                                 >>$@
	@echo '```'                                        >>$@
	@echo OK

clean:
	@rm -rf $(test-bin)


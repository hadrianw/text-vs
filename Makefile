CLONEURL=git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
NTOPFILES=10

bindiff: diff2edit topdiff
	valgrind --leak-check=full ./diff2edit topdiff > bindiff

diff2edit: diff2edit.c
	gcc $< -o $@ -g -std=c99 -Wall -Wextra -pedantic

topdiff: topfiles
	(cd linux && git log --reverse --pretty=format: -p --no-merges $$(awk 'NF==2{print $$2; exit}' ../$<)) > $@

topfiles: linux
	(cd linux && git log --pretty=format: --name-only | sort | uniq -c | sort -rg | head -n $(NTOPFILES)) > $@

linux:
	git clone --branch v4.9 ${CLONEURL}

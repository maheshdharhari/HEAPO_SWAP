# unlink
unlink ../linux-swap/mm/pos.c
unlink ../linux-swap/mm/mempolicy.c
unlink ../linux-swap/mm/pos_namespace.c
unlink ../linux-swap/mm/page_alloc.c

unlink ../linux-swap/include/linux/pos.h
unlink ../linux-swap/include/linux/pos_namespace.h

unlink ../linux-swap/mm/Makefile

# link source
ln -s	../../POS_SWAP2/pos.c			../linux-swap/mm/pos.c
ln -s	../../POS_SWAP2/pos_namespace.c		../linux-swap/mm/pos_namespace.c
ln -s	../../POS_SWAP2/memory.c		../linux-swap/mm/memory.c
ln -s	../../POS_SWAP2/mempolicy.c		../linux-swap/mm/mempolicy.c
ln -s	../../POS_SWAP2/page_alloc.c		../linux-swap/mm/page_alloc.c

# link header
ln -s	../../../POS_SWAP2/pos.h		../linux-swap/include/linux/pos.h
ln -s	../../../POS_SWAP2/pos_namespace.h	../linux-swap/include/linux/pos_namespace.h
ln -s	../../../POS_SWAP2/gfp.h		../linux-swap/include/linux/gfp.h

# link Makefile
ln -s	../../POS_SWAP2/Makefile		../linux-swap/mm/Makefile

# CP
#cp 	./.config				../linux-swap/.config

all:
	@for i in $(MODULES); do \
	  ( cd $$i && $(MAKE) CC='$(CC)' CC_OPS='$(CC_OPS) -I../../' LD_OPS='$(LD_OPS)' SHARED='$(SHARED)' all ) || exit 1; \
	done
clean:
	@for i in $(MODULES); do \
	  ( cd $$i && $(MAKE) clean ) || exit 1; \
	done


TARGETS = Concurrency/TicketLock \
			Concurrency/MSQueue \
			Cache \
			SegmentSystem \
			Journal \
			PhysicalStorage \
			VFS

fclean:
	@$(foreach dir, $(TARGETS), $(MAKE) fclean -C $(dir); )

test:
	@$(foreach dir, $(TARGETS), $(MAKE) re -C $(dir); )
	@$(foreach dir, $(TARGETS), ./$(dir)/test; )
	@$(foreach dir, $(TARGETS), $(MAKE) fclean -C $(dir); )

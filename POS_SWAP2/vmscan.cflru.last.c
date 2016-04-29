
static unsigned long cflru_shrink_page_list(struct list_head *page_list,
                                      struct zone *zone,
                                      struct scan_control *sc,
                                      enum ttu_flags ttu_flags,
                                      unsigned long *ret_nr_dirty,
                                      unsigned long *ret_nr_unqueued_dirty,
                                      unsigned long *ret_nr_congested,
                                      unsigned long *ret_nr_writeback,
                                      unsigned long *ret_nr_immediate,
                                      bool force_reclaim)
{
        LIST_HEAD(ret_pages);
        LIST_HEAD(free_pages);
		LIST_HEAD(wb_pages);
        int pgactivate = 0;
		int allcleanded = 0;
        unsigned long nr_unqueued_dirty = 0;
        unsigned long nr_dirty = 0;
        unsigned long nr_congested = 0;
        unsigned long nr_reclaimed = 0;
        unsigned long nr_writeback = 0;
        unsigned long nr_immediate = 0;
		//nyg
		unsigned long clean = 0;
		unsigned long dirty = 0;
		unsigned long unevictable = 0;
		unsigned long all_pages_number = 0;
		unsigned long need_to_reclaim = sc->nr_to_reclaim - sc->nr_reclaimed;
		
		
		
		if(!need_to_reclaim)
		{
			return 0;
		}
		//
        cond_resched();

        mem_cgroup_uncharge_start();
		
retry:
        while (!list_empty(page_list)) {
                struct address_space *mapping;
                struct page *page;
                int may_enter_fs;
                enum page_references references = PAGEREF_RECLAIM_CLEAN;
                bool dirty, writeback;

                cond_resched();

                page = lru_to_page(page_list);
                list_del(&page->lru);

                if (!trylock_page(page))
                        goto keep;

                VM_BUG_ON_PAGE(PageActive(page), page);
                VM_BUG_ON_PAGE(page_zone(page) != zone, page);

                sc->nr_scanned++;

                if (unlikely(!page_evictable(page)))
                        goto cull_mlocked;

                /* Double the slab pressure for mapped and swapcache pages */
                if (page_mapped(page) || PageSwapCache(page))
                        sc->nr_scanned++;

                may_enter_fs = (sc->gfp_mask & __GFP_FS) ||
                        (PageSwapCache(page) && (sc->gfp_mask & __GFP_IO));

                /*
                 * The number of dirty pages determines if a zone is marked
                 * reclaim_congested which affects wait_iff_congested. kswapd
                 * will stall and start writing pages if the tail of the LRU
                 * is all dirty unqueued pages.
                 */
                page_check_dirty_writeback(page, &dirty, &writeback);
                if (dirty || writeback)
                        nr_dirty++;

                if (dirty && !writeback)
                        nr_unqueued_dirty++;

                /*
                 * Treat this page as congested if the underlying BDI is or if
                 * pages are cycling through the LRU so quickly that the
                 * pages marked for immediate reclaim are making it to the
                 * end of the LRU a second time.
                 */
                mapping = page_mapping(page);
                if ((mapping && bdi_write_congested(mapping->backing_dev_info)) ||
                    (writeback && PageReclaim(page)))
                        nr_congested++;
				
				if(PageDirty(page)&&!allcleanded)
				{
					goto keep_locked;
				}
			
                if (PageWriteback(page)) {
                        if (current_is_kswapd() &&
                            PageReclaim(page) &&
                            zone_is_reclaim_writeback(zone)) {
                                nr_immediate++;
                                goto keep_locked;

                        /* Case 2 above */
                        } else if (global_reclaim(sc) ||
                            !PageReclaim(page) || !(sc->gfp_mask & __GFP_IO)) {
                                SetPageReclaim(page);
                                nr_writeback++;

                                goto keep_writeback;

                        /* Case 3 above */
                        } else {
                                wait_on_page_writeback(page);
                        }
                }

                if (!force_reclaim)
                        references = page_check_references(page, sc);

                switch (references) {
                case PAGEREF_ACTIVATE:
                        goto activate_locked;
                case PAGEREF_KEEP:
                        goto keep_locked;
                case PAGEREF_RECLAIM:
                case PAGEREF_RECLAIM_CLEAN:
                        ; /* try to reclaim the page below */
                }

                if (PageAnon(page) && !PageSwapCache(page)) {
                        if (!(sc->gfp_mask & __GFP_IO))
                                goto keep_locked;
                        if (!add_to_swap(page, page_list))
                                goto activate_locked;
                        may_enter_fs = 1;

                        /* Adding to swap updated mapping */
                        mapping = page_mapping(page);
                }

                if (page_mapped(page) && mapping) {
                        switch (try_to_unmap(page, ttu_flags)) {
                        case SWAP_FAIL:
                                goto activate_locked;
                        case SWAP_AGAIN:
                                goto keep_locked;
                        case SWAP_MLOCK:
                                goto cull_mlocked;
                        case SWAP_SUCCESS:
                                ; /* try to free the page below */
                        }
                }

                if (PageDirty(page)) {
					/* Page is dirty, try to write it out here */
                        switch (pageout(page, mapping, sc)) {
                        case PAGE_KEEP:
                                goto keep_locked;
                        case PAGE_ACTIVATE:
                                goto activate_locked;
                        case PAGE_SUCCESS:
                                if (PageWriteback(page))
                                        goto keep_writeback;
                                if (PageDirty(page))
                                        goto keep;
                                if (!trylock_page(page))
                                        goto keep;
                                if (PageWriteback(page))
                                        goto keep_writeback;
								if (PageDirty(page))
										goto keep_locked;
                                mapping = page_mapping(page);
                        case PAGE_CLEAN:
                                ; /* try to free the page below */
                        }
                }

                if (page_has_private(page)) {
                        if (!try_to_release_page(page, sc->gfp_mask))
                                goto activate_locked;
                        if (!mapping && page_count(page) == 1) {
                                unlock_page(page);
                                if (put_page_testzero(page))
                                        goto free_it;
                                else {
                                        nr_reclaimed++;
                                        continue;
                                }
                        }
                }

                if (!mapping || !__remove_mapping(mapping, page, true))
                        goto keep_locked;

                __clear_page_locked(page);
free_it:
                nr_reclaimed++;

                list_add(&page->lru, &free_pages);
				clean++;
                continue;

cull_mlocked:
                if (PageSwapCache(page))
                        try_to_free_swap(page);
                unlock_page(page);
				unevictable++;
                putback_lru_page(page);
                continue;

activate_locked:
                if (PageSwapCache(page) && vm_swap_full())
                        try_to_free_swap(page);
                VM_BUG_ON_PAGE(PageActive(page), page);
                SetPageActive(page);
                pgactivate++;
keep_locked:
                unlock_page(page);
keep:
				dirty++;
                list_add(&page->lru, &ret_pages);
                VM_BUG_ON_PAGE(PageLRU(page) || PageUnevictable(page), page);
keep_writeback:
				unlock_page(page);
                list_add(&page->lru, &wb_pages);
                VM_BUG_ON_PAGE(PageLRU(page) || PageUnevictable(page), page);
        }
		
		if(!dirty && !clean)
		{
			if(unevictable)
			{
				printk("[POS DEBUG] All page is unevictable\n");
				goto escape;
			}
		}
		
		if(clean < need_to_reclaim)
		{
			if(dirty)
			{
				nr_dirty -= dirty;
				dirty = 0;
				list_splice(&ret_pages, page_list);
				allcleanded = 1;
				goto retry;
			}
		}
		
escape:
		
        free_hot_cold_page_list(&free_pages, 1);
		list_splice(&wb_pages, page_list);
        list_splice(&ret_pages, page_list);
        count_vm_events(PGACTIVATE, pgactivate);
        mem_cgroup_uncharge_end();
        *ret_nr_dirty += nr_dirty;
        *ret_nr_congested += nr_congested;
        *ret_nr_unqueued_dirty += nr_unqueued_dirty;
        *ret_nr_writeback += nr_writeback;
        *ret_nr_immediate += nr_immediate;
        return nr_reclaimed;
}

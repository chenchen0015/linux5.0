

stext
  |->

```c
	sram_virt_addr = __fix_to_virt(FIX_SRAM);
	sram_virt_size = HC_SRAM_VIRT_PAGES * PAGE_SIZE;
	sram_phys_size = hc_sram_size();
	hot_code_size = (u64)(_hot_text_end - _hot_text_start);

	pr_info("hcs: sram virt addr = 0x%lx, size = 0x%lx", sram_virt_addr, sram_virt_size);
	pr_info("hcs: sram phys addr = 0x%lx, size = 0x%lx", sram_phys_addr, sram_phys_size);
	pr_info("hcs: hot code size = 0x%lx", hot_code_size);

	if (sram_virt_size < hot_code_size) {
		pr_err("hcs: sram virt size isn't enough to contain hot code");
		goto KHC_FAILED;
	}

	if (sram_phys_size < hot_code_size) {
		pr_err("hcs: sram phys size isn't enough to contain hot code");
		goto KHC_FAILED;
	}

	for (i = FIX_SRAM; i >= FIX_SRAM_END; i--) {
		sram_virt_addr = __fix_to_virt(i);
		sram_pte = (bm_pte + ((sram_virt_addr >> 12) & 0x1FF));
		set_pte(sram_pte, pfn_pte(__phys_to_pfn(sram_phys_addr), PAGE_KERNEL));
		sram_phys_addr += PAGE_SIZE;
	}

	memcpy(__fix_to_virt(FIX_SRAM), _hot_text_start, hot_code_size);

	for (i = FIX_SRAM; i >= FIX_SRAM_END; i--) {
		sram_virt_addr = __fix_to_virt(i);
		sram_pte = (bm_pte + ((sram_virt_addr >> 12) & 0x1FF));
		pte_clear(0, 0, sram_pte);
	}
```

```c
map_sram_kernel_segment(pgdp, _hot_text_start, _hot_text_end, text_prot, &sram_text, 0, VM_NO_GUARD); //内部魔改pa_start地址
map_kernel_segment(pgdp, _hot_text_end, _etext, text_prot, &vmlinux_text, 0, VM_NO_GUARD);
```



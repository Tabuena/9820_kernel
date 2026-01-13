# Exynos9820 GPU OC patch notes

This document captures how the Exynos9820 GPU was patched to run faster, based
on the same DVFS override path used for CPU overclocking.

## CPU OC pattern (reference)

The CPU overclocking logic lives in the CAL DVFS override flow:

- `drivers/soc/samsung/cal-if/cpucl1_dvfs_table.h` and
  `drivers/soc/samsung/cal-if/cpucl2_dvfs_table.h` define the full CPUCL1/2
  DVFS tables, including the highest (overclocked) bins.
- `drivers/soc/samsung/cal-if/cpucl1_dvfs_overrides.h` and
  `drivers/soc/samsung/cal-if/cpucl2_dvfs_overrides.h` expose those bins as
  override entries for CAL/FVMap/vclk.
- `drivers/soc/samsung/cal-if/vclk.c` inserts override bins into the vclk LUT
  at runtime and bumps the max/boot/resume rate if needed.
- `drivers/soc/samsung/cal-if/cal-if.c` ensures `cal_dfs_get_max_freq()` and
  `cal_dfs_get_rate_table()` return the override levels when present.

GPU OC follows the same mechanism and entry points.

## GPU OC implementation

### DVFS table + override exposure (CAL/FVMap/vclk)

The GPU overclock levels are defined in `drivers/soc/samsung/cal-if/g3d_dvfs_table.h`.
Each entry lists `(rate_khz, volt_uv, pll_freq_hz, pll_p, pll_m, pll_s, pll_k, override)`.
Only the top four bins (910/858/806/754 MHz) set `override = 1`, so
`drivers/soc/samsung/cal-if/gpu_dvfs_overrides.h` will materialize them as
`gpu_dvfs_override_entry` structures that carry the `(rate_khz, volt_uv)` pair.

`drivers/soc/samsung/cal-if/vclk.c::vclk_get_dfs_info` is the runtime hook that
builds each `vclk->lut` from the ECT `DVFS` block. After copying the standard
levels, it checks `is_gpu` and the override list. For each override entry it:

1. Finds an insertion point that keeps the LUT sorted (descending rates in the
   normal GPU DVFS ordering). A duplicate entry is skipped.
2. Allocates a params array cloned from the template level and patches the
   PLL indices via `vclk_pll_idx_for_rate()` so the override uses correct PLL
   settings.
3. Inserts the override level by shifting the LUT and hooks the new params.
4. Tracks `highest_override` and, once all overrides are injected, updates
   `vclk->max_freq`, `boot_freq`, and `resume_freq` so they point at the new top
   bin if they previously matched the original limit.

`drivers/soc/samsung/cal-if/cal-if.c::cal_dfs_get_max_freq()` mirrors this by
querying `gpu_dvfs_override_highest_rate()` (the highest override entry) and
forcing the CAL max rate to that value. `cal_dfs_get_rate_table()` will also
prepend the override rates to the published table so userspace sees them.

This is 1:1 with how CPUCL1/CPUCL2 OC works because `cal-if` treats GPUs, CPUCL1,
and CPUCL2 the same: each has a `_dvfs_overrides` file, the override count is
added to `alloc_num_rates`, and only the forced components (GPU/CPUCL1/CPUCL2)
apply the extra levels. This keeps FVMap, CAL, and vclk in sync without
special-casing GPU logic elsewhere.

### Device tree DVFS/limits (Mali node)

The Mali device tree tables must match the override table so that
governor/pm_qos consumers can request the new rates.

- `arch/arm64/boot/dts/exynos/exynos9820-mali_tables.dtsi` contains the
  `gpu_dvfs_table` (each row is `freq down up stay mif little middle big`), and
  the `gpu_cl_pmqos_table`. The top rows reach 910 MHz, and both
  `gpu_max_clock` and `gpu_max_clock_limit` are set to `910000`, ensuring the
  Mali driver enforces the same cap as CAL.
- `arch/arm64/boot/dts/samsung/exynos9820-mali_rev2.dtsi` pulls in the table and
  explicitly sets `gpu_dvfs_start_clock` / `gpu_dvfs_bl_config_clock` so the
  interactive governor can start at the lower boundary (and the boot rate
  stays within the table) while still allowing dynamic jumps to the override
  bins.

Together this guarantees that the DT-based Mali DVFS controller, the CAL
override path, and the GPU governor see a consistent set of frequencies.

## Changing the GPU OC bins (technical checklist)

1. **Define the new levels.** Extend `drivers/soc/samsung/cal-if/g3d_dvfs_table.h`
   with every new `(rate_khz, volt_uv, pll_freq_hz, pll_p, pll_m, pll_s, pll_k, override)`
   tuple. Set `override = 1` for all levels that should bypass the stock DVFS LUT.
   Confirm the PLL values match the `vclk_pll_idx_for_rate()` table inside
   `drivers/soc/samsung/cal-if/vclk.c` so the new rate can resolve to a valid
   PLL index when `dvfs_override_insert()` patches `override_params`.

2. **Keep overrides in sync.** No further source changes are needed: once the
   table exposes an override entry, `gpu_dvfs_overrides.h` automatically builds
   the `gpu_dvfs_override_entry[]`, and `vclk_get_dfs_info()` handles:
   - increasing `alloc_num_rates`, allocating `override_params`, and looking up the
     PLL index per clock via `vclk_pll_idx_for_rate()`.
   - inserting the level into `vclk->lut`, re-sorting (descending rates), and
     bumping `vclk->max_freq/boot_freq/resume_freq` if needed. Inspect the loop
     starting at `drivers/soc/samsung/cal-if/vclk.c:660` to understand the insertion flow.

3. **Update the Mali DT tables.**
   - `arch/arm64/boot/dts/exynos/exynos9820-mali_tables.dtsi` must list the same
     frequencies inside `gpu_dvfs_table`. Keep `gpu_max_clock`/`gpu_max_clock_limit`
     aligned to the highest override rate.
   - If you need a different startup or boot hint, change `gpu_dvfs_start_clock`
     / `gpu_dvfs_bl_config_clock` (and optionally `interactive_info` or
     `gpu_vk_boost_*`) inside `arch/arm64/boot/dts/samsung/exynos9820-mali_rev2.dtsi`.
   - Double-check that `gpu_sustainable_info` still matches the new bin range.

4. **Rebuild and verify the override path.**
   - After building, `cal_dfs_get_rate_table()` (called by sysfs/debugfs and CAL users)
     should list the new rate; it reuses `vclk->lut` including overrides (see the
     bottom of `drivers/soc/samsung/cal-if/cal-if.c`).
   - `cal_dfs_get_max_freq()` should now report `gpu_dvfs_override_highest_rate()`
     rather than the legacy max from the base DVFS table.
   - You can inspect `/sys/kernel/debug/clk/*/rates` or `cal_dfs_get_rate_table()`
     via `sysfs` to confirm the override is active.

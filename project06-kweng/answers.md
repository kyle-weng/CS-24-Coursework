# Q2
The inconsistency in the ratio is probably because when the CPU retrieves something and stores it in the cache, it doesn't necessarily place it in the same exact spot each time; thus, you can expect the time/number of clock cycles necessary to retrieve it from cache-- and thus the ratio-- to vary.
I have to run 100 trials before I find a consistent median of about 0.18.
# Q4
If the pages are not cleared from the cache, then it's possible to find multiple indices with very similar first and second read times. This is probably because there were multiple pages from the array already stored in cache.
# Q5
do_access would compile (it'd probably spit out a warning, though, because a character is not of type page_t *). If Q3 was run again with this change, the program would segfault. There is a way to recover the original character write a segfault signal handler to simply jump to a label you place somewhere in the program (as per Adam's lecture demoing such). Then, you can iterate through the page array and compare read times to find the index that corresponds to a character you're looking for.
# Q8
The attack can reveal pretty much any information that's currently being processed on a computer-- passwords, messages, photos, etc. I'm pretty sure there are demo videos out there of Meltdown being used to reveal this kind of stuff.
# Q9
ZombieLoad is a Meltdown-type attack; however, unlike Meltdown, which requires an explicit address to target (hence the address method we had to write), ZombieLoad doesn't need such an address. It recovers the values of previous memory ("zombie load") operations from the current or a sibling thread.
In more detail-- when the CPU finds that the data it needs is not in the L1 cache (to be specific, the data cache, as opposed to the instruction cache), it uses the line fill buffer as an interface between the L1 cache everything else-- the other caches and main memory. After the data it needs is loaded, the corresponding buffer entries are freed. When certain types of segfaults happen, the CPU may read values/do calculations before eventually rolling them back (as with all transient executions). These values, however, can still remain within the cache.
The attacker can't specify which address to attack; instead, ZombieLoad "leaks" any value that is currently being loaded or stored by the CPU.